#include "HitClusterFactory.hpp"

namespace nps::clustering {

void HitClusterFactory::Configure() {}

void HitClusterFactory::ChangeRun(int32_t run_number) {}

bool HitClusterFactory::PrepareTensorValues() {
	const auto &hits = m_fadchits();
	const size_t n_hits = hits.size();

	if (n_hits == 0) {
		return false;
	}

	for (size_t i = 0; i < n_hits; ++i) {
		const auto &charge = hits[i]->charge;
		const auto &time = hits[i]->time;
		m_input_tensors[0].fill_n(&charge, i * 2, 1);	// 1 element at offset i*2, no heap alloc
		m_input_tensors[0].fill_n(&time, i * 2 + 1, 1); // 1 element at offset i*2 + 1, no heap alloc
	}

	// Fill pos (col, row per hit)
	for (size_t i = 0; i < n_hits; ++i) {
		auto [col, row] = m_service_geometry().getColRowFromBlock(hits[i]->channel);
		const float pos[2] = {static_cast<float>(col), static_cast<float>(row)};
		m_input_tensors[1].fill_n(pos, i * 2, 2); // 2 elements at offset i*2, no heap alloc
	}

	// Fill masks
	m_input_tensors[2].fill(true); // all features valid (no padding)
	m_input_tensors[3].fill(true); // all graph-nodes valid (no downsampling)
	return true;
}

void HitClusterFactory::DeepCopyInput() {

	std::vector<nps::fadc_hit> copied_hits;
	copied_hits.reserve(m_fadchits().size());
	for (const auto *hit : m_fadchits()) {
		if (hit == nullptr) {
			continue;
		}
		copied_hits.push_back(*hit);
	}
	if (!copied_hits.empty()) {
		m_hit_queue.push_back(std::move(copied_hits));
	}
}

void HitClusterFactory::UnpackOutput() {
	auto &oc_head = m_oc_heads();

	auto &pos_tensor = m_input_tensors[1];
	auto &beta_tensor = m_output_tensors[0];
	auto &x_c_tensor = m_output_tensors[1];

	auto beta_tensor_dim = beta_tensor.dims();
	auto x_c_tensor_dim = x_c_tensor.dims();
	auto pos_tensor_dim = pos_tensor.dims();

	if (beta_tensor_dim.size() != 2) {
		std::string msg =
			"Unexpected beta tensor dimensions: " + std::to_string(beta_tensor_dim.size()) + " (expected 2)";
		throw std::runtime_error(msg);
	}
	if (x_c_tensor_dim.size() != 3) {
		std::string msg =
			"Unexpected x_c tensor dimensions: " + std::to_string(x_c_tensor_dim.size()) + " (expected 3)";
		throw std::runtime_error(msg);
	}
	if (beta_tensor_dim[0] != x_c_tensor_dim[0]) {
		std::string msg =
			"Batch size mismatch between beta tensor and x_c tensor: " + std::to_string(beta_tensor_dim[0]) + " vs " +
			std::to_string(x_c_tensor_dim[0]);
		throw std::runtime_error(msg);
	}
	if (beta_tensor_dim[1] != 1) {
		std::string msg =
			"Unexpected beta tensor second dimension: " + std::to_string(beta_tensor_dim[1]) + " (expected 1)";
		throw std::runtime_error(msg);
	}

	size_t batch_size = beta_tensor_dim[0];
	size_t n_hits = beta_tensor_dim[1];

	for (size_t i = 0; i < batch_size; ++i) {
		for (size_t j = 0; j < n_hits; ++j) {
			nps::oc_head head;

			head.event_index = m_event_index_queue[i];
			head.beta = static_cast<float *>(beta_tensor.data())[i * n_hits + j];
			head.x_c[0] = static_cast<float *>(x_c_tensor.data())[i * n_hits * 2 + j * 2];
			head.x_c[1] = static_cast<float *>(x_c_tensor.data())[i * n_hits * 2 + j * 2 + 1];
			head.channel = m_service_geometry().getBlockFromColRow(
				static_cast<float *>(pos_tensor.data())[i * n_hits * 2 + j * 2],
				static_cast<float *>(pos_tensor.data())[i * n_hits * 2 + j * 2 + 1]
			);
			m_oc_heads().push_back(new nps::oc_head(std::move(head)));
		}
	}
}

void HitClusterFactory::Describe() const {}

} // namespace nps::clustering
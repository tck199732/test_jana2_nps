#include "AiVtpClusterFactory.hpp"

namespace nps::clustering {

void AiVtpClusterFactory::Configure() {}

void AiVtpClusterFactory::ChangeRun(int32_t run_number) {}

void AiVtpClusterFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	if (m_rawhits().empty()) {
		return;
	}

	DeepCopyRawHits(m_rawhits());
	m_event_index_queue.push_back(event_index);

	if (m_rawhit_queue.size() < static_cast<size_t>(m_batch_size())) {
		return;
	}

	auto &session = m_service_onnx().createSession(m_session_name(), m_model_path(), m_onnx_nthreads(), m_use_cuda());
	try {
		PrepareTensors(session);
	} catch (const std::exception &e) {
		throw std::runtime_error("Failed to prepare tensors for ONNX Runtime session: " + std::string(e.what()));
	}

	std::vector<Ort::Value> input_tensors;
	std::vector<Ort::Value> output_tensors;
	std::vector<const char *> input_names;
	std::vector<const char *> output_names;

	input_tensors.reserve(m_input_tensors.size());
	output_tensors.reserve(m_output_tensors.size());
	input_names.reserve(m_input_tensors.size());
	output_names.reserve(m_output_tensors.size());

	for (auto &tensor : m_input_tensors) {
		input_tensors.push_back(tensor.GetOrtValue());
		input_names.push_back(tensor.name());
	}

	for (auto &tensor : m_output_tensors) {
		output_tensors.push_back(tensor.GetOrtValue());
		output_names.push_back(tensor.name());
	}

	try {
		session.Run(
			Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(), m_input_tensors.size(),
			output_names.data(), output_tensors.data(), m_output_tensors.size()
		);

	} catch (const Ort::Exception &e) {
		throw std::runtime_error("ONNX Runtime inference failed: " + std::string(e.what()));
	}

	PopulateOutput();

	m_rawhit_queue.clear();
	m_event_index_queue.clear();
}

void AiVtpClusterFactory::PrepareTensors(Ort::Session &session) {
	m_input_tensors.clear();
	m_output_tensors.clear();

	try {
		PrepareTensorInfo(session);
	} catch (const std::exception &e) {
		throw std::runtime_error("Failed to prepare tensor info for ONNX Runtime session: " + std::string(e.what()));
	}

	try {
		PrepareTensorValues();
	} catch (const std::exception &e) {
		throw std::runtime_error("Failed to prepare tensor values for ONNX Runtime session: " + std::string(e.what()));
	}
}

void AiVtpClusterFactory::PrepareTensorValues() {

	if (m_rawhit_queue.empty()) {
		throw std::runtime_error("Raw-hit queue is empty while preparing tensor values.");
	}
	if (m_input_tensors.size() < 4) {
		throw std::runtime_error("Expected at least 4 input tensors (x, pos, fea_mask, node_mask).");
	}
	const size_t batch = m_rawhit_queue.size();
	const size_t hits_per_event = m_rawhit_queue.front().size();

	if (batch == 0 || hits_per_event == 0) {
		throw std::runtime_error("Invalid batch/hit dimensions while preparing tensor values.");
	}

	if (m_input_tensors[0].n_elements() % batch != 0) {
		throw std::runtime_error("Input tensor x elements are not divisible by batch size.");
	}
	if (m_input_tensors[1].n_elements() % batch != 0) {
		throw std::runtime_error("Input tensor pos elements are not divisible by batch size.");
	}

	const size_t x_event_stride = m_input_tensors[0].n_elements() / batch;
	const size_t pos_event_stride = m_input_tensors[1].n_elements() / batch;
	if (x_event_stride % hits_per_event != 0) {
		throw std::runtime_error("Input tensor x stride is not divisible by hits per event.");
	}
	const size_t x_features_per_hit = x_event_stride / hits_per_event;

	if (pos_event_stride < hits_per_event * 2) {
		throw std::runtime_error("Position tensor is too small for [batch, hits, 2] layout.");
	}

	for (size_t ev = 0; ev < batch; ++ev) {
		const auto &ev_hits = m_rawhit_queue[ev];
		const size_t x_event_base = ev * x_event_stride;
		const size_t pos_event_base = ev * pos_event_stride;

		// Fill x (waveforms)
		size_t offset = x_event_base;
		for (size_t i = 0; i < ev_hits.size(); ++i) {
			const auto &waveform = ev_hits[i].getWaveform();
			if (waveform.size() > x_features_per_hit) {
				throw std::runtime_error(
					"Waveform length exceeds model feature dimension for tensor x: waveform.size=" +
					std::to_string(waveform.size()) + ", max_features=" + std::to_string(x_features_per_hit)
				);
			}
			m_input_tensors[0].fill_n(waveform.data(), offset, waveform.size());
			offset += x_features_per_hit;
		}

		// Fill pos (col, row per hit)
		for (size_t i = 0; i < ev_hits.size(); ++i) {
			auto [col, row] = m_service_geometry().getColRowFromBlock(ev_hits[i].getChannel());
			const float pos[2] = {static_cast<float>(col), static_cast<float>(row)};
			const size_t pos_offset = pos_event_base + i * 2;
			m_input_tensors[1].fill_n(pos, pos_offset, 2); // 2 elements at offset i*2 in this event slice
		}
	}

	// Fill masks
	m_input_tensors[2].fill(true); // all features valid (no padding in inference)
	m_input_tensors[3].fill(true); // all graph-nodes valid (no downsampling in inference)
}

void AiVtpClusterFactory::PrepareTensorInfo(Ort::Session &session) {

	const int64_t numHits = m_rawhit_queue.front().size();
	if (numHits <= 0) {
		throw std::runtime_error("Number of hits in an event must be greater than 0.");
	}
	for (const auto &ev_hits : m_rawhit_queue) {
		if (ev_hits.size() != numHits) {
			throw std::runtime_error("Inconsistent number of hits in events within the batch.");
		}
	}

	Ort::AllocatorWithDefaultOptions allocator;
	const size_t numInputNodes = session.GetInputCount();
	const size_t numOutputNodes = session.GetOutputCount();

	for (size_t i = 0; i < numInputNodes; ++i) {
		Ort::AllocatedStringPtr name = session.GetInputNameAllocated(i, allocator);
		Ort::TypeInfo type_info = session.GetInputTypeInfo(i);
		auto info = type_info.GetTensorTypeAndShapeInfo();
		auto shape = info.GetShape();
		auto type = info.GetElementType();

		if (shape.size() > 0 && shape[0] == -1) {
			shape.at(0) = m_batch_size();
		}
		if (shape.size() > 1 && shape[1] == -1) {
			shape.at(1) = numHits;
		}

		for (const auto &dim : shape) {
			if (dim <= 0) {
				throw std::runtime_error("Invalid tensor shape dimension: " + std::to_string(dim));
			}
		}
		m_input_tensors.push_back(onnx::Tensor::allocate(name.get(), shape, type));
	}

	for (size_t i = 0; i < numOutputNodes; ++i) {
		Ort::AllocatedStringPtr name = session.GetOutputNameAllocated(i, allocator);

		Ort::TypeInfo type_info = session.GetOutputTypeInfo(i);
		auto info = type_info.GetTensorTypeAndShapeInfo();
		auto shape = info.GetShape();
		auto type = info.GetElementType();

		if (shape.size() > 0 && shape[0] == -1) {
			shape.at(0) = m_batch_size();
		}
		if (shape.size() > 1 && shape[1] == -1) {
			shape.at(1) = numHits;
		}

		for (const auto &dim : shape) {
			if (dim <= 0) {
				throw std::runtime_error("Invalid tensor shape dimension: " + std::to_string(dim));
			}
		}
		m_output_tensors.push_back(onnx::Tensor::allocate(name.get(), shape, type));
	}
}

void AiVtpClusterFactory::DeepCopyRawHits(const std::vector<const nps::RawHit *> &rawhits) {
	std::vector<nps::RawHit> copied_hits;
	copied_hits.reserve(rawhits.size());
	for (const auto *hit : rawhits) {
		if (hit == nullptr) {
			continue;
		}
		copied_hits.push_back(*hit);
	}
	if (!copied_hits.empty()) {
		m_rawhit_queue.push_back(std::move(copied_hits));
	}
}

void AiVtpClusterFactory::PopulateOutput() {

	auto &pos_tensor = m_input_tensors[1];
	auto &x_c_tensor = m_output_tensors[0];
	auto &beta_tensor = m_output_tensors[1];

	const float *pos_data = static_cast<const float *>(pos_tensor.data());
	const float *x_c_data = static_cast<const float *>(x_c_tensor.data());
	const float *beta_data = static_cast<const float *>(beta_tensor.data());

	auto pos_shape = pos_tensor.dims();
	auto x_c_shape = x_c_tensor.dims();
	auto beta_shape = beta_tensor.dims();

	if (pos_shape[0] != beta_shape[0] || pos_shape[0] != x_c_shape[0] || pos_shape[0] != m_event_index_queue.size()) {

		auto msg = "Size mismatch in PopulateOutput: pos_shape[0]=" + std::to_string(pos_shape[0]) +
				   ", x_c_shape[0]=" + std::to_string(x_c_shape[0]) +
				   ", beta_shape[0]=" + std::to_string(beta_shape[0]) +
				   ", event_index_queue.size=" + std::to_string(m_event_index_queue.size());
		throw std::runtime_error(msg);
	}

	for (size_t i = 0; i < pos_shape[0]; ++i) {
		std::vector<double> x_c_vec = {x_c_data[i * x_c_shape[1]], x_c_data[i * x_c_shape[1] + 1]};
		double beta_val = static_cast<double>(beta_data[i * beta_shape[1]]);

		auto oc_output = new nps::clustering::ObjectCondensationOutput(x_c_vec, beta_val);

		auto ch = m_service_geometry().getBlockFromColRow(pos_data[i * pos_shape[1]], pos_data[i * pos_shape[1] + 1]);
		oc_output->setEventIndex(m_event_index_queue[i]);
		oc_output->setChannel(ch);

		m_oc_outputs().push_back(std::move(oc_output));
	}
}

void AiVtpClusterFactory::Describe() const {}

} // namespace nps::clustering
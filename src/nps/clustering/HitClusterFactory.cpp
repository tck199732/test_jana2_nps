#include "HitClusterFactory.hpp"

namespace nps::clustering {

void HitClusterFactory::Configure() {}

void HitClusterFactory::ChangeRun(int32_t run_number) {}

int HitClusterFactory::GetBatchSize() { return static_cast<int>(m_fadchits().size()); }

bool HitClusterFactory::PrepareTensorValues() {
	const auto &hits = m_fadchits();
	const size_t n_hits = hits.size();

	if (n_hits == 0) {
		return false;
	}

	size_t padding = 20;
	for (size_t i = 0; i < n_hits; ++i) {
		const auto &energy = hits[i]->getEnergy();
		const auto &time = hits[i]->getTime();
		m_input_tensors[0].fill_n(&energy, i * 2, 1);	// 1 element at offset i*2, no heap alloc
		m_input_tensors[0].fill_n(&time, i * 2 + 1, 1); // 1 element at offset i*2 + 1, no heap alloc
	}

	// Fill pos (col, row per hit)
	for (size_t i = 0; i < n_hits; ++i) {
		auto [col, row] = m_service_geometry().getColRowFromBlock(hits[i]->getChannel());
		const float pos[2] = {static_cast<float>(col), static_cast<float>(row)};
		m_input_tensors[1].fill_n(pos, i * 2, 2); // 2 elements at offset i*2, no heap alloc
	}

	// Fill masks
	m_input_tensors[2].fill(true); // all features valid (no padding)
	m_input_tensors[3].fill(true); // all graph-nodes valid (no downsampling)
	return true;
}

void HitClusterFactory::Describe() const {}

} // namespace nps::clustering
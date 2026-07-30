#include "WaveformClusterFactory.hpp"

namespace nps::clustering {

void WaveformClusterFactory::Configure() {}

void WaveformClusterFactory::ChangeRun(int32_t run_number) {}

int WaveformClusterFactory::GetBatchSize() { return static_cast<int>(m_rawhits().size()); }

bool WaveformClusterFactory::PrepareTensorValues() {
	const auto &hits = m_rawhits();
	const size_t n_hits = hits.size();

	if (n_hits == 0) {
		return false;
	}

	// Fill x (waveforms)
	size_t offset = 0;
	for (size_t i = 0; i < n_hits; ++i) {
		const auto &waveform = hits[i]->waveform;
		m_input_tensors[0].fill_n(waveform.data(), offset, waveform.size());
		offset += waveform.size();
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

void WaveformClusterFactory::Describe() const {}

} // namespace nps::clustering
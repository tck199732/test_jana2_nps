#include "EvioParsingFactory.hpp"

namespace nps::clustering {

void EvioParsingFactory::Configure() {}

void EvioParsingFactory::ChangeRun(int32_t run_number) {}

void EvioParsingFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	for (auto &data : m_block_data()) {
		PopulateFadcHits(data);
		PopulateFadcWaveforms(data);
	}
	return;
}

void EvioParsingFactory::PopulateFadcHits(const evio::sro::SroBlockData &data) {
	auto &output_hits = m_fadc_hits();
	output_hits.reserve(output_hits.size() + data.fadc_hits.size());

	for (const auto &evio_fadc : data.fadc_hits) {
		output_hits.emplace_back(
			nps::fadc_hit{
				.channel = evio_fadc.channel,
				.charge = evio_fadc.charge,
				.time = evio_fadc.time_ticks,
			}
		);
	}
}

void EvioParsingFactory::PopulateFadcWaveforms(const evio::sro::SroBlockData &data) {
	// no implementation yet
}

} // namespace nps::clustering
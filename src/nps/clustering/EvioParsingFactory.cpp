#include "EvioParsingFactory.hpp"

namespace nps::clustering {

void EvioParsingFactory::Configure() {}

void EvioParsingFactory::ChangeRun(int32_t run_number) {}

void EvioParsingFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	std::vector<const evio::sro::SroBlockData *> block_data = m_block_data();
	for (const auto *data : block_data) {
		PopulateFadcHits(data);
		PopulateFadcWaveforms(data);
	}
	return;
}

void EvioParsingFactory::PopulateFadcHits(const evio::sro::SroBlockData *data) {

	for (const auto &evio_fadc : data->fadc_hits) {
		m_fadc_hits().emplace_back(new nps::fadc_hit{
			.channel = evio_fadc.channel,
			.charge = evio_fadc.charge,
			.time = evio_fadc.time_ticks,
		});
	}
}

void EvioParsingFactory::PopulateFadcWaveforms(const evio::sro::SroBlockData *data) {
	// no implementation yet
}

} // namespace nps::clustering
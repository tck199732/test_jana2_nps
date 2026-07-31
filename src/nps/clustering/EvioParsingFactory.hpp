#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "evio/sro/SroBlockReader.hpp"
#include "struct/fadc_hits.hpp"
#include "struct/fadc_waveform.hpp"

#include <cassert>
#include <unordered_set>
#include <vector>

namespace nps::clustering {

class EvioParsingFactory : public JOmniFactory<EvioParsingFactory> {
public:
	Input<evio::sro::SroBlockData> m_block_data{this, {"sro_block_data"}};
	Output<nps::fadc_hit> m_fadc_hits{this, {"fadc_hits"}};
	Output<nps::fadc_waveform> m_fadc_waveforms{this, {"fadc_waveforms"}};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);

private:
	void PopulateFadcHits(const evio::sro::SroBlockData &data);
	void PopulateFadcWaveforms(const evio::sro::SroBlockData &data);
};

} // namespace nps::clustering
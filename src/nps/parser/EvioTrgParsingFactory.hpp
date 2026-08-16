#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "evio/halld_modules/data_struct.hpp"
#include "struct/fadc.hpp"

#include <cassert>
#include <unordered_set>
#include <vector>

namespace nps::clustering {

class EvioTrgParsingFactory : public JOmniFactory<EvioTrgParsingFactory> {
public:
	Input<halld::evio::ParsedEvent> m_parsed_events{this, {"evio_hd_trg_data"}};
	Output<nps::fadc_window_raw_record> m_f250_wraw{this, {"f250_wraw"}};
	Output<nps::fadc_pulse_record> m_f250_pulse{this, {"f250_pulse"}};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);

private:
	void PopulateFadcHits(const halld::evio::ParsedEvent *event);
	void PopulateFadcWaveforms(const halld::evio::ParsedEvent *event);
};

} // namespace nps::clustering
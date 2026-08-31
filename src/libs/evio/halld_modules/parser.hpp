

#pragma once

#include <atomic>
#include <chrono>
#include <cinttypes>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "daq_param_type.hpp"
#include "data_struct.hpp"
#include "module_type.hpp"

namespace halld::evio {

class EvioBlockedEventParser {
	size_t ievent_idx = 0;
	std::vector<ParsedEvent> m_parsed_events;

public:
	EvioBlockedEventParser() = default;
	~EvioBlockedEventParser() { m_parsed_events.clear(); };

	void ParseBlock(std::vector<uint32_t> &block);

	void ParseBank(uint32_t *istart, uint32_t *iend);
	void ParseEPICSbank(uint32_t *&iptr, uint32_t *iend);
	void ParseBORbank(uint32_t *&iptr, uint32_t *iend);
	void ParseControlEvent(uint32_t *&iptr, uint32_t *iend);
	void ParsePhysicsBank(uint32_t *&iptr, uint32_t *iend);
	void ParseCDAQBank(uint32_t *&iptr, uint32_t *iend);
	void ParseBuiltTriggerBank(uint32_t *&iptr, uint32_t *iend);
	void ParseDataBank(uint32_t *&iptr, uint32_t *iend);

	void ParseCAEN1190(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseModuleConfiguration(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseEventTagBank(uint32_t *&iptr, uint32_t *iend);
	void ParseJLabModuleData(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseSSPBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseTSscalerBank(uint32_t *&iptr, uint32_t *iend);
	void Parsef250scalerBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseRawTriggerBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseDGEMSRSBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);

	void MakeDGEMSRSWindowRawData(
		ParsedEvent &event, uint32_t rocid, uint32_t slot, uint32_t itrigger, uint32_t apv_id,
		const std::vector<int> &rawData16bits
	);
	// MIGRATION (JANA2 2026.x): JEvent::Insert(vector) now REPLACES the databundle
	// instead of appending, so per-APV inserts would drop all but the last APV.
	// SRS objects are accumulated here and inserted once per event.
	// void FlushDGEMSRSWindowRawData(JEvent *event);
	// std::vector<DGEMSRSWindowRawData *> m_srs_accumulator;
	void Parsef250Bank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void MakeDf250WindowRawData(
		ParsedEvent &event, uint32_t rocid, uint32_t slot, uint32_t itrigger, uint32_t *&iptr, uint32_t *iend
	);
	void Parsef125Bank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void MakeDf125WindowRawData(
		ParsedEvent &event, uint32_t rocid, uint32_t slot, uint32_t itrigger, uint32_t *&iptr, uint32_t *iend
	);
	void ParseF1TDCBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);
	void ParseTIBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend);

	ParsedEvent GetParsedEvent() const { return m_parsed_events[ievent_idx]; }
	std::vector<ParsedEvent> GetParsedEvents() const { return m_parsed_events; }
};

} // namespace halld::evio
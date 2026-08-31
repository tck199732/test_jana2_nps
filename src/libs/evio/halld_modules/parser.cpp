

#include "parser.hpp"

namespace halld::evio {

void EvioBlockedEventParser::ParseBlock(std::vector<uint32_t> &block) {

	uint32_t *iptr = block.data();

	// Extract number of events in this block at the event number of the first
	uint32_t M = 1;
	uint64_t event_num = 0;
	iptr++;
	uint32_t mask = 0xFF001000;
	if ((*iptr) >> 16 == 0xFF32) {

		// CDAQ BOR. Leave M=1

	} else if ((*iptr) >> 16 == 0xFF33) {

		// CDAQ Physics event
		M = iptr[2] & 0xFF;

		// Event number taken from first ROC's trigger bank
		uint64_t eventnum_lo = iptr[6];
		uint64_t eventnum_hi = 0; // Only lower 32bits in ROC trigger info.
		event_num = (eventnum_hi << 32) + (eventnum_lo);
	} else if (((*iptr) & mask) == mask) {
		// CODA Physics event
		M = *(iptr) & 0xFF;
		uint64_t eventnum_lo = iptr[4];
		uint64_t eventnum_hi = iptr[5];
		event_num = (eventnum_hi << 32) + (eventnum_lo);
	}

	// Sanity chack that this block does not contain a ridiculous number of m_parsed_events
	if (M > 200) {
		std::string msg = "Too many m_parsed_events in EVIO block (" + std::to_string(M) + " > 200)";
		throw std::runtime_error(msg);
	}

	m_parsed_events.clear();
	m_parsed_events.resize(M);

	ievent_idx = 0; // start filling m_parsed_events at the beginning

	uint32_t *istart = block.data();
	uint32_t *iend = &istart[block.size()];
	ParseBank(istart, iend);
}

void EvioBlockedEventParser::ParseBank(uint32_t *istart, uint32_t *iend) {
	uint32_t *iptr = istart;

	while (iptr < iend) {
		uint32_t event_len = iptr[0];
		uint32_t event_head = iptr[1];
		uint32_t tag = (event_head >> 16) & 0xFFFF;

		switch (tag) {
		case 0x0060:
			ParseEPICSbank(iptr, iend);
			break;
		case 0x0070:
			ParseBORbank(iptr, iend);
			break;

		case 0xFFD0:
		case 0xFFD1:
		case 0xFFD2:
		case 0xFFD3:
		case 0xFFD4:
			ParseControlEvent(iptr, iend);
			break;

		case 0xFF58:
		case 0xFF78:
		case 0xFF50:
		case 0xFF70:
			ParsePhysicsBank(iptr, iend);
			break;
		case 0xFF32:
		case 0xFF33:
			ParseCDAQBank(iptr, iend);
			break;

		default:
			iptr = &iptr[event_len + 1];
			if (event_len < 1) {
				iptr = iend;
			}
		}
	}
}

void EvioBlockedEventParser::ParseEPICSbank(uint32_t *&iptr, uint32_t *iend) {

	std::time_t timestamp = 0;

	// Outer bank
	uint32_t *istart = iptr;
	uint32_t epics_bank_len = *iptr++;
	if (epics_bank_len < 1) {
		iptr = iend;
		return;
	}

	uint32_t *iend_epics = &iptr[epics_bank_len];

	// Advance to first daughter bank
	iptr++;

	// Get pointer to the event we should place this in and
	// also advance ievent_idx for next event.
	auto &event = m_parsed_events[ievent_idx++];
	// DParsedEvent *pe = current_parsed_events.front();
	// pe->event_status_bits |= (1<<kSTATUS_EPICS_EVENT);

	// Loop over daughter banks
	while (iptr < iend_epics) {

		uint32_t bank_len = (*iptr) & 0xFFFF;
		uint32_t tag = ((*iptr) >> 24) & 0xFF;
		iptr++;

		if (tag == 0x61) {
			// timestamp bank
			timestamp = *iptr;
		} else if (tag == 0x62) {
			std::string nameval = (const char *)iptr;

			EPICSvalue epics_value;
			epics_value.timestamp = timestamp;
			epics_value.nameval = nameval;
			event.epics_values.push_back(epics_value);

		} else {
			std::string msg = "Unknown tag 0x" + std::to_string(tag) + " in EPICS event!";
			throw std::runtime_error(msg);
		}

		iptr = &iptr[bank_len];
	}

	iptr = iend_epics;
}

void EvioBlockedEventParser::ParseBORbank(uint32_t *&iptr, uint32_t *iend) {
	ievent_idx++;
	iptr = iend;
}

void EvioBlockedEventParser::ParseControlEvent(uint32_t *&iptr, uint32_t *iend) {
	auto &event = m_parsed_events[ievent_idx++];

	std::time_t t = (std::time_t)iptr[2];
	std::string tstr = ctime(&t);
	if (tstr.size() > 1) {
		tstr.erase(tstr.size() - 1);
	}

	std::string type = "Control";
	switch (iptr[1] >> 16) {
	case 0XFFD0:
		type = "Sync";
		break;
	case 0XFFD1:
		type = "Prestart";
		break;
	case 0XFFD2:
		type = "Go";
		break;
	case 0XFFD3:
		type = "Pause";
		break;
	case 0XFFD4:
		type = "End";
		break;
	}

	auto event_type = iptr[1] >> 16;
	CODAControlEvent controlevent{
		.event_type = static_cast<uint16_t>(event_type),
		.unix_time = static_cast<uint32_t>(t),
	};
	for (auto p = iptr; p != iend; p++) {
		controlevent.words.push_back(*p);
	}

	event.control_events.push_back(controlevent);

	iptr = iend;
}

void EvioBlockedEventParser::ParsePhysicsBank(uint32_t *&iptr, uint32_t *iend) {
	uint32_t physics_event_len = *iptr++;
	uint32_t *iend_physics_event = &iptr[physics_event_len];
	iptr++;

	// Built Trigger Bank
	uint32_t built_trigger_bank_len = *iptr;
	uint32_t *iend_built_trigger_bank = &iptr[built_trigger_bank_len + 1];
	ParseBuiltTriggerBank(iptr, iend_built_trigger_bank);
	iptr = iend_built_trigger_bank;

	// Loop over Data banks
	while (iptr < iend_physics_event) {

		uint32_t data_bank_len = *iptr;
		uint32_t *iend_data_bank = &iptr[data_bank_len + 1];

		ParseDataBank(iptr, iend_data_bank);

		iptr = iend_data_bank;
	}

	iptr = iend_physics_event;
}

void EvioBlockedEventParser::ParseCDAQBank(uint32_t *&iptr, uint32_t *iend) {

	// Check if this is a BOR event
	if ((iptr[1] & 0xFFFF0000) == 0xFF320000) {
		iptr += 2;
		try {
			ParseBORbank(iptr, iend);
		} catch (const std::exception &e) {
			std::cerr << "Error parsing BOR bank: " << e.what() << std::endl;
		}
		return;
	}

	// Must be physics event(s)
	// for(auto pe : current_parsed_events) pe->event_status_bits |= (1<<kSTATUS_PHYSICS_EVENT) + (1<<kSTATUS_CDAQ);

	// Set flag in JEventSource_EVIOpp that this is a CDAQ file
	// event_source->IS_CDAQ_FILE = true;

	uint32_t physics_event_len = *iptr++;
	uint32_t *iend_physics_event = &iptr[physics_event_len];
	iptr++;

	// Loop over Data banks
	while (iptr < iend_physics_event) {

		uint32_t data_bank_len = *iptr;
		uint32_t *iend_data_bank = &iptr[data_bank_len + 1];

		ParseDataBank(iptr, iend_data_bank);

		iptr = iend_data_bank;
	}

	iptr = iend_physics_event;
}

void EvioBlockedEventParser::ParseBuiltTriggerBank(uint32_t *&iptr, uint32_t *iend) {
	iptr++; // advance past length word
	uint32_t mask = 0xFF202000;
	if (((*iptr) & mask) != mask) {
		std::stringstream ss;
		ss << "Bad header word in Built Trigger Bank: " << std::hex << *iptr;
		throw std::runtime_error(ss.str());
	}

	uint32_t tag = (*iptr) >> 16; // 0xFF2X
	uint32_t Nrocs = (*iptr++) & 0xFF;
	uint32_t Mevents = m_parsed_events.size();

	// sanity check:
	if (Mevents == 0) {
		std::stringstream ss;
		ss << "EvioBlockedEventParser::ParseBuiltTriggerBank() called with zero m_parsed_events! " << std::endl;
		;
		throw std::runtime_error(ss.str());
	}

	//-------- Common data (64bit)
	uint32_t common_header64 = *iptr++;
	uint32_t common_header64_len = common_header64 & 0xFFFF;
	uint64_t *iptr64 = (uint64_t *)iptr;
	iptr = &iptr[common_header64_len];

	// First event number
	uint64_t first_event_num = *iptr64++;

	// Hi and lo 32bit words in 64bit numbers seem to be
	// switched for m_parsed_events read from ET, but not read from
	// file. Not sure if this is in the swapping routine
	//    if(event_source->source_type==event_source->kETSource) first_event_num = (first_event_num>>32) |
	//    (first_event_num<<32);

	// Average timestamps
	uint32_t Ntimestamps = (common_header64_len / 2) - 1;
	if (tag & 0x2) {
		Ntimestamps--;
		// subtract 1 for run number/type word if present
	}
	std::vector<uint64_t> avg_timestamps;
	for (uint32_t i = 0; i < Ntimestamps; i++) {
		avg_timestamps.push_back((uint64_t)(*iptr64++));
	}

	// run number and run type
	uint32_t run_number = 0;
	uint32_t run_type = 0;
	if (tag & 0x02) {
		run_number = (*iptr64) >> 32;
		run_type = (*iptr64) & 0xFFFFFFFF;
		iptr64++;
	}

	//-------- Common data (16bit)
	uint32_t common_header16 = *iptr++;
	uint32_t common_header16_len = common_header16 & 0xFFFF;
	uint16_t *iptr16 = (uint16_t *)iptr;
	iptr = &iptr[common_header16_len];

	std::vector<uint16_t> event_types;
	for (uint32_t i = 0; i < Mevents; i++) {
		event_types.push_back(*iptr16++);
	}

	//-------- ROC data (32bit)
	for (uint32_t iroc = 0; iroc < Nrocs; iroc++) {
		uint32_t common_header32 = *iptr++;
		uint32_t common_header32_len = common_header32 & 0xFFFF;
		uint32_t rocid = common_header32 >> 24;

		uint32_t Nwords_per_event = common_header32_len / Mevents;
		for (auto &event : m_parsed_events) {

			CODAROCInfo codarocinfo{.rocid = rocid, .timestamp = 0, .misc = {}};

			uint64_t ts_low = *iptr++;
			uint64_t ts_high = *iptr++;
			codarocinfo.timestamp = (ts_high << 32) + ts_low;
			codarocinfo.misc.clear(); // could be recycled from previous event
			for (uint32_t i = 2; i < Nwords_per_event; i++) {
				codarocinfo.misc.push_back(*iptr++);
			}

			event.roc_infos.push_back(codarocinfo);

			if (iptr > iend) {
				throw std::runtime_error("Bad data format in ParseBuiltTriggerBank!");
			}
		}
	}

	uint64_t ievent = 0;
	for (auto &event : m_parsed_events) {

		event.event_info = {
			.run_number = run_number,
			.run_type = run_type,
			.event_number = first_event_num + ievent,
			.event_type = event_types.empty() ? 0 : event_types[ievent],
			.avg_timestamp = avg_timestamps.empty() ? 0 : avg_timestamps[ievent],
		};

		ievent++;
	}
}

void EvioBlockedEventParser::ParseDataBank(uint32_t *&iptr, uint32_t *iend) {
	// Physics Event's Data Bank header
	iptr++; // advance past data bank length word
	uint32_t rocid = ((*iptr) >> 16) & 0xFFF;
	iptr++;

	// Loop over Data Block Banks
	while (iptr < iend) {

		uint32_t data_block_bank_len = *iptr++;
		uint32_t *iend_data_block_bank = &iptr[data_block_bank_len];
		uint32_t data_block_bank_header = *iptr++;

		// Not sure where this comes from, but it needs to be skipped if present
		while ((*iptr == 0xF800FAFA) && (iptr < iend)) {
			iptr++;
		}

		uint32_t det_id = (data_block_bank_header >> 16) & 0xFFF;
		switch (det_id) {

		case 20:
			ParseCAEN1190(rocid, iptr, iend_data_block_bank);
			break;

		case 0x55:
			ParseModuleConfiguration(rocid, iptr, iend_data_block_bank);
			break;

		case 0x56:
			ParseEventTagBank(iptr, iend_data_block_bank);
			break;

		case 0:
		case 1:
		case 3:
		case 6:	 // flash 250 module, MMD 2014/2/4
		case 16: // flash 125 module (CDC), DL 2014/6/19
		case 26: // F1 TDC module (BCAL), MMD 2014-07-31
			ParseJLabModuleData(rocid, iptr, iend_data_block_bank);
			break;

		case 0x123:
		case 0x28:
			ParseSSPBank(rocid, iptr, iend_data_block_bank);
			break;

			// These were implemented in the ROL for sync m_parsed_events
			// as 0xEE02 and 0xEE05. However, that violates the
			// spec. which reserves the top 4 bits as status bits
			// (the first "E" should really be a "1". We just check
			// other 12 bits here.
		case 0xE02:
			ParseTSscalerBank(iptr, iend);
			break;
		case 0xE05:
			//				Parsef250scalerBank(iptr, iend);
			break;
		case 0xE10:
			Parsef250scalerBank(rocid, iptr, iend_data_block_bank);
			break;

			// The CDAQ system leave the raw trigger info in the Physics event data
			// bank. Skip it for now.
		case 0xF11:
			ParseRawTriggerBank(rocid, iptr, iend_data_block_bank);
			break;

			// // When we write out single m_parsed_events in the offline, we also can save some
			// // higher level data objects to save disk space and speed up
			// // specialized processing (e.g. pi0 calibration)
			// case 0xD01:
			// 	ParseDVertexBank(iptr, iend_data_block_bank);
			// 	break;
			// case 0xD02:
			// 	ParseDEventRFBunchBank(iptr, iend_data_block_bank);
			// 	break;

			// case 5:
			// 	// old ROL Beni used had this but I don't think its
			// 	// been used for years. Run 10390 seems to have
			// 	// this though (???)
			// 	break;

		case 0x11:
			// attempt at GEM SRS parsing
			ParseDGEMSRSBank(rocid, iptr, iend_data_block_bank);
			break;

		case 0xDEC:
			// JLab helicity decoder board (parsed in halld_recon since 2025). Not used here - skip.
			break;

		default: {
			std::cerr << "Unknown module type (" << det_id << " = 0x" << std::hex << det_id << std::dec
					  << " ) encountered. Skipping bank." << std::endl;
		}
		}

		iptr = iend_data_block_bank;
	}
}

void EvioBlockedEventParser::ParseModuleConfiguration(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseEventTagBank(uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseJLabModuleData(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) {
	while (iptr < iend) {

		// Get module type from next word (bits 18-21)
		uint32_t mod_id = ((*iptr) >> 18) & 0x000F;
		module_type mod_type = static_cast<module_type>(mod_id);

		switch (mod_type) {
		case module_type::FADC250:
			Parsef250Bank(rocid, iptr, iend);
			break;

		case module_type::FADC125:
			Parsef125Bank(rocid, iptr, iend);
			break;

		case module_type::F1TDC32:
			ParseF1TDCBank(rocid, iptr, iend);
			break;

		case module_type::F1TDC48:
			ParseF1TDCBank(rocid, iptr, iend);
			break;

		case module_type::TID:
			ParseTIBank(rocid, iptr, iend);
			break;

		case module_type::UNKNOWN:
		default:
			// Skip to JLab block trailer
			while (iptr < iend && ((*iptr) & 0xF8000000) != 0x88000000) {
				iptr++;
			}
			iptr++; // advance past JLab block trailer

			// skip filler words after block trailer
			while (iptr < iend && *iptr == 0xF8000000) {
				iptr++;
			}
			throw std::runtime_error("Unknown JLab module type");
			break;
		}
	}
}

void EvioBlockedEventParser::ParseRawTriggerBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) {

	for (auto &event : m_parsed_events) {

		uint32_t segment_header = *iptr++;
		uint32_t segment_len = segment_header & 0xFFFF;
		uint32_t *iend_segment = &iptr[segment_len];

		iptr++;
		uint64_t ts_low = *iptr++;
		uint64_t ts_high = *iptr++;

		CODAROCInfo codarocinfo{.rocid = rocid, .timestamp = (ts_high << 32) + ts_low, .misc = {}};

		// rocid=1 is TS and produces 2 extra words for the trigger bits
		for (uint32_t i = 3; i < segment_len; i++) {
			codarocinfo.misc.push_back(*iptr++);
		}

		event.roc_infos.push_back(codarocinfo);

		if (iptr != iend_segment) {
			throw std::runtime_error("Bad raw trigger bank format");
		}
	}
}

void EvioBlockedEventParser::Parsef250Bank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) {

	int continue_on_format_error = false;

	ParsedEvent *event = nullptr;
	auto ievent = ievent_idx;

	uint32_t slot = 0;
	uint32_t itrigger = -1;

	uint32_t *istart_pulse_data = iptr;

	// Loop over data words
	for (; iptr < iend; iptr++) {

		// Skip all non-data-type-defining words at this
		// level. When we do encounter one, the appropriate
		// case block below should handle parsing all of
		// the data continuation words and advance the iptr.
		if (((*iptr >> 31) & 0x1) == 0)
			continue;

		uint32_t data_type = (*iptr >> 27) & 0x0F;
		switch (data_type) {
		case 0: // Block Header
			slot = (*iptr >> 22) & 0x1F;
			break;
		case 1: // Block Trailer
			event = nullptr;
			break;
		case 2: // Event Header
			itrigger = (*iptr >> 0) & 0x3FFFFF;
			if (ievent >= m_parsed_events.size()) {
				std::cerr << "FADC250: more event headers than events in block! rocid=" << rocid << " slot=" << slot
						  << ". Skipping rest of bank." << std::endl;
				iptr = iend;
				return;
			}
			event = &m_parsed_events[ievent++];
			break;
		case 3: // Trigger Time
		{
			uint64_t t = ((*iptr) & 0xFFFFFF) << 0;
			iptr++;
			if (((*iptr >> 31) & 0x1) == 0) {
				t += ((*iptr) & 0xFFFFFF) << 24; // from word on the street: second trigger time word is optional!!??
			} else {
				iptr--;
			}
			if (event) {
				f250TriggerTime fadc_trg_times(rocid, slot, 0, itrigger);
				fadc_trg_times.time = t;
				event->f250_trigger_times.push_back(std::move(fadc_trg_times));
			}
		} break;
		case 4: // Window Raw Data
			if (event) {
				MakeDf250WindowRawData(*event, rocid, slot, itrigger, iptr, iend);
			}
			break;
		case 5: // Window Sum
		{
			uint32_t channel = (*iptr >> 23) & 0x0F;
			uint32_t sum = (*iptr >> 0) & 0x3FFFFF;
			uint32_t overflow = (*iptr >> 22) & 0x1;
			if (event) {
				f250WindowSum fadc_window_sum(rocid, slot, channel, itrigger);
				fadc_window_sum.sum = sum;
				fadc_window_sum.overflow = overflow;
				event->f250_window_sums.push_back(std::move(fadc_window_sum));
			}
		} break;
		case 6: // Pulse Raw Data
			break;
		case 7: // Pulse Integral
		{
			uint32_t channel = (*iptr >> 23) & 0x0F;
			uint32_t pulse_number = (*iptr >> 21) & 0x03;
			uint32_t quality_factor = (*iptr >> 19) & 0x03;
			uint32_t sum = (*iptr >> 0) & 0x7FFFF;
			uint32_t nsamples_integral = 0; // must be overwritten later in GetObjects with value from Df125Config value
			uint32_t nsamples_pedestal = 1; // The firmware returns an already divided pedestal
			uint32_t pedestal = 0;			// This will be replaced by the one from Df250PulsePedestal in GetObjects
			if (event) {

				f250PulseIntegral pulse_integral(rocid, slot, channel, itrigger);
				pulse_integral.pulse_number = pulse_number;
				pulse_integral.quality_factor = quality_factor;
				pulse_integral.integral = sum;
				pulse_integral.pedestal = pedestal;
				pulse_integral.nsamples_integral = nsamples_integral;
				pulse_integral.nsamples_pedestal = nsamples_pedestal;
				event->f250_pulse_integrals.push_back(std::move(pulse_integral));
			}

		} break;
		case 8: // Pulse Time
		{
			uint32_t channel = (*iptr >> 23) & 0x0F;
			uint32_t pulse_number = (*iptr >> 21) & 0x03;
			uint32_t quality_factor = (*iptr >> 19) & 0x03;
			uint32_t pulse_time = (*iptr >> 0) & 0x7FFFF;
			if (event) {

				f250PulseTime pulse_time_obj(rocid, slot, channel, itrigger);
				pulse_time_obj.pulse_number = pulse_number;
				pulse_time_obj.quality_factor = quality_factor;
				pulse_time_obj.time = pulse_time;
				event->f250_pulse_times.push_back(std::move(pulse_time_obj));
			}
		} break;
		case 9: // Pulse Data (firmware instroduce in Fall 2016)
		{
			// from word 1
			uint32_t event_number_within_block = (*iptr >> 19) & 0xFF;
			uint32_t channel = (*iptr >> 15) & 0x0F;
			bool QF_pedestal = (*iptr >> 14) & 0x01;
			uint32_t pedestal = (*iptr >> 0) & 0x3FFF;

			// event_number_within_block=0 indicates error
			if (event_number_within_block == 0) {
				std::exit(-1);
			}

			// Event headers may be supressed so determine event from hit data
			// 2/26/2023 DL  -Note sure I understand this. It looks like this extracts the
			//                relative event number from this hit whereas the other hit info
			//                keeps track of the relative event by how many header words were seen.
			//                What is really confusing is that this seems to use and increment
			//                the same pe and pe_iter used by the other data types here(??!)
			if ((event_number_within_block > m_parsed_events.size())) {

				std::string err = "Bad f250 event number for rocid=" + std::to_string(rocid) +
								  " slot=" + std::to_string(slot) + " channel=" + std::to_string(channel);
				throw std::runtime_error(err);
			}
			event = &m_parsed_events[event_number_within_block - 1];

			itrigger = event_number_within_block; // is this right?
			uint32_t pulse_number = 0;

			while ((*++iptr >> 31) == 0) {

				if ((*iptr >> 30) != 0x01) {
					if (continue_on_format_error) {
						iptr = iend;
						return;
					} else
						throw std::runtime_error("Bad f250 Pulse Data!");
				}

				// from word 2
				uint32_t integral = (*iptr >> 12) & 0x3FFFF;
				bool QF_NSA_beyond_PTW = (*iptr >> 11) & 0x01;
				bool QF_overflow = (*iptr >> 10) & 0x01;
				bool QF_underflow = (*iptr >> 9) & 0x01;
				uint32_t nsamples_over_threshold = (*iptr >> 0) & 0x1FF;

				iptr++;
				if ((*iptr >> 30) != 0x00) {
					if (continue_on_format_error) {
						iptr = iend;
						return;
					} else
						throw std::runtime_error("Bad f250 Pulse Data!");
				}

				// from word 3
				uint32_t course_time = (*iptr >> 21) & 0x1FF; //< 4 ns/count
				uint32_t fine_time = (*iptr >> 15) & 0x3F;	  //< 0.0625 ns/count
				uint32_t pulse_peak = (*iptr >> 3) & 0xFFF;
				bool QF_vpeak_beyond_NSA = (*iptr >> 2) & 0x01;
				bool QF_vpeak_not_found = (*iptr >> 1) & 0x01;
				bool QF_bad_pedestal = (*iptr >> 0) & 0x01;

				// FIRMWARE BUG: If pulse integral was zero, this is an invalid bad pulse;
				// skip over bogus repeated pulse time repeats, and ignore it altogether.
				// March 18, 2020 -rtj-
				if (integral == 0 && *iptr == *(iptr + 1)) {
					while (*(iptr + 1) == *iptr) {
						++iptr;
					}
					std::cerr << "Bug #1: bad f250 Pulse Data for rocid=" << rocid << " slot=" << slot
							  << " channel=" << channel << std::endl;
					continue_on_format_error = true;
					break;
				}

				if (event) {

					f250PulseData pulse_data(rocid, slot, channel, itrigger);
					pulse_data.event_within_block = event_number_within_block;
					pulse_data.QF_pedestal = QF_pedestal;
					pulse_data.pedestal = pedestal;
					pulse_data.integral = integral;
					pulse_data.QF_NSA_beyond_PTW = QF_NSA_beyond_PTW;
					pulse_data.QF_overflow = QF_overflow;
					pulse_data.QF_underflow = QF_underflow;
					pulse_data.nsamples_over_threshold = nsamples_over_threshold;
					pulse_data.course_time = course_time;
					pulse_data.fine_time = fine_time;
					pulse_data.pulse_peak = pulse_peak;
					pulse_data.QF_vpeak_beyond_NSA = QF_vpeak_beyond_NSA;
					pulse_data.QF_vpeak_not_found = QF_vpeak_not_found;
					pulse_data.QF_bad_pedestal = QF_bad_pedestal;
					pulse_data.pulse_number = pulse_number++;
					event->f250_pulse_data.push_back(std::move(pulse_data));
				}
			}
			iptr--; // backup so when outer loop advances, it points to next data defining word

		} break;
		case 10: // Pulse Pedestal
		{
			uint32_t channel = (*iptr >> 23) & 0x0F;
			uint32_t pulse_number = (*iptr >> 21) & 0x03;
			uint32_t pedestal = (*iptr >> 12) & 0x1FF;
			uint32_t pulse_peak = (*iptr >> 0) & 0xFFF;
			if (event) {

				f250PulsePedestal pulse_pedestal(rocid, slot, channel, itrigger);
				pulse_pedestal.pulse_number = pulse_number;
				pulse_pedestal.pedestal = pedestal;
				pulse_pedestal.pulse_peak = pulse_peak;
				event->f250_pulse_pedestals.push_back(std::move(pulse_pedestal));
			}
		} break;
		case 13: // Event Trailer
				 // This is marked "suppressed for normal readout – debug mode only" in the
				 // current manual (v2). It does not contain any data so the most we could do here
				 // is return early. I'm hesitant to do that though since it would mean
				 // different behavior for debug mode data as regular data.
		case 14: // Data not valid (empty module)
		case 15: // Filler (non-data) word
			break;
		default:
			std::string err =
				"FADC250 unknown data type (" + std::to_string(data_type) + ") (0x" + std::to_string(*iptr) + ")";
			if (continue_on_format_error) {
				iptr = iend;
				return;
			} else
				throw std::runtime_error(err);
		}
	}

	// Chop off filler words
	for (; iptr < iend; iptr++) {
		if (((*iptr) & 0xf8000000) != 0xf8000000) {
			break;
		}
	}
}

void EvioBlockedEventParser::MakeDf250WindowRawData(
	ParsedEvent &event, uint32_t rocid, uint32_t slot, uint32_t itrigger, uint32_t *&iptr, uint32_t *iend
) {
	uint32_t channel = (*iptr >> 23) & 0x0F;
	uint32_t window_width = (*iptr >> 0) & 0x0FFF;

	f250WindowRawData wrd(rocid, slot, channel, itrigger);

	for (uint32_t isample = 0; isample < window_width; isample += 2) {

		// Advance to next word
		iptr++;

		// Truncated data: block ends before all advertised samples (halld_recon fix ff374582)
		if (iptr >= iend) {
			static std::atomic<int> warn_count{0};
			std::cerr << "fa250 window raw data are incomplete - the collection of samples has been truncated!"
					  << std::endl;
			iptr--; // calling method expects us to point to last word in block
			break;
		}

		// Make sure this is a data continuation word, if not, stop here
		if (((*iptr >> 31) & 0x1) != 0x0) {
			iptr--; // calling method expects us to point to last word in block
			break;
		}

		bool invalid_1 = (*iptr >> 29) & 0x1;
		bool invalid_2 = (*iptr >> 13) & 0x1;
		uint16_t sample_1 = 0;
		uint16_t sample_2 = 0;
		if (!invalid_1) {
			sample_1 = (*iptr >> 16) & 0x1FFF;
		}
		if (!invalid_2) {
			sample_2 = (*iptr >> 0) & 0x1FFF;
		}

		// Sample 1
		wrd.samples.push_back(sample_1);
		wrd.invalid_samples |= invalid_1;
		wrd.overflow |= (sample_1 >> 12) & 0x1;

		if (((isample + 2) == window_width) && invalid_2) {
			break; // skip last sample if flagged as invalid
		}
		// Sample 2
		wrd.samples.push_back(sample_2);
		wrd.invalid_samples |= invalid_2;
		wrd.overflow |= (sample_2 >> 12) & 0x1;
	}

	if (window_width != wrd.samples.size()) {
		std::cerr << " FADC250 Window Raw Data number of samples does not match header! (" << wrd.samples.size()
				  << " != " << window_width << ") for rocid=" << rocid << " slot=" << slot << " channel=" << channel
				  << std::endl;
	}

	event.f250_window_raw_data.push_back(std::move(wrd));
}

void EvioBlockedEventParser::ParseSSPBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseTSscalerBank(uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseDGEMSRSBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::Parsef125Bank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) {
	iptr = &iptr[(*iptr) + 1];
}

void EvioBlockedEventParser::Parsef250scalerBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseCAEN1190(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseF1TDCBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

void EvioBlockedEventParser::ParseTIBank(uint32_t rocid, uint32_t *&iptr, uint32_t *iend) { iptr = iend; }

} // namespace halld::evio
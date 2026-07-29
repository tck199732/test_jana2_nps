// Flat data containers filled by the SRO frame-set parser.
//
// Design constraint: one parsed evio block produces a handful of std::vector
// allocations, never per-hit heap objects. (The predecessor HDEVIO chain spent
// its time deleting thousands of small objects per event; keep it flat.)

#pragma once

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace evio::sro {

/// One FADC250 hit word, unpacked. Raw rocid/slot/channel are always valid;
/// translated fields are -1 when the cabling tables do not cover the channel.
struct FadcHit {
	uint32_t frame_index; ///< index into SroBlockData::frames
	uint16_t rocid;
	uint8_t slot;		 ///< physical VME slot (payload2slot applied)
	uint8_t channel;	 ///< 0-15
	uint16_t charge;	 ///< raw ADC integral, 13 bits, no pedestal/gain calibration
	uint16_t time_ticks; ///< within-frame time, 4 ns ticks, 14 bits

	// not implemented yet
	// std::vector<uint16_t> samples;
	// std::vector<uint16_t> timestamps;
};

/// One time frame (= one aggregated frame set on disk).
/// Hit ranges are contiguous slices of the hit vectors in the same SroBlockData,
/// so a frame's hits are [first, first+count) - no per-frame containers.
/// Under lazy parsing the hit vectors hold only ECAL hits and the deferred range
/// points at the frame's undecoded ROC banks (see ParseBlockBodyLazy).
struct FrameInfo {
	uint32_t frame_number;
	uint64_t timestamp; ///< ticks (~1 ns; frame length is 65536 ticks in sro_000791)
	uint32_t first_fadc_hit;
	uint32_t fadc_hit_count;
	uint32_t first_deferred_roc = 0;
	uint32_t deferred_roc_count = 0;
};

/// One not-yet-decoded ROC time-slice bank: a word range inside
/// SroBlockData::body_words, decodable later with DecodeDeferredFrame.
struct DeferredRocBank {
	uint32_t bank_pos;	 ///< word index of the ROC bank header
	uint32_t bank_limit; ///< walk limit (enclosing set bank content end)
};

/// Parse anomalies, counted instead of aborting (dac called exit() here).
/// A nonzero count is data to look at, not necessarily an error.
struct ParseStats {
	uint64_t fake_frames = 0;		   ///< streams with empty payload map (aggregator gap fill)
	uint64_t empty_payload_banks = 0;  ///< zero-length duplicate payload banks (num=1)
	uint64_t unknown_module_words = 0; ///< payload data words skipped: module_id not 0/1
	uint64_t unknown_rocid_hits = 0;   ///< FADC hits stored with Detector::Unknown
	uint64_t untranslated_hits = 0;	   ///< known detector but table gave -1 for the channel
	uint64_t structure_errors = 0;	   ///< bank lengths inconsistent; rest of set skipped

	void Add(const ParseStats &other) {
		fake_frames += other.fake_frames;
		empty_payload_banks += other.empty_payload_banks;
		unknown_module_words += other.unknown_module_words;
		unknown_rocid_hits += other.unknown_rocid_hits;
		untranslated_hits += other.untranslated_hits;
		structure_errors += other.structure_errors;
	}

	std::string ToString() const {
		std::ostringstream text;
		text << "fake_frames=" << fake_frames << " empty_payload_banks=" << empty_payload_banks
			 << " unknown_module_words=" << unknown_module_words << " unknown_rocid_hits=" << unknown_rocid_hits
			 << " untranslated_hits=" << untranslated_hits << " structure_errors=" << structure_errors;
		return text.str();
	}
};

/// Everything parsed from one evio block (11 frame sets in sro_000791).
/// lazy=true: fadc_hits holds ECAL hits only, dcrb_hits is empty, body_words
/// owns the raw block body so deferred_rocs ranges stay decodable.
struct SroBlockData {
	uint32_t block_number = 0;
	uint32_t event_count = 0; ///< expected frame sets (block header); parse input
	uint32_t body_word_count = 0;
	bool lazy = false;
	bool parse_pending = false; ///< raw body attached, parse deferred to the unfolder's Preprocess
	std::vector<FrameInfo> frames;
	std::vector<FadcHit> fadc_hits;
	std::vector<uint32_t> body_words;		 ///< owned raw body (fread reader)
	const uint32_t *external_body = nullptr; ///< borrowed raw body (mmap reader)
	std::shared_ptr<const void> body_owner;	 ///< pins the file mapping while this block lives

	const uint32_t *Body() const { return external_body != nullptr ? external_body : body_words.data(); }
	std::vector<DeferredRocBank> deferred_rocs;
	ParseStats stats;
};

} // namespace evio::sro

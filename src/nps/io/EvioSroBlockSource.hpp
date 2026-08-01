#pragma once

#include <memory>
#include <string>

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "evio/sro/SroBlockReader.hpp"
#include "evio/sro/SroFrameSetParser.hpp"

namespace nps::io {

class EvioSroBlockSource : public JEventSource {

private:
	std::unique_ptr<evio::sro::SroBlockReader> m_reader;
	evio::sro::RawBlock m_raw_block;   // reused buffer, contents overwritten per Emit
	evio::sro::ParseStats m_run_stats; // accumulated over all blocks, printed at Close
	uint64_t m_blocks_read = 0;

	Parameter<bool> m_parse_enabled{
		this, "evio6_sro_block_source:parse", true,
		"false = read blocks but skip parsing (pure I/O measurement; no frames reach the output)"
	};
	Parameter<bool> m_lazy_parse{
		this, "evio6_sro_block_source:lazy_parse", false,
		"true = decode only ECAL ROC banks up front; the unfolder decodes the rest per selected frame (false = eager "
		"full decode)"
	};
	Parameter<std::string> m_reader_mode{
		this, "evio6_sro_block_source:reader", "fread", "'fread' (copying baseline) or 'mmap' (zero-copy mapped input)"
	};

public:
	EvioSroBlockSource();
	virtual ~EvioSroBlockSource() = default;

	void Init() override;
	void Open() override;
	void Close() override;
	Result Emit(JEvent &event) override;

	static std::string GetDescription() { return "SRO evio block source (timeslice level, naive reader)"; }
};

} // namespace nps::io

#include "EvioSroBlockSource.hpp"

namespace nps::io {

EvioSroBlockSource::EvioSroBlockSource() : JEventSource() {
	SetTypeName(NAME_OF_THIS);
	SetLevel(JEventLevel::Timeslice);
	SetCallbackStyle(CallbackStyle::ExpertMode);
}

void EvioSroBlockSource::Init() {
	// Nothing to do here for now
}

void EvioSroBlockSource::Open() {
	m_reader = std::make_unique<evio::sro::SroBlockReader>(
		std::vector<std::string>{GetResourceName()}, m_reader_mode() == "mmap"
	);
}

void EvioSroBlockSource::Close() { m_reader.reset(); }

JEventSource::Result EvioSroBlockSource::Emit(JEvent &event) {
	if (!m_reader->ReadNextBlock(m_raw_block)) {
		return Result::FailureFinished;
	}
	m_blocks_read++;

	// The block is parsed right here, in the source, on the source thread -
	// the simplest correct topology. (Optimization phases may move parsing into
	// the unfolder's Preprocess to parallelize it; measure against this first.)
	auto *block_data = new evio::sro::SroBlockData();
	block_data->block_number = m_raw_block.block_number;
	if (m_parse_enabled() && m_lazy_parse()) {
		// Lazy mode: only the read happens on the source thread. The block
		// carries its raw body - zero-copy from the mapping when mmap-read,
		// otherwise by taking the fread buffer - and SroFrameUnfolder::
		// Preprocess parses it on the parallel map arrow.
		if (m_raw_block.mapping != nullptr) {
			block_data->external_body = m_raw_block.body;
			block_data->body_owner = m_raw_block.mapping;
		} else {
			block_data->body_words = std::move(m_raw_block.words);
		}
		block_data->body_word_count = static_cast<uint32_t>(m_raw_block.body_word_count);
		block_data->event_count = m_raw_block.event_count;
		block_data->parse_pending = true;
	} else if (m_parse_enabled()) {
		evio::sro::ParseBlockBody(m_raw_block.body, m_raw_block.body_word_count, m_raw_block.event_count, *block_data);
		m_run_stats.Add(block_data->stats);
	}

	event.SetEventNumber(m_raw_block.block_number);
	event.Insert(block_data, "SroBlockData");
	return Result::Success;
}

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::EvioSroBlockSource>::CheckOpenable(std::string resource_name) {

	/// CheckOpenable() decides how confident we are that this EventSource can handle this resource.
	///    0.0        -> 'Cannot handle'
	///    (0.0, 1.0] -> 'Can handle, with this confidence level'

	/// To determine confidence level, feel free to open up the file and check for magic bytes or metadata.
	/// Returning a confidence <- {0.0, 1.0} is perfectly OK!

	if (resource_name.size() >= 5 && resource_name.substr(resource_name.size() - 5) == ".evio") {
		return 1.0;
	} else if (resource_name.find(".evio.") != std::string::npos) {
		// Higher confidence than CDAQfile's 0.5: SRO streaming files carry ".evio."
		// with a numeric split suffix (sro_000791.evio.00000).
		return 0.5;
	}
	return 0.0;
}

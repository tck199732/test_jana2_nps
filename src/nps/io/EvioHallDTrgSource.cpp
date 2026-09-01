#include "EvioHallDTrgSource.hpp"

namespace nps::io {
EvioHallDTrgSource::EvioHallDTrgSource() {
	SetTypeName(NAME_OF_THIS); // Provide JANA with class name
	SetCallbackStyle(CallbackStyle::ExpertMode);
}

EvioHallDTrgSource::~EvioHallDTrgSource() {
	if (m_hdevio) {
		m_hdevio->PrintStats();
		m_hdevio.reset();
	}
}

void EvioHallDTrgSource::Init() {
	m_hdevio = std::make_unique<HDEVIO>(GetResourceName(), true, 2); // 2 for VERBOSE level
	if (!m_hdevio->is_open) {
		std::string err = m_hdevio->err_mess.str();
		throw std::runtime_error("Failed to open EVIO file " + GetResourceName() + ": " + err);
	}
}

JEventSource::Result EvioHallDTrgSource::Emit(JEvent &event) {
	std::vector<uint32_t> block_data;
	block_data.resize(m_buff_len);

	bool read_ok = m_hdevio->readNoFileBuff(block_data.data(), block_data.capacity());
	uint32_t cur_len = m_hdevio->last_event_len;

	// Handle not read_ok
	if (not read_ok) {
		if (m_hdevio->err_code == HDEVIO::HDEVIO_EOF) {
			throw RETURN_STATUS::kNO_MORE_EVENTS;

		} else if (m_hdevio->err_code == HDEVIO::HDEVIO_USER_BUFFER_TOO_SMALL) {
			m_buff_len = cur_len;
			block_data.clear();
			throw RETURN_STATUS::kTRY_AGAIN;
		} else {
			throw JException("Unhandled EVIO file reading return status " + m_hdevio->err_code, __FILE__, __LINE__);
		}
	}

	block_data.resize(cur_len);

	auto parser = new halld::evio::EvioBlockedEventParser();
	parser->ParseBlock(block_data);
	auto parsed_events = parser->GetParsedEvents();

	for (auto parsed_event : parsed_events) {
		auto ev = new halld::evio::ParsedEvent(std::move(parsed_event));
		event.Insert(ev, "evio_hd_trg_data");
	}
	return Result::Success;
}

void EvioHallDTrgSource::Open() {}
void EvioHallDTrgSource::Close() {
	if (m_hdevio) {
		m_hdevio.reset();
	}
}

std::string EvioHallDTrgSource::GetDescription() {
	return "EvioHallDTrgSource - Read an *.evio file and parse the blocks into JEvents.";
}

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::EvioHallDTrgSource>::CheckOpenable(std::string resource_name) {

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

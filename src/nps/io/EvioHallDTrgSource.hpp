#pragma once

#include <memory>
#include <string>

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

#include <evio/halld/HDEVIO.h>
#include <evio/halld_modules/parser.hpp>

namespace nps::io {

constexpr uint32_t DEFAULT_READ_BUFF_LEN = 4000000;

class EvioHallDTrgSource : public JEventSource {

private:
	std::unique_ptr<HDEVIO> m_hdevio;

	uint32_t m_buff_len = DEFAULT_READ_BUFF_LEN;
	int m_block_number = 1;

public:
	EvioHallDTrgSource();

	virtual ~EvioHallDTrgSource();

	void Init() override;
	void Open() override;
	void Close() override;
	Result Emit(JEvent &event) override;

	static std::string GetDescription();
};
} // namespace nps::io

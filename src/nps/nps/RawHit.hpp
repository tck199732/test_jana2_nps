#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nps {

class RawHit {
public:
	RawHit() = default;
	~RawHit() = default;

	RawHit(int channel, std::vector<double> waveform) : m_channel(channel), m_waveform(std::move(waveform)) {}
	// copy constructor
	RawHit(const RawHit &other) {
		m_channel = other.m_channel;
		m_waveform = other.m_waveform;
	}
	// move constructor
	RawHit(RawHit &&other) noexcept : m_channel(other.m_channel), m_waveform(std::move(other.m_waveform)) {
		other.m_channel = -1; // Invalidate the moved-from object
	}

	int getChannel() const { return m_channel; }
	const std::vector<double> &getWaveform() const { return m_waveform; }

	void setChannel(int channel) { m_channel = channel; }
	void setWaveform(const std::vector<double> &waveform) { m_waveform = waveform; }
	void setWaveform(std::vector<double> &&waveform) { m_waveform = std::move(waveform); }

private:
	int m_channel{-1};				// channel no. / block ID (0-1079)
	std::vector<double> m_waveform; // ADC samples of the waveform
};

} // namespace nps
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nps {

class fAdcHit {
public:
	fAdcHit() = default;
	~fAdcHit() = default;

	// copy constructor
	fAdcHit(const fAdcHit &other) {
		m_channel = other.m_channel;
		m_energy = other.m_energy;
		m_time = other.m_time;
	}

	// move constructor
	fAdcHit(fAdcHit &&other) noexcept : m_channel(other.m_channel), m_energy(other.m_energy), m_time(other.m_time) {
		other.m_channel = -1; // Invalidate the moved-from object
		other.m_energy = 0.0;
		other.m_time = 0.0;
	}

	fAdcHit(int ch, double energy, double time) : m_channel(ch), m_energy(energy), m_time(time) {}

	double getEnergy() const { return m_energy; }
	double getTime() const { return m_time; }
	int getChannel() const { return m_channel; }

	void setEnergy(double energy) { m_energy = energy; }
	void setTime(double time) { m_time = time; }
	void setChannel(int ch) { m_channel = ch; }

private:
	int m_channel{-1};
	double m_energy{0.0};
	double m_time{0.0};
};

} // namespace nps
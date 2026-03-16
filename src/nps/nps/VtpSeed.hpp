#pragma once

#include <cstddef>
#include <vector>

namespace nps {

class VtpSeed {
public:
	VtpSeed() = default;
	~VtpSeed() = default;
	VtpSeed(int channel, int size, double time, double energy) :
		m_channel(channel),
		m_size(size),
		m_time(time),
		m_energy(energy) {}

	// copy constructor
	VtpSeed(const VtpSeed &other) {
		m_channel = other.m_channel;
		m_size = other.m_size;
		m_time = other.m_time;
		m_energy = other.m_energy;
	}

	// move constructor
	VtpSeed(VtpSeed &&other) noexcept :
		m_channel(other.m_channel),
		m_size(other.m_size),
		m_time(other.m_time),
		m_energy(other.m_energy) {
		other.m_channel = -1; // Invalidate the moved-from object
		other.m_size = -1;
		other.m_time = -1.0;
		other.m_energy = -1.0;
	}

	int getChannel() const { return m_channel; }
	int getSize() const { return m_size; }
	double getTime() const { return m_time; }
	double getEnergy() const { return m_energy; }

	void setChannel(int channel) { m_channel = channel; }
	void setSize(int size) { m_size = size; }
	void setTime(double time) { m_time = time; }
	void setEnergy(double energy) { m_energy = energy; }

private:
	int m_channel{-1};	   // block ID (0-1079)
	int m_size{-1};		   // number of hits in the cluster within the 3x3 window
	double m_time{-1.0};   // rise time of the seed pulse in ns
	double m_energy{-1.0}; // total energy of the cluster in GeV
};

} // namespace nps
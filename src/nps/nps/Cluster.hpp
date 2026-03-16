#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nps {

class Cluster {
public:
	Cluster() = default;
	~Cluster() = default;

	// copy constructor
	Cluster(const Cluster &other) {
		m_index = other.m_index;
		m_channels = other.m_channels;
		m_energies = other.m_energies;
		m_times = other.m_times;
	}
	// move constructor
	Cluster(Cluster &&other) noexcept :
		m_index(other.m_index),
		m_channels(std::move(other.m_channels)),
		m_energies(std::move(other.m_energies)),
		m_times(std::move(other.m_times)) {
		other.m_index = -1; // Invalidate the moved-from object
	}

	void addHit(int channel, double energy, int time) {
		m_channels.push_back(channel);
		m_energies.push_back(energy);
		m_times.push_back(time);
	}

	const std::vector<int> &getChannels() const { return m_channels; }
	const std::vector<double> &getEnergies() const { return m_energies; }
	const std::vector<int> &getTimes() const { return m_times; }

private:
	int m_index{-1};				// cluster index in the event
	std::vector<int> m_channels;	// block IDs of hits in the cluster
	std::vector<double> m_energies; // energies of hits in the cluster
	std::vector<int> m_times;		// times of hits in the cluster
};

} // namespace nps
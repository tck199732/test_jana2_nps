#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nps {

class Cluster {
public:
	Cluster() = default;
	Cluster(int event_index, int cluster_index) : m_event_index(event_index), m_cluster_index(cluster_index) {}
	~Cluster() = default;

	// copy constructor
	Cluster(const Cluster &other) {
		m_event_index = other.m_event_index;
		m_cluster_index = other.m_cluster_index;
		m_channels = other.m_channels;
		m_energies = other.m_energies;
		m_times = other.m_times;
	}
	// move constructor
	Cluster(Cluster &&other) noexcept :
		m_event_index(other.m_event_index),
		m_cluster_index(other.m_cluster_index),
		m_channels(std::move(other.m_channels)),
		m_energies(std::move(other.m_energies)),
		m_times(std::move(other.m_times)) {
		other.m_event_index = -1;	// Invalidate the moved-from object
		other.m_cluster_index = -1; // Invalidate the moved-from object
	}

	void setEventIndex(int index) { m_event_index = index; }
	void setClusterIndex(int index) { m_cluster_index = index; }
	void addHit(int channel, double energy, int time) {
		m_channels.push_back(channel);
		m_energies.push_back(energy);
		m_times.push_back(time);
	}

	int getEventIndex() const { return m_event_index; }
	int getClusterIndex() const { return m_cluster_index; }
	const std::vector<int> &getChannels() const { return m_channels; }
	const std::vector<double> &getEnergies() const { return m_energies; }
	const std::vector<int> &getTimes() const { return m_times; }

private:
	int m_event_index{-1};			// event index of the cluster
	int m_cluster_index{-1};		// cluster index in the event
	std::vector<int> m_channels;	// block IDs of hits in the cluster
	std::vector<double> m_energies; // energies of hits in the cluster
	std::vector<int> m_times;		// times of hits in the cluster
};

} // namespace nps
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nps::clustering {

class ObjectCondensationOutput {
public:
	ObjectCondensationOutput() = default;
	ObjectCondensationOutput(const std::vector<double> &x_c, double beta) : m_x_c(x_c), m_beta(beta) {}
	~ObjectCondensationOutput() = default;

	// copy constructor
	ObjectCondensationOutput(const ObjectCondensationOutput &other) {
		m_event_index = other.m_event_index;
		m_channel = other.m_channel;
		m_beta = other.m_beta;
		m_x_c = other.m_x_c;
	}
	// move constructor
	ObjectCondensationOutput(ObjectCondensationOutput &&other) noexcept :
		m_event_index(other.m_event_index),
		m_channel(other.m_channel),
		m_beta(other.m_beta),
		m_x_c(std::move(other.m_x_c)) {}

	void setEventIndex(int index) { m_event_index = index; }
	void setChannel(int channel) { m_channel = channel; }
	void setBeta(double beta) { m_beta = beta; }
	void setXc(const std::vector<double> &x_c) { m_x_c = x_c; }

	int getEventIndex() const { return m_event_index; }
	int getChannel() const { return m_channel; }
	double getBeta() const { return m_beta; }
	const std::vector<double> &getXc() const { return m_x_c; }

private:
	int m_event_index{-1};	   // event index of the cluster
	int m_channel{-1};		   // channel of the cluster
	double m_beta{0.0};		   // beta value of the cluster
	std::vector<double> m_x_c; // condensation coordinate of the cluster
};

} // namespace nps::clustering
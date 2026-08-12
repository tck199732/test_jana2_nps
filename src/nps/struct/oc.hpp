#pragma once

#include <cstdint>
#include <vector>

namespace nps {

struct oc_head {
	uint64_t event_index;	 // event index
	int channel;			 // channel index
	double beta;			 // cluster seedness parameter
	std::vector<double> x_c; // condensation coordinate of the cluster
};

} // namespace nps

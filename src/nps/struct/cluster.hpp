#pragma once

#include <vector>

namespace nps {

struct cluster {
	std::vector<int> channels;	  // block IDs of hits in the cluster
	std::vector<int> hit_indices; // indices of hits in the cluster
	std::vector<double> energies; // energies of hits in the cluster
	std::vector<int> times;		  // times of hits in the cluster
};

} // namespace nps
#pragma once

#include <vector>

namespace nps {

enum class cluster_type {
	NOT_TRIGGERED = 0,
	TRIGGERED = 1,
	UNKNOWN = -1,
};

/**
 * @brief Represents a cluster of hits in the detector.
 * In Vtp trigger mode, a cluster seed is defined as the center hit of a 3x3 window of hits which satisfies the
 * following conditions:
 * 1. The seed hit must have the highest energy among the hits in the 3x3 window.
 * 2. The seed hit must have an energy above a certain threshold defined in the configuration.
 * 3. The cluster must have a minimum number of hits defined in the configuration.
 * 4. The hits are grouped based on their spatial and temporal proximity, with the time window defined in the
 * configuration. The neighboring hits in the 3x3 window are not recorded in NPS Phase 1 but are reconstructed in
 * VtpClusterFactory. However, a trigger is generated only if the total energy of the cluster exceeds a certain
 * threshold. This struct represents clusters regardless of whether they are triggered or not, and the is_triggered flag
 * indicates whether the cluster meets the trigger conditions.
 */
struct cluster {
	int id;						  // unique identifier for the cluster
	cluster_type type;			  // whether the cluster is triggered
	std::vector<int> channels;	  // block IDs of hits in the cluster
	std::vector<int> hit_indices; // indices of hits in the cluster
	std::vector<double> energies; // energies of hits in the cluster
	std::vector<int> times;		  // times of hits in the cluster
};

} // namespace nps
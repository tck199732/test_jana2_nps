#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace nps {

struct vtp_seed {
	int channel;   // block ID (0-1079)
	int size;	   // number of hits in the cluster within the 3x3 window
	double time;   // rise time of the seed pulse in ns
	double energy; // total energy of the cluster in GeV
};

struct vtp_cfg {
	int firmware_version;
	int firmware_type;
	int width;
	int offset;
	int payload_en;
	int fiber_en;

	// nps configuration
	int nps_trig_width;
	int nps_trig_latency;
	std::array<uint16_t, 32> nps_trig_prescale; // [trgbit] -> [0..65535] prescale value
	std::array<uint16_t, 32> nps_trig_delay;	// [trgbit] -> [0..1020] delay value in ns

	int ecalcluster_seed_thr;
	int ecalcluster_hit_dt;
	int ecalcluster_cluster_trigger_thr;
	int ecalcluster_cluster_readout_thr;
	int ecalcluster_cluster_pair_trigger_thr;
	int ecalcluster_cluster_pair_trigger_width;
	int ecalcluster_crate_id;
	int ecalcluster_nhit_min;
	int ecalcluster_fadcmask_offset;
	int ecalcluster_fadcmask_width;
	int ecalcluster_fadcmask_prescale;
	int ecalcluster_fadcmask_mode;
	int ecalcluster_cosmic_scint_dt;
	int ecalcluster_cosmic_column_veto_en;
	int ecalcluster_cosmic_column_dt;
	int ecalcluster_cosmic_column_multmin;
};

} // namespace nps
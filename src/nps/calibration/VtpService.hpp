#pragma once

#include <JANA/JService.h>
#include <JANA/Services/JServiceLocator.h>
#include <memory>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace nps::calib {

typedef struct {
	std::unordered_map<int, int> firmware_type;
	std::unordered_map<int, int> firmware_ver;
	std::unordered_map<int, int> offset;
	std::unordered_map<int, int> width;

	std::unordered_map<int, int> cluster_hit_dt;
	std::unordered_map<int, int> cluster_seed_thr;
	std::unordered_map<int, int> cluster_nhits_min;
	std::unordered_map<int, int> cluster_readout_thr;
	std::unordered_map<int, int> cluster_trigger_thr;
	std::unordered_map<int, int> cluster_pair_trigger_thr;
	std::unordered_map<int, int> cluster_pair_trigger_width;
	std::unordered_map<int, int> fadcmask_mode;

} vtp_cfg;

class VtpService : public JService {
public:
	explicit VtpService() : JService() {}
	~VtpService() override = default;

	void Init() override;
	bool Load(const std::string &filename);
	void Reset();

	const vtp_cfg &getConfig() const { return m_config; }

private:
	vtp_cfg m_config;

	Parameter<std::string> m_config_file{
		this, "calib:vtp_config_file", "config_vtp.txt", "Path to VTP configuration file"
	};

protected:
	// default configuration according to "https://hallcweb.jlab.org/wiki/images/b/b3/NPS_VTP_DAQ.pdf"

	double m_default_fadc_offset = 4500; // FADC Lookback time from trigger in ns
	double m_default_fadc_width = 440;	 // Waveform readout window in ns
	double m_default_offset = 4448;		 // VTP Lookback time time from trigger in ns
	double m_default_width = 1000;		 // Window width to find clusters in ns

	double m_default_seed_threshold = 50.0;			 // Threshold for defining a cluster seed in MeV
	double m_default_hit_timing_window = 20.0;		 // Coincidence window for cluster formation in ns
	int m_default_min_hits = 1;						 // Mininum # Hits to define cluster
	double m_default_cluster_threshold = 900.0;		 // Single-cluster validation threshold (Bit 0) MeV
	double m_default_pair_cluster_threshold = 500.0; // Two-cluster validation threshold MeV
	double m_default_pair_cluster_width = 20.0;		 //  Output width of VTP Bit 4 ns

	// for Sparisfication
	int m_default_readout_mode = 7;				// 0 for 5x5 or 1 for 7x7
	double m_default_readout_threshold = 100.0; //  Cluster threshold for readout in MeV
};

} // namespace nps::calib

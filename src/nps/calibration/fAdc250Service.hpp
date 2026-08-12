#pragma once

#include <JANA/JService.h>
#include <JANA/Services/JServiceLocator.h>
#include <memory>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace nps::calib {

struct fadc250_cfg {

	int clock_cycles;	  // number of clock cycles to wait after a pulse is detected before looking for another pulse
	double time_interval; // time interval of each FADC sample in ns
	std::unordered_map<int, double> thr;  // FADC250_TET per channel
	std::unordered_map<int, double> gain; // FADC250_GAIN per channel
	std::unordered_map<int, double> ped;  // FADC250_ALLCH_PED per channel
	std::unordered_map<int, int> nsa;	  // FADC250_NSA per channel
	std::unordered_map<int, int> nsb;	  // FADC250_NSB per channel
};

class fAdc250Service : public JService {

public:
	explicit fAdc250Service() : JService() {}
	~fAdc250Service() override = default;

	void Init() override;
	bool Load(const std::string &filename);
	void Reset();

	const fadc250_cfg &getConfig() const { return m_config; }

private:
	Parameter<std::string> m_config_file{
		this, "calib:fadc_config_file", "config_fadc250.txt", "Path to VME configuration file for fADC250 device"
	};

	Parameter<double> m_time_interval{this, "calib:fadc_time_interval", 4.0, "Time interval of each FADC sample in ns"};

	Parameter<int> m_clock_cycles{
		this, "calib:fadc_clock_cycles", 7,
		"Number of clock cycles to wait after a pulse is detected before looking for another pulse"
	};

	fadc250_cfg m_config;

protected:
	// Below are fixed values used in THcRawAdcHit class in hcana
	int m_max_pulses = 4; // number of maximum pulses to search for in a waveform, by default 4
	int m_nsat = 2;		  // number of samples above threshold after crossing so that pulse is considered to be valid

	// default configuration according to "https://hallcweb.jlab.org/wiki/images/b/b3/NPS_VTP_DAQ.pdf"
	double m_default_channel_threshold = 10.0;
	double m_default_channel_gain = 1.0;
	double m_default_channel_pedestal = 0.0;
	int m_default_channel_nsa = 4; // n samples before the pulse crossing threshold
	int m_default_channel_nsb = 9; // n samples after the pulse crossing threshold
};

} // namespace nps::calib

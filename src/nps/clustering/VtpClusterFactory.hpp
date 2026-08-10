#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "geometry/NpsGeometryService.hpp"

#include "struct/cluster.hpp"
#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

#include <cassert>
#include <unordered_set>
#include <vector>

namespace nps::clustering {

class VtpClusterFactory : public JOmniFactory<VtpClusterFactory> {
public:
	Input<nps::fadc_waveform> m_fadc_waveforms{this, {"fadc_waveforms", JEventLevel::None, false}};
	Input<nps::fadc_hit> m_input_fadc_hits{this, {"fadc_hits", JEventLevel::None, true}};
	Input<nps::vtp_seed> m_vtp_seeds{this, {"vtp_seeds", JEventLevel::None, true}};

	Output<nps::cluster> m_clusters{this, "vtp_clusters"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};
	Service<nps::calib::VtpService> m_service_vtp{this};
	Service<nps::calib::fAdc250Service> m_service_fadc{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);

private:
	// for processing fAdc signals
	void processRawWaveform(const nps::fadc_waveform *waveform, std::vector<nps::fadc_hit> &hits);
	std::vector<int> findPulses(const std::vector<double> &waveform_adc, double thr, int clk) const;

	// process vtp clusterization
	std::vector<nps::cluster> selectGridCandidate(const std::vector<nps::fadc_hit> &hits);
	bool isSeed(const nps::cluster &clus);
	bool isTriggered(const nps::cluster &clus);
	bool isMatched(const nps::cluster &clus, const nps::vtp_seed &seed, double de_thr, double tmin, double tmax);

	Parameter<bool> m_use_waveform{
		this, "use_waveform", true, "Whether to use waveform data to reconstruct hit or use input hit data directly."
	};

	Parameter<int> m_grid_size{
		this, "grid_size", 3,
		"Size of the grid for cluster formation (3x3 or 5x5). If match_seed is set to true, make sure to set grid_size "
		"according to the experimental configuration (3x3 in nps phase 1)."
	};

	Parameter<bool> m_match_seed{this, "match_seed", true, "Whether to match clusters to VTP seeds."};
	Parameter<double> m_de_thr{
		this, "de_thr", 5.0, "Energy difference threshold for cluster matching, Only useful if match_seed is true."
	};
	Parameter<double> m_tmin{
		this, "tmin", 50.0, "Minimum time for cluster matching, Only useful if match_seed is true."
	};
	Parameter<double> m_tmax{
		this, "tmax", 370.0, "Maximum time for cluster matching, Only useful if match_seed is true."
	};
};
} // namespace nps::clustering
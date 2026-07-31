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
	Input<nps::fadc_waveform> m_fadc_waveforms{this, {"fadc_waveforms"}};
	Input<nps::vtp_seed> m_vtp_seeds{this, {"vtp_seeds"}};
	Output<nps::cluster> m_clusters{this, "vtp_clusters"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};
	Service<nps::calib::VtpService> m_service_vtp{this};
	Service<nps::calib::fAdc250Service> m_service_fadc{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);

private:
	// for processing fAdc signals
	void processRawWaveform(const nps::fadc_waveform &waveform, std::vector<nps::fadc_hit> &hits);
	std::vector<int> findPulses(const std::vector<double> &waveform_adc, double thr, int clk) const;

	// process vtp clusterization
	std::vector<nps::cluster> selectGridCandidate(const std::vector<nps::fadc_hit> &hits);
	bool isTriggered(const nps::cluster &clus);
	bool isMatched(const nps::cluster &clus, const nps::vtp_seed &seed, double de_thr, double tmin, double tmax);

	Parameter<double> m_de_thr{this, "clus:de_thr", 5.0, "Energy difference threshold for cluster matching"};
	Parameter<double> m_tmin{this, "clus:tmin", 50.0, "Minimum time for cluster matching"};
	Parameter<double> m_tmax{this, "clus:tmax", 370.0, "Maximum time for cluster matching"};
};
} // namespace nps::clustering
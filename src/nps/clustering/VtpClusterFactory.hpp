#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "geometry/NpsGeometryService.hpp"

#include "nps/Cluster.hpp"
#include "nps/RawHit.hpp"
#include "nps/VtpSeed.hpp"
#include "nps/fAdcHit.hpp"

#include <cassert>
#include <unordered_set>
#include <vector>

namespace nps::clustering {

class VtpClusterFactory : public JOmniFactory<VtpClusterFactory> {
public:
	Input<nps::RawHit> m_rawhits{this, {"RawHits"}};
	Input<nps::VtpSeed> m_vtpseeds{this, {"VtpSeeds"}};
	Output<nps::Cluster> m_clusters{this, "VtpClusters"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};
	Service<nps::calib::VtpService> m_service_vtp{this};
	Service<nps::calib::fAdc250Service> m_service_fadc{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);

private:
	// for processing fAdc signals
	void processRawWaveform(const std::vector<double> &waveform, int channel, std::vector<fAdcHit> &hits);
	std::vector<int> findPulses(const std::vector<double> &waveform_adc, double thr, int clk) const;

	// process vtp clusterization
	std::vector<Cluster> selectGridCandidate(const std::vector<fAdcHit> &fadc_hits);
	bool isTriggered(const Cluster &clus);
	bool isMatched(const Cluster &clus, const VtpSeed &seed, double de_thr, double tmin, double tmax);

	Parameter<double> m_de_thr{this, "clus:de_thr", 5.0, "Energy difference threshold for cluster matching"};
	Parameter<double> m_tmin{this, "clus:tmin", 50.0, "Minimum time for cluster matching"};
	Parameter<double> m_tmax{this, "clus:tmax", 370.0, "Maximum time for cluster matching"};
};
} // namespace nps::clustering
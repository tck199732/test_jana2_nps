#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "BaseOnnxClusterFactory.hpp"
#include "calibration/fAdc250Service.hpp"
#include "geometry/NpsGeometryService.hpp"

#include "nps/Cluster.hpp"
#include "nps/fAdcHit.hpp"

#include <cassert>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace nps::clustering {

class HitClusterFactory : public BaseOnnxClusterFactory {
public:
	Input<nps::fAdcHit> m_fadchits{this, {"fAdcHits"}};
	Output<nps::Cluster> m_clusters{this, "HitClusters"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Describe() const;

private:
	int GetBatchSize() override;
	bool PrepareTensorValues() override;
};
} // namespace nps::clustering
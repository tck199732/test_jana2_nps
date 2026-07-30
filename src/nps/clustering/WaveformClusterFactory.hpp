#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "BaseOnnxClusterFactory.hpp"
#include "calibration/fAdc250Service.hpp"
#include "geometry/NpsGeometryService.hpp"
#include "onnx/OnnxRuntimeService.hpp"
#include "onnx/OnnxTensor.hpp"

#include "struct/cluster.hpp"
#include "struct/fadc.hpp"

#include <cassert>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace nps::clustering {

class WaveformClusterFactory : public BaseOnnxClusterFactory {
public:
	Input<nps::fadc_hit> m_rawhits{this, {"fadc_hits"}};
	Output<nps::cluster> m_clusters{this, "waveform_clusters"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

	WaveformClusterFactory() : BaseOnnxClusterFactory("waveform_clus") {}
	void Configure();
	void ChangeRun(int32_t run_number);
	void Describe() const;

private:
	int GetBatchSize() override;
	bool PrepareTensorValues() override;
};
} // namespace nps::clustering
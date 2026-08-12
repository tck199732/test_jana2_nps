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

#include "struct/fadc.hpp"
#include "struct/oc.hpp"

#include <cassert>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace nps::clustering {

class WaveformClusterFactory : public BaseOnnxClusterFactory {
public:
	Input<nps::fadc_waveform> m_fadc_waveforms{this, {"fadc_waveforms"}};
	Output<nps::oc_head> m_oc_heads{this, "oc_heads"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Describe() const;

private:
	std::deque<std::vector<nps::fadc_waveform>> m_queue;

	bool PrepareTensorValues() override;
	void DeepCopyInput() override;
	void UnpackOutput() override;
};
} // namespace nps::clustering
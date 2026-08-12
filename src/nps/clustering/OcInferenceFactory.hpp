#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "clustering/OcInferenceFactory.hpp"
#include "geometry/NpsGeometryService.hpp"
#include "onnx/OnnxRuntimeService.hpp"
#include "onnx/OnnxTensor.hpp"

#include "struct/cluster.hpp"
#include "struct/oc.hpp"

#include <cassert>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace nps::clustering {

class OcInferenceFactory : public JOmniFactory<OcInferenceFactory> {
public:
	Input<nps::oc_head> m_oc_heads{this, {"oc_heads"}};
	Output<nps::cluster> m_clusters{this, "clusters"};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);
	void Describe() const;

private:
	// per event inference
	void Inference(const std::vector<nps::oc_head> &oc_outputs);
	void InferenceOc(const std::vector<nps::oc_head> &oc_outputs);
	void InferenceDummy();

	Parameter<double> m_beta_thres{
		this, "beta_thres", 0.5, "Threshold for seedness parameter in cluster reconstruction."
	};

	Parameter<double> m_dist_thres{
		this, "dist_thres", 0.1, "Threshold for distance in latent space for cluster reconstruction."
	};

	Parameter<bool> m_use_dummy{
		this, "use_dummy", false,
		"Whether to use dummy inference that directly uses OC outputs as clusters. For debugging only."
	};
};
} // namespace nps::clustering
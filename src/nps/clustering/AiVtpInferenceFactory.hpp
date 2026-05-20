#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "clustering/ObjectCondensation.hpp"
#include "geometry/NpsGeometryService.hpp"
#include "onnx/OnnxRuntimeService.hpp"
#include "onnx/OnnxTensor.hpp"

#include "nps/Cluster.hpp"
#include "nps/RawHit.hpp"
#include "nps/VtpSeed.hpp"
#include "nps/fAdcHit.hpp"

#include <cassert>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace nps::clustering {

class AiVtpInferenceFactory : public JOmniFactory<AiVtpInferenceFactory> {
public:
	Input<nps::clustering::ObjectCondensationOutput> m_oc_outputs{this, {"ObjectCondensationOutputs"}};
	Output<nps::Cluster> m_clusters{this, "RecoClusters"};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);
	void Describe() const;

private:
	// per event inference
	void Inference(const std::vector<nps::clustering::ObjectCondensationOutput> &oc_outputs);
	void InferenceOc(const std::vector<nps::clustering::ObjectCondensationOutput> &oc_outputs);
	void InferenceDummy();

	Parameter<double> m_beta_thres{
		this, "clus:beta_thres", 0.5, "Threshold for seedness parameter in cluster reconstruction."
	};

	Parameter<double> m_dist_thres{
		this, "clus:dist_thres", 0.1, "Threshold for distance in latent space for cluster reconstruction."
	};

	Parameter<bool> m_use_dummy{
		this, "clus:use_dummy", false,
		"Whether to use dummy inference that directly uses OC outputs as clusters. For debugging only."
	};
};
} // namespace nps::clustering
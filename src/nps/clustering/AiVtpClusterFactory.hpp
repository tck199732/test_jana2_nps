#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "geometry/NpsGeometryService.hpp"
#include "onnx/OnnxRuntimeService.hpp"
#include "onnx/OnnxTensor.hpp"

#include "nps/Cluster.hpp"
#include "nps/RawHit.hpp"
#include "nps/VtpSeed.hpp"
#include "nps/fAdcHit.hpp"

#include <cassert>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace nps::clustering {

class AiVtpClusterFactory : public JOmniFactory<AiVtpClusterFactory> {
public:
	Input<nps::RawHit> m_rawhits{this, {"RawHits"}};
	Output<nps::Cluster> m_clusters{this, "RecoClusters"};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};
	Service<onnx::OnnxRuntimeService> m_service_onnx{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);
	void Describe() const;

private:
	std::vector<onnx::Tensor> m_input_tensors;
	std::vector<onnx::Tensor> m_output_tensors;

	bool PrepareTensors(Ort::Session &session);
	void PrepareTensorInfo(Ort::Session &session);
	void PrepareTensorValues();

	Parameter<std::string> m_model_path{
		this, "clus:model_path", "database/models/vtp_reco/my_model.onnx",
		"Path to the ONNX model for cluster reconstruction."
	};

	Parameter<std::string> m_session_name{
		this, "clus:session_name", "vtp_cluster_reco_session",
		"Name of the ONNX Runtime session to use for cluster reconstruction."
	};

	Parameter<int> m_num_threads{this, "clus:num_threads", 1, "Number of threads to use for ONNX Runtime inference."};

	Parameter<bool> m_use_cuda{this, "clus:use_cuda", false, "Whether to use CUDA for ONNX Runtime inference."};
};
} // namespace nps::clustering
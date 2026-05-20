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
#include <unordered_set>
#include <variant>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace nps::clustering {

class AiVtpClusterFactory : public JOmniFactory<AiVtpClusterFactory> {
public:
	Input<nps::RawHit> m_rawhits{this, {"RawHits"}};
	Output<nps::clustering::ObjectCondensationOutput> m_oc_outputs{this, {"ObjectCondensationOutputs"}};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};
	Service<onnx::OnnxRuntimeService> m_service_onnx{this};

	void Configure();
	void ChangeRun(int32_t run_number);
	void Execute(int32_t run_nr, uint64_t event_index);
	void Describe() const;

private:
	std::deque<std::vector<nps::RawHit>> m_rawhit_queue; // for accumulate events
	std::deque<uint64_t> m_event_index_queue;			 // for accumulate event indices

	std::vector<onnx::Tensor> m_input_tensors;
	std::vector<onnx::Tensor> m_output_tensors;

	void DeepCopyRawHits(const std::vector<const nps::RawHit *> &rawhits);
	void PopulateOutput();
	void PrepareTensors(Ort::Session &session);
	void PrepareTensorInfo(Ort::Session &session);
	void PrepareTensorValues();

	Parameter<std::string> m_model_path{
		this, "clus:model_path", "database/models/vtp_reco/my_model.onnx",
		"Path to the ONNX model for cluster reconstruction."
	};

	Parameter<std::string> m_session_name{
		this, "clus:ort_session_name", "vtp_cluster_reco_session",
		"Name of the ONNX Runtime session to use for cluster reconstruction."
	};

	Parameter<int> m_batch_size{
		this, "clus:batch_size", 1, "Number of events to process in a single batch for ONNX Runtime inference."
	};

	Parameter<bool> m_use_cuda{this, "clus:use_cuda", false, "Whether to use CUDA for ONNX Runtime inference."};
};
} // namespace nps::clustering
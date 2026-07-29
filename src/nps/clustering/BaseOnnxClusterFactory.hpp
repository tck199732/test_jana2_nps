#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "onnx/OnnxRuntimeService.hpp"
#include "onnx/OnnxTensor.hpp"

#include <cassert>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace nps::clustering {

class BaseOnnxClusterFactory : public JOmniFactory<BaseOnnxClusterFactory> {
public:
	virtual void Configure();
	virtual void ChangeRun(int32_t run_number);
	virtual void Execute(int32_t run_nr, uint64_t event_index);
	virtual void Describe() const;

	Service<onnx::OnnxRuntimeService> m_service_onnx{this};

private:
	bool PrepareTensors(Ort::Session &session);
	bool PrepareTensorInfo(Ort::Session &session);
	virtual int GetBatchSize() { return 1; }
	virtual bool PrepareTensorValues() = 0;

protected:
	std::vector<onnx::Tensor> m_input_tensors;
	std::vector<onnx::Tensor> m_output_tensors;

	Parameter<std::string> m_model_path{
		this, "clus:model_path", "my_model.onnx", "Path to the ONNX model for cluster reconstruction."
	};

	Parameter<std::string> m_session_name{
		this, "clus:session_name", "onnx_session", "Name of the ONNX Runtime session to use for cluster reconstruction."
	};

	Parameter<int> m_num_threads{this, "clus:num_threads", 1, "Number of threads to use for ONNX Runtime."};
	Parameter<bool> m_use_cuda{this, "clus:use_cuda", false, "Whether to use CUDA for ONNX Runtime."};
};

} // namespace nps::clustering
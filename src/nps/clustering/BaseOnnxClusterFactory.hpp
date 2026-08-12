#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include "onnx/OnnxRuntimeService.hpp"
#include "onnx/OnnxTensor.hpp"

#include <cassert>
#include <deque>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace nps::clustering {

/**
 * @brief Base class for factories that use ONNX models for clustering.
 *
 * This class provides a common interface and functionality for factories that perform clustering using ONNX models.
 * It handles the preparation of input and output tensor shape, execution of the ONNX model, and management of ONNX
 * Runtime sessions. Inheriting classes must implement the PrepareTensorValues() method to fill the input tensors which
 * satisfy the ONNX model's requirements. Inheriting classes can also override GetBatchSize() to specify the batch size
 * for the input tensors.
 */
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
	virtual bool
	PrepareTensorValues() = 0;		  // inherit to fill the input tensors which satisfy the ONNX model's requirements
	virtual void DeepCopyInput() = 0; // inherit to deep copy the input data into the input tensors
	virtual void
	UnpackOutput() = 0; // inherit to pack the output tensors into the desired output format (e.g., clusters)

protected:
	std::vector<onnx::Tensor> m_input_tensors;
	std::vector<onnx::Tensor> m_output_tensors;
	std::deque<uint64_t> m_event_index_queue;

	Parameter<std::string> m_model_path{this, "model_path", "my_model.onnx", "Path to the ONNX model."};
	Parameter<std::string> m_session_name{this, "session_name", "onnx_session", "Name of the ONNX Runtime session."};
	Parameter<int> m_num_threads{this, "num_threads", 1, "Number of threads to use for ONNX Runtime."};
	Parameter<bool> m_use_cuda{this, "use_cuda", false, "Whether to use CUDA for ONNX Runtime."};
	Parameter<int> m_batch_size{this, "batch_size", 1, "Batch size for input tensors."};
};

} // namespace nps::clustering
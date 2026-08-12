#include "BaseOnnxClusterFactory.hpp"

namespace nps::clustering {

void BaseOnnxClusterFactory::Configure() {}

void BaseOnnxClusterFactory::ChangeRun(int32_t run_number) {}

void BaseOnnxClusterFactory::Execute(int32_t run_nr, uint64_t event_index) {

	DeepCopyInput();
	m_event_index_queue.push_back(event_index);

	if (m_event_index_queue.size() < static_cast<size_t>(m_batch_size())) {
		return; // Wait until we have enough events to fill the batch
	}

	auto options = m_service_onnx().createSessionOptions(m_num_threads(), m_use_cuda());
	auto &session = m_service_onnx().createSession(m_session_name(), m_model_path(), options);
	if (!PrepareTensors(session)) {
		return;
	}

	std::vector<Ort::Value> input_tensors;
	std::vector<Ort::Value> output_tensors;
	std::vector<const char *> input_names;
	std::vector<const char *> output_names;

	input_tensors.reserve(m_input_tensors.size());
	output_tensors.reserve(m_output_tensors.size());
	input_names.reserve(m_input_tensors.size());
	output_names.reserve(m_output_tensors.size());

	for (auto &tensor : m_input_tensors) {
		input_tensors.push_back(tensor.GetOrtValue());
		input_names.push_back(tensor.name());
	}

	for (auto &tensor : m_output_tensors) {
		output_tensors.push_back(tensor.GetOrtValue());
		output_names.push_back(tensor.name());
	}

	try {
		session.Run(
			Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(), m_input_tensors.size(),
			output_names.data(), output_tensors.data(), m_output_tensors.size()
		);

	} catch (const Ort::Exception &e) {
		throw std::runtime_error("ONNX Runtime inference failed: " + std::string(e.what()));
	}

	UnpackOutput();
	m_event_index_queue.clear(); // Clear the queue after processing the batch
}

bool BaseOnnxClusterFactory::PrepareTensors(Ort::Session &session) {
	m_input_tensors.clear();
	m_output_tensors.clear();

	bool success_info = PrepareTensorInfo(session);
	bool success_values = PrepareTensorValues();
	return success_info && success_values;
}

void BaseOnnxClusterFactory::Describe() const {
	std::string description =
		"BaseOnnxClusterFactory: This factory uses an ONNX model to perform clustering on input data. It prepares "
		"input and output tensors based on the model's requirements and executes inference using the ONNX Runtime.";
}

bool BaseOnnxClusterFactory::PrepareTensorInfo(Ort::Session &session) {

	Ort::AllocatorWithDefaultOptions allocator;

	const size_t numInputNodes = session.GetInputCount();
	const size_t numOutputNodes = session.GetOutputCount();

	for (size_t i = 0; i < numInputNodes; ++i) {
		Ort::AllocatedStringPtr name = session.GetInputNameAllocated(i, allocator);
		Ort::TypeInfo type_info = session.GetInputTypeInfo(i);
		auto info = type_info.GetTensorTypeAndShapeInfo();
		auto shape = info.GetShape();
		auto type = info.GetElementType();

		if (shape.size() > 0 && shape[0] == -1) {
			shape.at(0) = 1;
		}
		if (shape.size() > 1 && shape[1] == -1) {
			shape.at(1) = m_batch_size();
		}

		for (const auto &dim : shape) {
			if (dim <= 0) {
				throw std::runtime_error("Invalid tensor shape dimension: " + std::to_string(dim));
			}
		}
		m_input_tensors.push_back(onnx::Tensor::allocate(name.get(), shape, type));
	}

	for (size_t i = 0; i < numOutputNodes; ++i) {
		Ort::AllocatedStringPtr name = session.GetOutputNameAllocated(i, allocator);

		Ort::TypeInfo type_info = session.GetOutputTypeInfo(i);
		auto info = type_info.GetTensorTypeAndShapeInfo();
		auto shape = info.GetShape();
		auto type = info.GetElementType();

		if (shape.size() > 0 && shape[0] == -1) {
			shape.at(0) = 1;
		}
		if (shape.size() > 1 && shape[1] == -1) {
			shape.at(1) = m_batch_size();
		}

		for (const auto &dim : shape) {
			if (dim <= 0) {
				throw std::runtime_error("Invalid tensor shape dimension: " + std::to_string(dim));
			}
		}
		m_output_tensors.push_back(onnx::Tensor::allocate(name.get(), shape, type));
	}
	return true;
}

} // namespace nps::clustering
#include "AiVtpClusterFactory.hpp"

namespace nps::clustering {

void AiVtpClusterFactory::Configure() {}

void AiVtpClusterFactory::ChangeRun(int32_t run_number) {}

void AiVtpClusterFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	if (m_rawhits().empty()) {
		return;
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
}

bool AiVtpClusterFactory::PrepareTensors(Ort::Session &session) {
	m_input_tensors.clear();
	m_output_tensors.clear();

	try {
		PrepareTensorInfo(session);
	} catch (const std::exception &e) {
		return false;
	}

	try {
		PrepareTensorValues();
	} catch (const std::exception &e) {
		return false;
	}
	return true;
}

void AiVtpClusterFactory::PrepareTensorValues() {
	const auto &hits = m_rawhits();
	const size_t n_hits = hits.size();

	if (n_hits == 0) {
		return;
	}

	// Fill x (waveforms)
	size_t offset = 0;
	for (size_t i = 0; i < n_hits; ++i) {
		const auto &waveform = hits[i]->getWaveform();
		m_input_tensors[0].fill_n(waveform.data(), offset, waveform.size());
		offset += waveform.size();
	}

	// Fill pos (col, row per hit)
	for (size_t i = 0; i < n_hits; ++i) {
		auto [col, row] = m_service_geometry().getColRowFromBlock(hits[i]->getChannel());
		const float pos[2] = {static_cast<float>(col), static_cast<float>(row)};
		m_input_tensors[1].fill_n(pos, i * 2, 2); // 2 elements at offset i*2, no heap alloc
	}

	// Fill masks
	m_input_tensors[2].fill(true); // all features valid (no padding)
	m_input_tensors[3].fill(true); // all graph-nodes valid (no downsampling)
}

void AiVtpClusterFactory::PrepareTensorInfo(Ort::Session &session) {

	const int64_t batchSize = 1;
	const int64_t numHits = m_rawhits().size();

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
			shape.at(0) = batchSize;
		}
		if (shape.size() > 1 && shape[1] == -1) {
			shape.at(1) = numHits;
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
			shape.at(0) = batchSize;
		}
		if (shape.size() > 1 && shape[1] == -1) {
			shape.at(1) = numHits;
		}

		for (const auto &dim : shape) {
			if (dim <= 0) {
				throw std::runtime_error("Invalid tensor shape dimension: " + std::to_string(dim));
			}
		}
		m_output_tensors.push_back(onnx::Tensor::allocate(name.get(), shape, type));
	}
}

void AiVtpClusterFactory::Describe() const {}

} // namespace nps::clustering
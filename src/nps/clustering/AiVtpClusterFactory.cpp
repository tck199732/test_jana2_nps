#include "AiVtpClusterFactory.hpp"

namespace nps::clustering {

void AiVtpClusterFactory::Configure() {}

void AiVtpClusterFactory::ChangeRun(int32_t run_number) {}

void AiVtpClusterFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	auto options = m_service_onnx().createSessionOptions(m_num_threads(), m_use_cuda());
	auto &session = m_service_onnx().createSession(m_session_name(), m_model_path(), options);
	PrepareTensors(session);

	// pack tensor data into Ort::Value
	std::vector<Ort::Value> inputTensors;
	std::vector<Ort::Value> outputTensors;

	Ort::MemoryInfo memoryInfo =
		Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

	for (auto &tensor : m_input_tensors) {
		inputTensors.push_back(tensor.GetOrtValue());
	}

	for (auto &tensor : m_output_tensors) {
		outputTensors.push_back(tensor.GetOrtValue());
	}

	std::vector<const char *> input_names;
	for (const auto &tensor : m_input_tensors) {
		input_names.push_back(tensor.name().c_str());
	}
	std::vector<const char *> output_names;
	for (const auto &tensor : m_output_tensors) {
		output_names.push_back(tensor.name().c_str());
	}

	try {
		session.Run(
			Ort::RunOptions{nullptr}, input_names.data(), inputTensors.data(), m_input_tensors.size(),
			output_names.data(), outputTensors.data(), m_output_tensors.size()
		);
	} catch (const Ort::Exception &e) {
		std::cerr << "ONNX Runtime inference failed: " << e.what() << std::endl;
		return;
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

	if (n_hits == 0)
		return;

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

	auto resolve_shape = [&](std::vector<int64_t> &shape) -> std::vector<int64_t> {
		if (shape.size() > 0 && shape[0] == -1)
			shape[0] = batchSize;
		if (shape.size() > 1 && shape[1] == -1)
			shape[1] = numHits;
		return shape;
	};

	for (size_t i = 0; i < numInputNodes; ++i) {
		auto info = session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
		m_input_tensors.push_back(
			onnx::Tensor::allocate(
				session.GetInputNameAllocated(i, allocator).get(), resolve_shape(info.GetShape()), info.GetElementType()
			)
		);
	}

	for (size_t i = 0; i < numOutputNodes; ++i) {
		auto info = session.GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo();
		m_output_tensors.push_back(
			onnx::Tensor::allocate(
				session.GetOutputNameAllocated(i, allocator).get(), resolve_shape(info.GetShape()),
				info.GetElementType()
			)
		);
	}
}

void AiVtpClusterFactory::Describe() const {}

} // namespace nps::clustering
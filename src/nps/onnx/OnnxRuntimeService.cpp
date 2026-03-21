#include "OnnxRuntimeService.hpp"

namespace onnx {

void OnnxRuntimeService::Init() {
	m_env = std::make_unique<Ort::Env>(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, m_instance_name().c_str());
}

Ort::SessionOptions OnnxRuntimeService::createSessionOptions(int num_threads, bool use_cuda) const {
	Ort::SessionOptions session_options;
	session_options.SetIntraOpNumThreads(num_threads);
	if (use_cuda) {
		// Using CUDA backend
		// https://github.com/microsoft/onnxruntime/blob/v1.8.2/include/onnxruntime/core/session/onnxruntime_cxx_api.h#L329
		OrtCUDAProviderOptions cuda_options{};
		session_options.AppendExecutionProvider_CUDA(cuda_options);
	}
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

	return session_options;
}

Ort::Session &OnnxRuntimeService::createSession(const std::string &session_name, const std::string &model_filepath) {
	auto options = createSessionOptions(/*num_threads=*/1, /*use_cuda=*/false);
	return createSession(session_name, model_filepath, options);
}

Ort::Session &OnnxRuntimeService::createSession(
	const std::string &session_name, const std::string &model_filepath, const Ort::SessionOptions &sessionOptions
) {
	std::lock_guard<std::mutex> lock(m_mutex);

	auto [it, inserted] = m_sessions.try_emplace(session_name, nullptr);
	if (inserted) {
		it->second = std::make_unique<Ort::Session>(*m_env, model_filepath.c_str(), sessionOptions);
	}
	return *(it->second);
}

} // namespace onnx

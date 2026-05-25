#include "OnnxRuntimeService.hpp"
#include <algorithm>
#include <iostream>

namespace onnx {

void OnnxRuntimeService::Init() {
	m_env = std::make_unique<Ort::Env>(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, m_instance_name().c_str());
}

Ort::SessionOptions OnnxRuntimeService::createSessionOptions(int num_threads, bool use_cuda) const {
	Ort::SessionOptions session_options;
	session_options.SetLogSeverityLevel(0);
	session_options.SetIntraOpNumThreads(num_threads);
	if (use_cuda) {
		const auto available_providers = Ort::GetAvailableProviders();
		const auto cuda_provider =
			std::find(available_providers.begin(), available_providers.end(), "CUDAExecutionProvider");
		if (cuda_provider == available_providers.end()) {
			std::string msg = "CUDAExecutionProvider is not available. Available providers are: ";
			for (const auto &provider : available_providers) {
				msg += provider + " ";
			}
			throw std::runtime_error(msg);
		}
		// Using CUDA backend
		// https://github.com/microsoft/onnxruntime/blob/v1.8.2/include/onnxruntime/core/session/onnxruntime_cxx_api.h#L329
		OrtCUDAProviderOptions cuda_options{};
		session_options.AppendExecutionProvider_CUDA(cuda_options);
	}
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

	return session_options;
}

Ort::Session &OnnxRuntimeService::createSession(
	const std::string &session_name, const std::string &model_filepath, int n_threads, bool use_cuda
) {
	Ort::SessionOptions options = createSessionOptions(n_threads, use_cuda);
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

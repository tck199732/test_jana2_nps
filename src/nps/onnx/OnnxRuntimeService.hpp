#pragma once

#include <JANA/JService.h>
#include <JANA/Services/JServiceLocator.h>
#include <onnxruntime_cxx_api.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace onnx {

class OnnxRuntimeService : public JService {

public:
	explicit OnnxRuntimeService() : JService() {}
	~OnnxRuntimeService() override = default;

	void Init() override;
	Ort::Session &createSession(
		const std::string &session_name, const std::string &model_filepath, int n_threads = 1, bool use_cuda = false
	);
	Ort::Session &createSession(
		const std::string &session_name, const std::string &model_filepath, const Ort::SessionOptions &session_options
	);

	Ort::SessionOptions createSessionOptions(int num_threads, bool use_cuda) const;
	Ort::Env &GetEnv() const { return *m_env; }

private:
	std::mutex m_mutex;
	std::unique_ptr<Ort::Env> m_env;
	std::map<std::string, std::unique_ptr<Ort::Session>> m_sessions;

	Parameter<std::string> m_instance_name{
		this, "onnx:instance_name", "onnx_runtime_service", "Instance name for ONNX Runtime environment logging."
	};
};

} // namespace onnx

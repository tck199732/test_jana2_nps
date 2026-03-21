#pragma once

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace onnx {

template <typename Func> void DispatchOnnxType(ONNXTensorElementDataType type, Func &&f) {
	switch (type) {
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
		f(float{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
		f(double{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
		f(int32_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
		f(int64_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
		f(uint8_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8:
		f(int8_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16:
		f(uint16_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16:
		f(int16_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL:
		f(bool{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16:
		f(uint16_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32:
		f(uint32_t{});
		break;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64:
		f(uint64_t{});
		break;
	default:
		throw std::runtime_error("Unsupported ONNX tensor type");
	}
}

class Tensor {
	std::string m_name;
	std::vector<int64_t> m_dims;
	ONNXTensorElementDataType m_type;
	std::unique_ptr<std::byte[]> m_data; // renamed: no conflict with data()
	size_t m_byte_size = 0;
	size_t m_n_elements = 0;

public:
	static Tensor allocate(std::string name, std::vector<int64_t> dims, ONNXTensorElementDataType type) {
		Tensor t;
		t.m_name = std::move(name);
		t.m_dims = std::move(dims);
		t.m_type = type;

		t.m_n_elements = std::accumulate(t.m_dims.begin(), t.m_dims.end(), 1LL, std::multiplies<int64_t>());

		DispatchOnnxType(type, [&](auto arg) {
			using DataT = decltype(arg);
			t.m_byte_size = t.m_n_elements * sizeof(DataT);
			t.m_data = std::make_unique<std::byte[]>(t.m_byte_size);
		});
		return t;
	}

	template <typename T> void fill_n(const T *src, size_t begin, size_t count) {
		if (src == nullptr) {
			throw std::invalid_argument("Source pointer cannot be null");
		}
		if (begin + count > m_n_elements) {
			throw std::out_of_range("fill_n: range exceeds tensor size");
		}

		DispatchOnnxType(m_type, [&](auto arg) {
			using DataT = decltype(arg);
			auto *dst = static_cast<DataT *>(static_cast<void *>(m_data.get()));

			if constexpr (std::is_same_v<T, DataT>) {
				std::memcpy(dst + begin, src, count * sizeof(DataT));
			} else {
				std::transform(src, src + count, dst + begin, [](const T &v) { return static_cast<DataT>(v); });
			}
		});
	}

	template <typename T> void fill(const T &src) {
		DispatchOnnxType(m_type, [&](auto arg) {
			using DataT = decltype(arg);
			auto *dst = static_cast<DataT *>(static_cast<void *>(m_data.get()));
			std::fill_n(dst, m_n_elements, static_cast<DataT>(src));
		});
	}

	Ort::Value GetOrtValue() {
		if (m_byte_size == 0 || m_data == nullptr) {
			throw std::runtime_error("Tensor data is not allocated");
		}
		auto mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		return Ort::Value::CreateTensor(mem_info, data(), m_byte_size, m_dims.data(), m_dims.size(), m_type);
	}

	void *data() { return static_cast<void *>(m_data.get()); }
	const void *data() const { return static_cast<const void *>(m_data.get()); }

	const char *name() const { return m_name.c_str(); }
	const std::vector<int64_t> &dims() const { return m_dims; }
	ONNXTensorElementDataType type() const { return m_type; }
	size_t byte_size() const { return m_byte_size; }
	size_t n_elements() const { return m_n_elements; }
};

} // namespace onnx
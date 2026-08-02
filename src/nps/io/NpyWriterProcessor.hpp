#pragma once

#include <JANA/JEventProcessor.h>
#include <JANA/Services/JServiceLocator.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include "geometry/NpsGeometryService.hpp"
#include "struct/cluster.hpp"
#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

#include "io/cnpy/cnpy.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

namespace nps::io {

class BaseNpyWriterProcessor : public JEventProcessor {

private:
	std::string m_dir; // output tag (e.g. run number, split, etc.)

public:
	BaseNpyWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void Init() override { m_dir = GetApplication()->GetParameterValue<std::string>("npy:output_dir"); }

	void ProcessSequential(const JEvent &event) override {
		auto run_dir = std::filesystem::path(m_dir) / std::to_string(event.GetRunNumber());
		std::error_code ec;
		std::filesystem::create_directories(run_dir, ec);
		if (ec) {
			throw std::runtime_error("Unable to create run directory '" + run_dir.string() + "': " + ec.message());
		}
		WriteEntries(event, run_dir);
	}

	void Finish() override {}

	virtual void WriteEntries(const JEvent &event, const std::filesystem::path &output_dir) = 0;
};

class VtpClusterNpyWriteProcessor : public BaseNpyWriterProcessor {
	Input<nps::fadc_waveform> m_fadc_waveforms{this, {"fadc_waveforms", JEventLevel::None, true}};
	Input<nps::fadc_hit> m_fadc_hits{this, {"fadc_hits", JEventLevel::None, true}};
	Input<nps::cluster> m_vtp_clusters{this, {"vtp_clusters", JEventLevel::None, true}};
	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

public:
	explicit VtpClusterNpyWriteProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void WriteEntries(const JEvent &event, const std::filesystem::path &output_dir) override {

		const auto &waveforms = m_fadc_waveforms();
		const auto &clusters = m_vtp_clusters();

		if (waveforms.empty()) {
			return;
		}

		size_t nsamples = 0;
		size_t max_channel = 0;

		for (const auto &waveform : waveforms) {
			nsamples = std::max(nsamples, waveform->samples.size());
			max_channel = std::max(max_channel, static_cast<size_t>(waveform->channel));
		}

		const size_t nchannels = max_channel + 1;

		std::vector<double> waveform_data(nchannels * nsamples, 0.0);

		std::vector<double> geometry_data(nchannels * 2, std::numeric_limits<double>::quiet_NaN());

		std::vector<std::int64_t> cluster_data(nchannels, -1);

		for (auto &waveform : m_fadc_waveforms()) {

			const size_t channel = static_cast<size_t>(waveform->channel);
			if (channel >= nchannels) {
				throw std::runtime_error("Waveform channel index is out of bounds");
			}

			for (size_t sample = 0; sample < waveform->samples.size(); ++sample) {
				waveform_data[channel * nsamples + sample] = static_cast<double>(waveform->samples[sample]);
			}

			const auto [column, row] = m_service_geometry().getColRowFromBlock(waveform->channel);

			geometry_data[channel * 2] = static_cast<double>(column);

			geometry_data[channel * 2 + 1] = static_cast<double>(row);
		}

		for (size_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index) {
			for (const auto channel_value : clusters[cluster_index]->channels) {
				const size_t channel = static_cast<size_t>(channel_value);

				if (channel >= nchannels) {
					throw std::runtime_error(
						"Cluster channel " + std::to_string(channel) + " is outside the waveform channel range"
					);
				}

				cluster_data[channel] = static_cast<std::int64_t>(cluster_index);
			}
		}

		cnpy::npy_save(output_dir / "waveforms.npy", waveform_data.data(), {nchannels, nsamples});
		cnpy::npy_save(output_dir / "geometry.npy", geometry_data.data(), {nchannels, 2});
		cnpy::npy_save(output_dir / "clusters.npy", cluster_data.data(), {nchannels});
	}
};

} // namespace nps::io
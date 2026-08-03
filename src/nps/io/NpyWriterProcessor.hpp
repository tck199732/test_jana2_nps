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
	std::string m_dir; // output directory

public:
	BaseNpyWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void Init() override { m_dir = GetApplication()->GetParameterValue<std::string>("npy:output_dir"); }

	void ProcessSequential(const JEvent &event) override {
		auto run_dir = std::filesystem::path(m_dir);
		auto event_dir = run_dir / std::to_string(event.GetEventNumber());
		std::error_code ec;
		std::filesystem::create_directories(event_dir, ec);
		if (ec) {
			throw std::runtime_error("Unable to create event directory '" + event_dir.string() + "': " + ec.message());
		}
		WriteEntries(event, event_dir);
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

		const auto &fadc_waveforms = m_fadc_waveforms();
		const auto &fadc_hits = m_fadc_hits();
		const auto &vtp_clusters = m_vtp_clusters();

		size_t nsamples = 0;
		size_t max_channel = 0;

		for (const auto &waveform : fadc_waveforms) {
			nsamples = std::max(nsamples, waveform->samples.size());
			max_channel = std::max(max_channel, static_cast<size_t>(waveform->channel));
		}

		const size_t nchannels = max_channel + 1;

		const size_t max_hit_per_channel = nsamples / 8; // 32ns between hits, 4ns per sample

		std::vector<double> waveform_data(nchannels * nsamples, 0.0);

		std::vector<double> hit_data(
			nchannels * 2 * max_hit_per_channel, 0.0
		); // [nchannels][max_hits * 2] (energy, time)

		std::vector<double> geometry_data(nchannels * 2, std::numeric_limits<double>::quiet_NaN());

		std::vector<std::int64_t> cluster_data(nchannels, -1);

		for (auto &waveform : fadc_waveforms) {

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

		std::vector<size_t> hit_count(nchannels, 0);
		for (auto &hit : fadc_hits) {
			const size_t channel = static_cast<size_t>(hit->channel);
			if (channel >= nchannels) {
				throw std::runtime_error("Waveform channel index is out of bounds");
			}

			size_t count = hit_count[channel];
			if (count >= max_hit_per_channel) {
				std::cerr << "Warning: Exceeded max hits for channel " << channel << ". Skipping additional hits."
						  << std::endl;
				continue;
			}

			hit_data[channel * 2 * max_hit_per_channel + count * 2] = static_cast<double>(hit->charge);
			hit_data[channel * 2 * max_hit_per_channel + count * 2 + 1] = static_cast<double>(hit->time);
			hit_count[channel]++;
		}

		for (size_t cluster_index = 0; cluster_index < vtp_clusters.size(); ++cluster_index) {
			for (const auto channel_value : vtp_clusters[cluster_index]->channels) {
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
		cnpy::npy_save(output_dir / "hits.npy", hit_data.data(), {nchannels, max_hit_per_channel * 2});
		cnpy::npy_save(output_dir / "geometry.npy", geometry_data.data(), {nchannels, 2});
		cnpy::npy_save(output_dir / "clusters.npy", cluster_data.data(), {nchannels});
	}
};

} // namespace nps::io
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

		// fixed size for waveform data: [nchannels][nsamples]
		std::vector<double> waveform_data(m_nchannels * m_nsamples, 0.0);
		for (auto &waveform : fadc_waveforms) {
			const size_t channel = static_cast<size_t>(waveform->channel);
			if (channel >= m_nchannels) {
				throw std::runtime_error("Waveform channel index is out of bounds");
			}
			for (size_t sample = 0; sample < waveform->samples.size(); ++sample) {
				waveform_data[channel * m_nsamples + sample] = static_cast<double>(waveform->samples[sample]);
			}
		}

		// multiple hits per channel, hence 1 node per hit
		std::vector<double> hit_data;
		std::vector<double> geometry_data;

		for (auto &hit : fadc_hits) {
			const size_t channel = static_cast<size_t>(hit->channel);
			if (channel >= m_nchannels) {
				throw std::runtime_error("Hit channel index is out of bounds");
			}
			hit_data.push_back(static_cast<double>(hit->charge));
			hit_data.push_back(static_cast<double>(hit->time));

			const auto [column, row] = m_service_geometry().getColRowFromBlock(channel);
			geometry_data.push_back(static_cast<double>(column));
			geometry_data.push_back(static_cast<double>(row));
		}

		std::vector<std::int64_t> edge_index_data;
		for (size_t cluster_index = 0; cluster_index < vtp_clusters.size(); ++cluster_index) {
			auto &channels = vtp_clusters[cluster_index]->channels;
			auto seed_channel = channels[0];
			for (size_t i = 1; i < channels.size(); ++i) {
				auto channel = channels[i];
				edge_index_data.push_back(static_cast<std::int64_t>(seed_channel));
				edge_index_data.push_back(static_cast<std::int64_t>(channel));
			}
		}
		size_t num_edges = edge_index_data.size() / 2;

		cnpy::npy_save(output_dir / "waveforms.npy", waveform_data.data(), {m_nchannels, m_nsamples});
		cnpy::npy_save(output_dir / "hits.npy", hit_data.data(), {fadc_hits.size(), 2});
		cnpy::npy_save(output_dir / "geometry.npy", geometry_data.data(), {fadc_hits.size(), 2});
		cnpy::npy_save(output_dir / "edge_index.npy", edge_index_data.data(), {2, num_edges});
	}

protected:
	size_t m_nsamples = 110;
	size_t m_nchannels = 1080;
};

} // namespace nps::io
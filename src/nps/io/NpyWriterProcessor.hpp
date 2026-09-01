#pragma once

#include <JANA/JEventProcessor.h>
#include <JANA/Services/JServiceLocator.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include "geometry/NpsGeometryService.hpp"
#include "struct/cluster.hpp"
#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

#include <cnpy/cnpy.hpp>

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
	Input<nps::cluster> m_vtp_clusters{this, {"vtp_clusters", JEventLevel::None, true}};
	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

public:
	explicit VtpClusterNpyWriteProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void WriteEntries(const JEvent &event, const std::filesystem::path &output_dir) override {

		const auto &fadc_waveforms = m_fadc_waveforms();
		const auto &vtp_clusters = m_vtp_clusters();

		std::vector<double> waveform_data(fadc_waveforms.size() * m_nsamples, 0.0); // [nchannels, nsamples]
		for (size_t i = 0; i < fadc_waveforms.size(); ++i) {
			std::transform(
				fadc_waveforms[i]->samples.begin(), fadc_waveforms[i]->samples.end(),
				waveform_data.begin() + i * m_nsamples, [](auto sample) { return static_cast<double>(sample); }
			);
		}

		// not the same as fadc_hits.size() since some hits are dropped during clustering,
		// e.g. not matching vtp seed.
		size_t total_hits = 0;
		for (const auto &cluster : vtp_clusters) {
			total_hits += cluster->hit_indices.size();
		}

		size_t edge_count = 0;
		for (const auto *clus : vtp_clusters) {
			edge_count += clus->hit_indices.size() - 1; // edges from seed to other hits
		}

		std::vector<double> hit_data(total_hits * 2, 0.0);			   // charge, time
		std::vector<double> geometry_data(total_hits * 2, 0.0);		   // column, row
		std::vector<std::int64_t> cluster_data(total_hits, -1);		   // object id (0,.., nclusters-1)
		std::vector<std::int64_t> cluster_type(total_hits, -1);		   // -1 = UNKNOWN, see cluster_type enum
		std::vector<std::int64_t> edge_index_data(edge_count * 2, -1); // [2, nedges]

		size_t hit_index_offset = 0;
		size_t edge_pos = 0;
		for (const auto *clus : vtp_clusters) {
			for (size_t i = 0; i < clus->hit_indices.size(); ++i) {

				size_t hit_index = hit_index_offset + i;
				hit_data[hit_index * 2] = clus->energies[i];
				hit_data[hit_index * 2 + 1] = clus->times[i];

				int channel = clus->channels[i];
				auto [col, row] = m_service_geometry().getColRowFromBlock(channel);
				geometry_data[hit_index * 2] = static_cast<double>(col);
				geometry_data[hit_index * 2 + 1] = static_cast<double>(row);
				cluster_data[hit_index] = static_cast<std::int64_t>(clus->id);
				cluster_type[hit_index] = static_cast<std::int64_t>(clus->type);
			}

			auto src_id = hit_index_offset;
			for (size_t i = 1; i < clus->channels.size(); ++i) {
				auto des_id = hit_index_offset + i;
				edge_index_data[edge_pos * 2] = static_cast<std::int64_t>(src_id);
				edge_index_data[edge_pos * 2 + 1] = static_cast<std::int64_t>(des_id);
				edge_pos++;
			}

			hit_index_offset += clus->hit_indices.size();
		}

		// sanity check: all hits are assigned to valid indices
		for (const auto *clus : vtp_clusters) {
			if (clus->id == -1) {
				throw std::runtime_error("Cluster id is -1, which is invalid.");
			}
		}

		cnpy::npy_save(output_dir / "waveforms.npy", waveform_data.data(), {fadc_waveforms.size(), m_nsamples});
		cnpy::npy_save(output_dir / "hits.npy", hit_data.data(), {total_hits, 2});
		cnpy::npy_save(output_dir / "geometry.npy", geometry_data.data(), {total_hits, 2});
		cnpy::npy_save(output_dir / "edge_index.npy", edge_index_data.data(), {2, edge_index_data.size() / 2});
		cnpy::npy_save(output_dir / "cluster_index.npy", cluster_data.data(), {total_hits});
		cnpy::npy_save(output_dir / "cluster_type.npy", cluster_type.data(), {total_hits});
	}

protected:
	size_t m_nsamples = 110;
	size_t m_nchannels = 1080;
};

} // namespace nps::io
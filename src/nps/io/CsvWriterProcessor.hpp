#pragma once

#include <JANA/JEventProcessor.h>
#include <JANA/Services/JServiceLocator.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include "geometry/NpsGeometryService.hpp"
#include "struct/cluster.hpp"
#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

#include <fstream>
#include <vector>

namespace nps::io {
class CsvWriterProcessor : public JEventProcessor {

	Input<nps::cluster> m_vtp_clusters{this, {"vtp_clusters", JEventLevel::None, true}};
	Input<nps::cluster> m_wf_clusters{this, {"waveform_clusters", JEventLevel::None, true}};
	Input<nps::cluster> m_hit_clusters{this, {"hit_clusters", JEventLevel::None, true}};
	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

private:
	std::string m_tag; // output tag (e.g. run number, split, etc.)

	// Output file streams for each cluster type
	std::ofstream m_vtp_ofile;
	std::ofstream m_wf_ofile;
	std::ofstream m_hit_ofile;

	// Output file names for each cluster type
	std::string m_vtp_filename;
	std::string m_wf_filename;
	std::string m_hit_filename;

	bool m_vtp_opened = false;
	bool m_wf_opened = false;
	bool m_hit_opened = false;

	bool HasDatabundle(const JEvent &event, const auto &input) {
		auto *facset = jana::components::GetFactorySetAtLevel(event, input.GetLevel());
		if (facset == nullptr) {
			return false;
		}
		auto *db = facset->GetDatabundle(std::type_index(typeid(nps::cluster)), input.GetDatabundleName());
		return db != nullptr;
	}

public:
	CsvWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void Init() override {

		m_tag = GetApplication()->GetParameterValue<std::string>("csv:output_tag");

		m_vtp_filename = "vtp." + m_tag + ".csv";
		m_wf_filename = "wf." + m_tag + ".csv";
		m_hit_filename = "hit." + m_tag + ".csv";

		// check if the input is activated ?

		m_vtp_ofile.open(m_vtp_filename);
		if (!m_vtp_ofile.is_open()) {
			throw JException("Failed to open VTP cluster output file: %s", m_vtp_filename.c_str());
		}
		writeClusterHeader(m_vtp_ofile);

		m_wf_ofile.open(m_wf_filename);
		if (!m_wf_ofile.is_open()) {
			throw JException("Failed to open WF cluster output file: %s", m_wf_filename.c_str());
		}
		writeClusterHeader(m_wf_ofile);

		m_hit_ofile.open(m_hit_filename);
		if (!m_hit_ofile.is_open()) {
			throw JException("Failed to open HIT cluster output file: %s", m_hit_filename.c_str());
		}
		writeClusterHeader(m_hit_ofile);
	}

	void OpenIfNeeded(const JEvent &event) {
		if (!m_vtp_opened && HasDatabundle(event, m_vtp_clusters)) {
			m_vtp_ofile.open(m_vtp_filename);
			writeClusterHeader(m_vtp_ofile);
			m_vtp_opened = true;
		}
		if (!m_wf_opened && HasDatabundle(event, m_wf_clusters)) {
			m_wf_ofile.open(m_wf_filename);
			writeClusterHeader(m_wf_ofile);
			m_wf_opened = true;
		}
		if (!m_hit_opened && HasDatabundle(event, m_hit_clusters)) {
			m_hit_ofile.open(m_hit_filename);
			writeClusterHeader(m_hit_ofile);
			m_hit_opened = true;
		}
	}

	void writeClusterHeader(std::ofstream &ofile) {
		std::vector<std::string> fields = {
			"event_id",	  // event number
			"cluster_id", // cluster identifier
			"hit_id",	  // hit identifier within the cluster
			"channel",	  // channel number
			"row_id",	  // row identifier
			"column_id",  // column identifier
			"energy",	  // energy
			"time"		  // time
		};

		for (size_t i = 0; i < fields.size(); ++i) {
			ofile << fields[i];
			if (i < fields.size() - 1) {
				ofile << ",";
			}
		}
		ofile << "\n";
	}

	void ProcessSequential(const JEvent &event) override {

		OpenIfNeeded(event);
		uint64_t eventNumber = event.GetEventNumber();

		if (m_vtp_opened) {
			const auto &vtp_clusters = m_vtp_clusters();
			for (std::size_t i = 0; i < vtp_clusters.size(); ++i) {
				const auto *clus = vtp_clusters[i];
				WriteClusterEntry(eventNumber, i, clus, m_vtp_ofile);
			}
		}

		if (m_wf_opened) {
			const auto &wf_clusters = m_wf_clusters();

			for (std::size_t i = 0; i < wf_clusters.size(); ++i) {
				const auto *clus = wf_clusters[i];
				WriteClusterEntry(eventNumber, i, clus, m_wf_ofile);
			}
		}

		if (m_hit_opened) {
			const auto &hit_clusters = m_hit_clusters();

			for (std::size_t i = 0; i < hit_clusters.size(); ++i) {
				const auto *clus = hit_clusters[i];
				WriteClusterEntry(eventNumber, i, clus, m_hit_ofile);
			}
		}
	}

	void WriteClusterEntry(uint64_t eventIndex, uint64_t clus_id, const nps::cluster *clus, std::ofstream &ofile) {

		auto clus_size = clus->channels.size();

		for (size_t hit_id = 0; hit_id < clus_size; ++hit_id) {
			auto ch = clus->channels[hit_id];
			auto t = clus->times[hit_id];
			auto e = clus->energies[hit_id];
			auto [col, row] = m_service_geometry().getColRowFromBlock(ch);

			ofile << eventIndex << "," << clus_id << "," << hit_id << "," << ch << "," << row << "," << col << "," << e
				  << "," << t << "\n";
		}
	}

	void Finish() override {
		if (m_vtp_ofile.is_open()) {
			m_vtp_ofile.close();
		}
		if (m_wf_ofile.is_open()) {
			m_wf_ofile.close();
		}
		if (m_hit_ofile.is_open()) {
			m_hit_ofile.close();
		}
	}
};

} // namespace nps::io
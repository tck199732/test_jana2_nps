#pragma once

#include <JANA/JEventProcessor.h>
#include <JANA/Services/JServiceLocator.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include "geometry/NpsGeometryService.hpp"
#include "nps/Cluster.hpp"
#include "nps/RawHit.hpp"
#include "nps/VtpSeed.hpp"

#include <fstream>
#include <vector>

namespace nps::io {
class CsvWriterProcessor : public JEventProcessor {

	// Input<nps::Cluster> m_vtp_clusters{this, {"VtpClusters"}};
	Input<nps::Cluster> m_reco_clusters{this, {"RecoClusters"}};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

private:
	Parameter<std::string> m_cfg_filePrefix{
		this, "nps:output_prefix", "nps_output", "Output prefix for created files (no extension, alias to -o,--output)"
	};

	// std::ofstream m_vtp_clusterFile;
	// std::string m_vtp_clusterFileName;
	std::ofstream m_reco_clusterFile;
	std::string m_reco_clusterFileName;

public:
	CsvWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void Init() override {
		// m_vtp_clusterFileName = m_cfg_filePrefix() + ".clusters.csv";
		// m_vtp_clusterFile.open(m_vtp_clusterFileName);
		// if (!m_vtp_clusterFile.is_open()) {
		// 	throw JException("Failed to open VTP cluster output file: %s", m_vtp_clusterFileName.c_str());
		// }

		m_reco_clusterFileName = m_cfg_filePrefix() + ".reco_clusters.csv";
		m_reco_clusterFile.open(m_reco_clusterFileName);
		if (!m_reco_clusterFile.is_open()) {
			throw JException("Failed to open Reco cluster output file: %s", m_reco_clusterFileName.c_str());
		}

		writeClusterHeader(m_reco_clusterFile);
		// writeClusterHeader(m_vtp_clusterFile);
	}

	void writeClusterHeader(std::ofstream &file) {
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
			file << fields[i];
			if (i < fields.size() - 1) {
				file << ",";
			}
		}
		file << "\n";
	}

	void ProcessSequential(const JEvent &event) override {
		uint64_t eventNumber = event.GetEventNumber();
		for (auto clus : m_reco_clusters()) {
			WriteClusterEntry(m_reco_clusterFile, eventNumber, clus, true);
		}

		// for (auto clus : m_vtp_clusters()) {
		// 	WriteClusterEntry(m_vtp_clusterFile, eventNumber, clus, false);
		// }
	}

	void WriteClusterEntry(std::ofstream &file, uint64_t eventIndex, const nps::Cluster *clus, bool useClusEvent) {

		auto evt_id = useClusEvent ? clus->getEventIndex() + eventIndex : eventIndex;
		auto clus_id = clus->getClusterIndex();
		auto channels = clus->getChannels();
		auto energies = clus->getEnergies();
		auto times = clus->getTimes();

		for (size_t hit_id = 0; hit_id < channels.size(); ++hit_id) {
			auto ch = channels[hit_id];
			auto t = times[hit_id];
			auto e = energies[hit_id];
			auto [col, row] = m_service_geometry().getColRowFromBlock(ch);

			file << evt_id << "," << clus_id << "," << hit_id << "," << ch << "," << row << "," << col << "," << e
				 << "," << t << "\n";
		}
	}

	void Finish() override {
		// if (m_vtp_clusterFile.is_open()) {
		// 	m_vtp_clusterFile.close();
		// }
		if (m_reco_clusterFile.is_open()) {
			m_reco_clusterFile.close();
		}
	}
};

} // namespace nps::io
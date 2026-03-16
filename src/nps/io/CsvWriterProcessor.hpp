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

	// Later add Clusters reconstructed from AI/ML algorithms
	Input<nps::Cluster> m_in_clusters{this, {"VtpClusters"}};

	Service<nps::geo::NpsGeometryService> m_service_geometry{this};

private:
	std::string m_cfg_filePrefix;
	std::ofstream m_vtp_clusterFile;
	std::string m_vtp_clusterFileName;

public:
	CsvWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void Init() override {

		m_cfg_filePrefix = GetApplication()->GetParameterValue<std::string>("nps:output");

		// Construct file names
		m_vtp_clusterFileName = m_cfg_filePrefix + ".clusters.csv";

		// Open files
		m_vtp_clusterFile.open(m_vtp_clusterFileName);

		if (!m_vtp_clusterFile.is_open()) {
			throw JException("Failed to open VTP cluster output file: %s", m_vtp_clusterFileName.c_str());
		}
		writeVtpClusterHeader();
	}

	void writeVtpClusterHeader() {
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
			m_vtp_clusterFile << fields[i];
			if (i < fields.size() - 1) {
				m_vtp_clusterFile << ",";
			}
		}
		m_vtp_clusterFile << "\n";
	}

	void ProcessSequential(const JEvent &event) override {
		uint64_t eventNumber = event.GetEventNumber();
		for (auto clus : m_in_clusters()) {
			WriteClusterEntry(eventNumber, clus);
		}
	}

	void WriteClusterEntry(uint64_t eventIndex, const nps::Cluster *clus) {
		auto clus_id = clus->getIndex();
		auto channels = clus->getChannels();
		auto energies = clus->getEnergies();
		auto times = clus->getTimes();

		for (size_t hit_id = 0; hit_id < channels.size(); ++hit_id) {
			auto ch = channels[hit_id];
			auto t = times[hit_id];
			auto e = energies[hit_id];
			auto [col, row] = m_service_geometry().getColRowFromBlock(ch);

			m_vtp_clusterFile << eventIndex << "," << clus_id << "," << hit_id << "," << ch << "," << row << "," << col
							  << "," << e << "," << t << "\n";
		}
	}

	void Finish() override {
		if (m_vtp_clusterFile.is_open()) {
			m_vtp_clusterFile.close();
		}
	}
};

} // namespace nps::io
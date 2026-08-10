#include "VtpService.hpp"

namespace nps::calib {

void VtpService::Init() {

	if (m_config_file().empty()) {
		throw std::runtime_error(
			"Error: Configuration file path is empty. Please set the 'calib:vtp_config_file' parameter."
		);
	}
	Load(m_config_file());
}

bool VtpService::Load(const std::string &filename) {

	std::ifstream config_file(filename);
	if (!config_file.is_open()) {
		std::cerr << "Error opening configuration file: " << filename << std::endl;
		return false;
	}

	std::string line;
	std::getline(config_file, line); // header
	std::stringstream ss(line);

	std::vector<std::string> columns;
	std::string column;

	auto trim = [](std::string &s) {
		s.erase(0, s.find_first_not_of(" \t\r\n"));
		s.erase(s.find_last_not_of(" \t\r\n") + 1);
	};

	while (std::getline(ss, column, ',')) {
		trim(column);
		columns.push_back(column);
	}

	std::vector<std::string> required_fields = {
		"channel",
		"VTP_FIRMWARETYPE",
		"VTP_FIRMWAREVERSION",
		"VTP_W_OFFSET",
		"VTP_W_WIDTH",
		"VTP_NPS_ECALCLUSTER_HIT_DT",
		"VTP_NPS_ECALCLUSTER_SEED_THR",
		"VTP_NPS_ECALCLUSTER_NHIT_MIN",
		"VTP_NPS_ECALCLUSTER_CLUSTER_READOUT_THR",
		"VTP_NPS_ECALCLUSTER_CLUSTER_TRIGGER_THR",
		"VTP_NPS_ECALCLUSTER_CLUSTER_PAIR_TRIGGER_THR",
		"VTP_NPS_ECALCLUSTER_CLUSTER_PAIR_TRIGGER_WIDTH",
		"VTP_NPS_ECALCLUSTER_FADCMASK_MODE"
	};

	for (const auto &field : required_fields) {
		if (std::find(columns.begin(), columns.end(), field) == columns.end()) {
			std::cerr << "Error: Required field " << field << " not found in VTP config file.\n";
			return false;
		}
	}

	std::unordered_map<std::string, size_t> col_idx;
	for (size_t i = 0; i < columns.size(); ++i) {
		col_idx[columns[i]] = i;
	}

	while (std::getline(config_file, line)) {

		std::vector<std::string> values;
		std::stringstream ss(line);
		std::string value;

		while (std::getline(ss, value, ',')) {
			values.push_back(value);
		}

		if (values.size() != columns.size()) {
			std::cerr << "Column count mismatch\n";
			continue;
		}

		int ch = std::stoi(values[col_idx["channel"]]);

		m_config.firmware_type[ch] = std::stoi(values[col_idx["VTP_FIRMWARETYPE"]]);
		m_config.firmware_ver[ch] = std::stoi(values[col_idx["VTP_FIRMWAREVERSION"]]);
		m_config.offset[ch] = std::stoi(values[col_idx["VTP_W_OFFSET"]]);
		m_config.width[ch] = std::stoi(values[col_idx["VTP_W_WIDTH"]]);
		m_config.cluster_hit_dt[ch] =
			std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_HIT_DT"]]); // in units of time samples

		m_config.cluster_seed_thr[ch] = std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_SEED_THR"]]);
		m_config.cluster_nhits_min[ch] = std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_NHIT_MIN"]]);
		m_config.cluster_readout_thr[ch] = std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_CLUSTER_READOUT_THR"]]);
		m_config.cluster_trigger_thr[ch] = std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_CLUSTER_TRIGGER_THR"]]);
		m_config.cluster_pair_trigger_thr[ch] =
			std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_CLUSTER_PAIR_TRIGGER_THR"]]);
		m_config.cluster_pair_trigger_width[ch] =
			std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_CLUSTER_PAIR_TRIGGER_WIDTH"]]);
		m_config.fadcmask_mode[ch] = std::stoi(values[col_idx["VTP_NPS_ECALCLUSTER_FADCMASK_MODE"]]);
	}

	config_file.close();

	return true;
}

void VtpService::Reset() {

	m_config.firmware_type.clear();
	m_config.firmware_ver.clear();
	m_config.offset.clear();
	m_config.width.clear();
	m_config.cluster_hit_dt.clear();
	m_config.cluster_seed_thr.clear();
	m_config.cluster_nhits_min.clear();
	m_config.cluster_readout_thr.clear();
	m_config.cluster_trigger_thr.clear();
	m_config.cluster_pair_trigger_thr.clear();
	m_config.cluster_pair_trigger_width.clear();
	m_config.fadcmask_mode.clear();
}

} // namespace nps::calib
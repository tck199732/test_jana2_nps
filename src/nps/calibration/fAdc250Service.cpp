#include "fAdc250Service.hpp"

namespace nps::calib {

void fAdc250Service::Init() {

	if (m_config_file().empty()) {
		throw std::runtime_error(
			"Error: Configuration file path is empty. Please set the 'calib:fadc_config_file' parameter."
		);
	}
	Load(m_config_file());
}

void fAdc250Service::Reset() {
	m_config.thr.clear();
	m_config.gain.clear();
	m_config.ped.clear();
	m_config.nsa.clear();
	m_config.nsb.clear();
}

bool fAdc250Service::Load(const std::string &filename) {

	std::ifstream config_file(filename.c_str());

	if (!config_file.is_open()) {
		std::cerr << "Error opening configuration file: " << filename << std::endl;
		return false;
	}
	config_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::string line;

	while (std::getline(config_file, line)) {
		std::stringstream ss(line);
		std::string value;
		std::vector<std::string> row_data;
		while (std::getline(ss, value, ',')) {
			row_data.push_back(value);
		}

		// channel,FADC250_MODE,FADC250_COMPRESSION,FADC250_VXSREADOUT,FADC250_W_OFFSET,FADC250_W_WIDTH,FADC250_NSA,FADC250_NSB,FADC250_NPEAK,FADC250_TRG_MASK,FADC250_TRG_WIDTH,FADC250_TRG_MINTOT,FADC250_TRG_MINMULT,FADC250_ADC_MASK,FADC250_TET_IGNORE_MASK,FADC250_INVERT_MASK,FADC250_PLAYBACK_DISABLE_MASK,FADC250_TRG_MODE_MASK,FADC250_ALLCH_DAC,FADC250_ALLCH_PED,FADC250_ALLCH_TET,FADC250_ALLCH_DELAY,FADC250_ALLCH_GAIN,FADC250_SPARSIFICATION,FADC250_ACCUMULATOR_SCALER_MODE_MASK

		int channel = std::stoi(row_data[0]);
		auto FADC250_ALLCH_TET = std::stod(row_data[20]);			   // ADC unit
		auto FADC250_ALLCH_GAIN = std::stod(row_data[22]);			   // ADC * GAIN --> MeV
		auto FADC250_NSA = std::stod(row_data[6]) / m_time_interval(); // timestamp unit
		auto FADC250_NSB = std::stod(row_data[7]) / m_time_interval(); // timestamp unit
		auto FADC250_ALLCH_PED = std::stod(row_data[19]);			   // ADC unit

		if (std::floor(FADC250_NSA) != FADC250_NSA || std::floor(FADC250_NSB) != FADC250_NSB) {
			throw std::runtime_error(
				"Error: FADC250_NSA and FADC250_NSB must be multiples of " + std::to_string(m_time_interval()) + " ns."
			);
		}

		m_config.thr[channel] = FADC250_ALLCH_TET;
		m_config.gain[channel] = FADC250_ALLCH_GAIN;
		m_config.nsa[channel] = static_cast<int>(FADC250_NSA);
		m_config.nsb[channel] = static_cast<int>(FADC250_NSB);
		m_config.ped[channel] = FADC250_ALLCH_PED;
	}

	m_config.clock_cycles = m_clock_cycles();
	m_config.time_interval = m_time_interval();

	config_file.close();
	return true;
}

} // namespace nps::calib

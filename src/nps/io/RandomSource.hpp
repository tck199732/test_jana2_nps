#pragma once

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

namespace nps::io {

class RandomSource : public JEventSource {

	std::mt19937 m_rng{std::random_device{}()};
	std::uniform_int_distribution<int> m_channel_dist;
	std::uniform_real_distribution<double> m_waveform_dist;
	std::uniform_real_distribution<double> m_clus_energy_dist;
	std::uniform_real_distribution<double> m_clus_time_dist;

	Parameter<int> m_nhits{this, "random_source:nhits", 10, "Number of hits to generate per event. Default is 10."};

	Parameter<int> m_nfeatures{
		this, "random_source:nfeatures", 110,
		"Number of features (time samples) to generate for each hit. Default is 110."
	};

	Parameter<int> m_channel_min{
		this, "random_source:channel_min", 0, "Minimum channel number for generated hits. Default is 0."
	};

	Parameter<int> m_channel_max{
		this, "random_source:channel_max", 1079, "Maximum channel number for generated hits. Default is 1079."
	};

	Parameter<int> m_waveform_min{
		this, "random_source:waveform_min", 0, "Minimum value for generated hit waveforms. Default is 0."
	};

	Parameter<int> m_waveform_max{
		this, "random_source:waveform_max", 65535, "Maximum value for generated hit waveforms. Default is 65535."
	};

	Parameter<float> m_clus_energy_min{
		this, "random_source:clus_energy_min", 0.0, "Minimum energy for generated VTP seeds. Default is 0.0."
	};

	Parameter<float> m_clus_energy_max{
		this, "random_source:clus_energy_max", 100.0, "Maximum energy for generated VTP seeds. Default is 100.0."
	};

	Parameter<float> m_clus_time_min{
		this, "random_source:clus_time_min", 0.0, "Minimum time for generated VTP seeds. Default is 0.0."
	};

	Parameter<float> m_clus_time_max{
		this, "random_source:clus_time_max", 100.0, "Maximum time for generated VTP seeds. Default is 100.0."
	};

	Parameter<int> m_run_number{
		this, "random_source:run_number", 0, "Run number to assign to events from the source. Default is 0."
	};

public:
	RandomSource();

	virtual ~RandomSource() = default;

	void Init() override;

	void Open() override;

	void Close() override;

	Result Emit(JEvent &event) override;

	static std::string GetDescription();
};

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::RandomSource>::CheckOpenable(std::string resource_name);
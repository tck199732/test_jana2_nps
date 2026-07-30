#include "RandomSource.hpp"

namespace nps::io {

RandomSource::RandomSource() : JEventSource() {
	SetTypeName(NAME_OF_THIS); // Provide JANA with class name
	SetCallbackStyle(CallbackStyle::ExpertMode);
}

void RandomSource::Init() {
	m_channel_dist = std::uniform_int_distribution<int>(m_channel_min(), m_channel_max());
	m_waveform_dist = std::uniform_real_distribution<double>(m_waveform_min(), m_waveform_max());
	m_clus_energy_dist = std::uniform_real_distribution<double>(m_clus_energy_min(), m_clus_energy_max());
	m_clus_time_dist = std::uniform_real_distribution<double>(m_clus_time_min(), m_clus_time_max());
}

void RandomSource::Open() { std::string resource_name = GetResourceName(); }

void RandomSource::Close() {}

JEventSource::Result RandomSource::Emit(JEvent &event) {
	static size_t current_event_number = 0;
	event.SetEventNumber(current_event_number++);
	event.SetRunNumber(m_run_number());

	const std::size_t hit_count = m_nhits();
	const std::size_t feature_count = m_nfeatures();

	std::vector<nps::fadc_hit *> hits;
	hits.reserve(hit_count);

	std::vector<nps::vtp_seed *> seeds;
	seeds.reserve(hit_count);

	for (std::size_t ihit = 0; ihit < hit_count; ++ihit) {
		std::vector<double> waveform(feature_count);

		std::generate(waveform.begin(), waveform.end(), [this] { return m_waveform_dist(m_rng); });

		const double charge = std::accumulate(waveform.begin(), waveform.end(), 0.0) * 0.1;

		auto *hit = new nps::fadc_hit{
			.channel = m_channel_dist(m_rng),
			.charge = charge,
			.time = m_clus_time_dist(m_rng),
			.waveform = std::move(waveform)
		};

		hits.push_back(hit);

		seeds.push_back(new nps::vtp_seed{
			.channel = hit->channel, .size = 1, .time = m_clus_time_dist(m_rng), .energy = m_clus_energy_dist(m_rng)
		});
	}

	event.Insert(hits, "fadc_hits");
	event.Insert(seeds, "vtp_seeds");

	return Result::Success;
}

std::string RandomSource::GetDescription() { return ""; }

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::RandomSource>::CheckOpenable(std::string resource_name) {
	return 1.0;
}

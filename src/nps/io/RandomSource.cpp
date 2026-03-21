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

	std::vector<nps::RawHit *> hits;
	for (size_t ihit = 0; ihit < m_nhits(); ihit++) {
		auto ch = m_channel_dist(m_rng);
		std::vector<double> signal(m_nfeatures());
		std::generate(signal.begin(), signal.end(), [&] { return m_waveform_dist(m_rng); });
		auto hit = new nps::RawHit(ch, std::move(signal));
		hits.push_back(hit);
	}

	// transform each hit into a VtpSeed with clus size 1
	std::vector<nps::VtpSeed *> seeds(hits.size());
	std::transform(hits.begin(), hits.end(), seeds.begin(), [&](const nps::RawHit *hit) {
		return new nps::VtpSeed(
			hit->getChannel(),
			1,						  // size
			m_clus_time_dist(m_rng),  // time
			m_clus_energy_dist(m_rng) // energy
		);
	});

	event.Insert(hits, "RawHits");
	event.Insert(seeds, "VtpSeeds");

	return Result::Success;
}

std::string RandomSource::GetDescription() { return ""; }

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::RandomSource>::CheckOpenable(std::string resource_name) {
	return 1.0;
}

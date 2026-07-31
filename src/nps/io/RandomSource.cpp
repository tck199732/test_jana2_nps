#include "RandomSource.hpp"

namespace nps::io {

RandomSource::RandomSource() : JEventSource() {
	SetTypeName(NAME_OF_THIS); // Provide JANA with class name
	SetCallbackStyle(CallbackStyle::ExpertMode);
}

void RandomSource::Init() {
	m_channel_dist = std::uniform_int_distribution<int>(m_channel_min(), m_channel_max());
	m_waveform_dist = std::uniform_real_distribution<double>(m_waveform_min(), m_waveform_max());
}

void RandomSource::Open() { std::string resource_name = GetResourceName(); }

void RandomSource::Close() {}

JEventSource::Result RandomSource::Emit(JEvent &event) {
	static size_t current_event_number = 0;
	event.SetEventNumber(current_event_number++);
	event.SetRunNumber(m_run_number());

	const std::size_t hit_count = m_nhits();
	const std::size_t feature_count = m_nfeatures();

	std::vector<nps::fadc_hit *> fadc_hits;
	std::vector<nps::fadc_waveform *> fadc_waveforms;
	std::vector<nps::vtp_seed *> vtp_seeds;

	fadc_hits.reserve(hit_count);
	fadc_waveforms.reserve(hit_count);
	vtp_seeds.reserve(hit_count);

	for (std::size_t ihit = 0; ihit < hit_count; ++ihit) {

		std::vector<double> waveform(feature_count);
		std::vector<double> timestamps(feature_count);
		std::generate(waveform.begin(), waveform.end(), [this] { return m_waveform_dist(m_rng); });
		std::iota(timestamps.begin(), timestamps.end(), 1);

		const int channel = m_channel_dist(m_rng);
		const double charge = std::accumulate(waveform.begin(), waveform.end(), 0.0);
		const double clus_energy = charge * 0.1;
		const double time =
			timestamps[std::distance(waveform.begin(), std::max_element(waveform.begin(), waveform.end()))];

		fadc_hits.push_back(new nps::fadc_hit{
			.channel = channel,
			.charge = charge,
			.time = time,
		});

		fadc_waveforms.push_back(new nps::fadc_waveform{
			.channel = channel,
			.samples = std::move(waveform),
			.timestamps = std::move(timestamps),
		});

		vtp_seeds.push_back(new nps::vtp_seed{.channel = channel, .size = 1, .time = time, .energy = clus_energy});
	}

	event.Insert(fadc_hits, "fadc_hits");
	event.Insert(fadc_waveforms, "fadc_waveforms");
	event.Insert(vtp_seeds, "vtp_seeds");

	return Result::Success;
}

std::string RandomSource::GetDescription() { return ""; }

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::RandomSource>::CheckOpenable(std::string resource_name) {
	return 1.0;
}

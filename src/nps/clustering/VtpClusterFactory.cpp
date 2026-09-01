#include "VtpClusterFactory.hpp"

namespace nps::clustering {

void VtpClusterFactory::Configure() { m_service_geometry(); }

void VtpClusterFactory::ChangeRun(int32_t run_number) {}

void VtpClusterFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	std::vector<nps::fadc_hit> fadc_hits;

	if (m_use_waveform()) {
		for (const auto *waveform_ptr : m_fadc_waveforms()) {
			if (waveform_ptr == nullptr) {
				continue;
			}
			processRawWaveform(waveform_ptr, fadc_hits);
		}

	} else {
		for (const auto *hit_ptr : m_input_fadc_hits()) {
			if (hit_ptr == nullptr) {
				continue;
			}
			fadc_hits.push_back(*hit_ptr);
		}
	}

	// vtp clusterization
	std::vector<nps::cluster> candidates = selectGridCandidate(fadc_hits); // spacetime grids
	std::vector<nps::cluster> seed_clusters;							   // seed clusters

	std::vector<bool> matched_seeds(m_vtp_seeds().size(), false); // track which seeds have been matched

	for (auto &candidate : candidates) {
		if (isSeed(candidate)) {

			// only take the matched clusters
			if (m_match_seed()) {
				for (size_t i = 0; i < m_vtp_seeds().size(); ++i) {
					const auto *seed_ptr = m_vtp_seeds()[i];
					if (seed_ptr == nullptr || matched_seeds[i]) {
						continue;
					}
					if (isMatched(candidate, *seed_ptr, m_de_thr(), m_tmin(), m_tmax())) {
						matched_seeds[i] = true; // mark this seed as matched
						seed_clusters.emplace_back(std::move(candidate));
						break; // Move to the next candidate after a match
					}
				}
			}
			// take every
			else {
				seed_clusters.emplace_back(std::move(candidate));
			}
		}
	}

	for (size_t i = 0; i < seed_clusters.size(); ++i) {
		// reset cluster idx
		seed_clusters[i].id = static_cast<int>(i);
		// update trigger flag
		seed_clusters[i].type =
			isTriggered(seed_clusters[i]) ? nps::cluster_type::TRIGGERED : nps::cluster_type::NOT_TRIGGERED;
	}

	// take everything
	for (auto &clus : seed_clusters) {
		m_clusters().push_back(new nps::cluster(std::move(clus)));
	}
}

void VtpClusterFactory::processRawWaveform(const nps::fadc_waveform *waveform, std::vector<nps::fadc_hit> &hits) {

	const auto &cfg = m_service_fadc().getConfig(); // get the latest config in case it was updated at runtime

	const int channel = waveform->channel;
	const auto &samples = waveform->samples;

	auto clk = cfg.clock_cycles;
	auto dt = cfg.time_interval;

	auto ped = cfg.ped.at(channel);
	auto thr = cfg.thr.at(channel) + ped;
	auto gain = cfg.gain.at(channel);
	auto nsa = cfg.nsa.at(channel);
	auto nsb = cfg.nsb.at(channel);

	const auto pulse_times = findPulses(samples, thr, clk);
	for (const int t : pulse_times) {
		// integration
		int begin = std::max(0, t - nsb);
		int end = std::min((int)samples.size() - 1, t + nsa - 1);
		int nsamples = end - begin + 1;
		auto raw_sum = std::accumulate(samples.begin() + begin, samples.begin() + end + 1, 0.0);

		// pedestal subtraction
		auto ped_sub = static_cast<int>(ped * nsamples + (0.001 * nsamples));
		auto charge = static_cast<double>(static_cast<int>(raw_sum) - ped_sub);

		// apply gain and saturation
		auto scaled_gain = static_cast<int>(gain * 256.0);
		charge = static_cast<double>(static_cast<int>(charge * scaled_gain) / 256.0);
		charge = std::max(0.0, std::min(8191.0, charge));

		hits.emplace_back(
			nps::fadc_hit{
				.channel = static_cast<int>(channel),
				.charge = static_cast<double>(charge),
				.time = static_cast<double>(t),
			}
		);
	}
}

std::vector<int> VtpClusterFactory::findPulses(const std::vector<double> &waveform_adc, double thr, int clk) const {
	int current_over = 0;
	int last_over = 0;
	int last_over_hist = 0;
	std::vector<int> res;

	uint32_t mask = (1u << clk) - 1;

	for (int i = 0; i < (int)waveform_adc.size(); ++i) {
		current_over = (waveform_adc[i] > thr) ? 1 : 0;

		if (i > 0 && current_over && !last_over && !(last_over_hist & mask)) {
			res.push_back(i);
		}

		// Update history: Only record the leading edge transition in the bit history
		if (current_over && !last_over) {
			last_over_hist = (last_over_hist << 1) | 1;
		} else {
			last_over_hist = (last_over_hist << 1);
		}

		last_over = current_over;
	}
	return res;
}

std::vector<nps::cluster> VtpClusterFactory::selectGridCandidate(const std::vector<nps::fadc_hit> &hits) {

	const auto &vtp_cfg = m_service_vtp().getConfig();
	const auto &fadc_cfg = m_service_fadc().getConfig();

	std::vector<nps::cluster> candidates;
	candidates.reserve(hits.size()); // number of hits >= number of clusters

	for (std::size_t i = 0; i < hits.size(); ++i) {
		const auto hit = hits[i];

		nps::cluster candidate;
		int nch = m_grid_size() * m_grid_size();
		candidate.channels.reserve(nch);
		candidate.hit_indices.reserve(nch);
		candidate.energies.reserve(nch);
		candidate.times.reserve(nch);

		candidate.channels.push_back(hit.channel);
		candidate.hit_indices.push_back(i);
		candidate.energies.push_back(hit.charge);
		candidate.times.push_back(hit.time);

		const auto it = vtp_cfg.cluster_hit_dt.find(hit.channel);
		if (it == vtp_cfg.cluster_hit_dt.end()) {
			continue; // or throw an exception
		}

		const auto clus_dt = it->second; // in unit of time bucket
		for (std::size_t j = 0; j < hits.size(); ++j) {
			if (i == j) {
				continue;
			}

			const auto neighbor_hit = hits[j];
			if (std::abs(neighbor_hit.time - hit.time) > clus_dt) {
				continue;
			}

			if (m_service_geometry().isInsideGrid(hit.channel, neighbor_hit.channel, m_grid_size())) {
				candidate.id = static_cast<int>(i);
				candidate.type = nps::cluster_type::NOT_TRIGGERED; // update later
				candidate.channels.push_back(neighbor_hit.channel);
				candidate.hit_indices.push_back(j);
				candidate.energies.push_back(neighbor_hit.charge);
				candidate.times.push_back(neighbor_hit.time);
			}
		}
		candidates.emplace_back(std::move(candidate));
	}

	return candidates;
}

bool VtpClusterFactory::isSeed(const nps::cluster &clus) {
	const auto &cfg = m_service_vtp().getConfig();
	const auto &channels = clus.channels;
	const auto &energies = clus.energies;
	const auto &times = clus.times;

	auto ch_seed = channels[0]; // first hit is the seed
	auto seed_thr = cfg.cluster_seed_thr.at(ch_seed);

	// seed energy requirement
	if (energies[0] < seed_thr) {
		return false;
	}

	// local maximum requirement
	for (int i = 1; i < energies.size(); i++) {
		if (energies[i] > energies[0]) {
			return false;
		}
	}
	return true;
}

bool VtpClusterFactory::isTriggered(const nps::cluster &clus) {
	const auto &cfg = m_service_vtp().getConfig();

	const auto &channels = clus.channels;
	const auto &energies = clus.energies;
	const auto &times = clus.times;

	int size = channels.size();
	auto ch_seed = channels[0]; // first hit is the seed

	auto seed_thr = cfg.cluster_seed_thr.at(ch_seed);
	auto min_hits = cfg.cluster_nhits_min.at(ch_seed);
	auto cluster_thr = cfg.cluster_trigger_thr.at(ch_seed);
	auto pair_cluster_thr = cfg.cluster_pair_trigger_thr.at(ch_seed);

	// minimum number of hits
	if (size < min_hits) {
		return false;
	}

	auto total_e = std::accumulate(energies.begin(), energies.end(), 0.0);
	if (total_e < cluster_thr) {
		return false;
	}

	return true;
}

bool VtpClusterFactory::isMatched(
	const nps::cluster &clus, const nps::vtp_seed &seed, double de_thr, double tmin, double tmax
) {

	bool match = (seed.channel == clus.channels[0]); // same seed channel
	match &= (seed.time == clus.times[0]);			 // same rise time
	match &= (seed.size == clus.channels.size());	 // same cluster size
	match &=
		(std::abs(seed.energy - std::accumulate(clus.energies.begin(), clus.energies.end(), 0.0)) <
		 de_thr);										 // energy difference within threshold
	match &= (seed.time >= tmin) && (seed.time <= tmax); // time window requirement
	return match;
}

} // namespace nps::clustering
#include "VtpClusterFactory.hpp"

namespace nps::clustering {

void VtpClusterFactory::Configure() { m_service_geometry(); }

void VtpClusterFactory::ChangeRun(int32_t run_number) {
	// TODO : update config in all services
	// m_service_vtp().Reset();
	// m_service_fadc().Reset();
}

void VtpClusterFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	// find all hits for each waveform
	std::vector<fAdcHit> fadc_hits;
	for (const auto &hit : m_rawhits()) {
		auto channel = hit->getChannel();
		auto waveform = hit->getWaveform();
		processRawWaveform(waveform, channel, fadc_hits);
	}

	// vtp clusterization
	std::vector<Cluster> candidates = selectGridCandidate(fadc_hits);
	std::vector<Cluster> reco_clusters;
	for (auto c : candidates) {
		if (isTriggered(c)) {
			reco_clusters.push_back(std::move(c));
		}
	}

	// compare with VTP seeds and fill the matched clusters
	std::unordered_set<int> usedRecoIndices;
	std::unordered_set<int> usedVtpIndices;

	const auto &seeds = m_vtpseeds();

	for (size_t i_seed = 0; i_seed < seeds.size(); i_seed++) {

		const nps::VtpSeed &seed = *seeds[i_seed];
		if (usedVtpIndices.count(i_seed)) {
			continue;
		}
		for (int i_reco = 0; i_reco < reco_clusters.size(); i_reco++) {
			if (usedRecoIndices.count(i_reco)) {
				continue;
			}
			auto match = isMatched(reco_clusters[i_reco], seed, m_de_thr(), m_tmin(), m_tmax());
			if (match) {
				usedRecoIndices.insert(i_reco);
				usedVtpIndices.insert(i_seed);
				auto matchedCluster = new nps::Cluster(std::move(reco_clusters[i_reco]));
				matchedCluster->setClusterIndex(i_reco);
				m_clusters().push_back(matchedCluster);
				break;
			}
		}
	}
}

void VtpClusterFactory::processRawWaveform(
	const std::vector<double> &waveform, int channel, std::vector<fAdcHit> &hits
) {

	const auto &cfg = m_service_fadc().getConfig(); // get the latest config in case it was updated at runtime

	auto clk = cfg.clock_cycles;
	auto dt = cfg.time_interval;

	auto ped = cfg.ped.at(channel);
	auto thr = cfg.thr.at(channel) + ped;
	auto gain = cfg.gain.at(channel);
	auto nsa = cfg.nsa.at(channel);
	auto nsb = cfg.nsb.at(channel);

	auto pulses = findPulses(waveform, thr, clk);
	for (const auto &p : pulses) {
		// integration
		int begin = std::max(0, p - nsb);
		int end = std::min((int)waveform.size() - 1, p + nsa - 1);
		int nsamples = end - begin + 1;
		auto raw_sum = std::accumulate(waveform.begin() + begin, waveform.begin() + end + 1, 0.0);

		// pedestal subtraction
		auto ped_sub = static_cast<int>(ped * nsamples + (0.001 * nsamples));
		auto charge = static_cast<int>(raw_sum) - ped_sub;

		// apply gain and saturation
		charge = static_cast<int>(charge * gain * 256.0 / 256.0);
		charge = std::max(0, std::min(8191, charge));

		auto hit = nps::fAdcHit(channel, charge, p);
		hits.push_back(hit);
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

std::vector<nps::Cluster> VtpClusterFactory::selectGridCandidate(const std::vector<nps::fAdcHit> &fadc_hits) {

	auto vtp_cfg = m_service_vtp().getConfig();
	auto fadc_cfg = m_service_fadc().getConfig();

	std::vector<nps::Cluster> candidates;

	for (int i = 0; i < fadc_hits.size(); i++) {
		nps::Cluster clus;

		// add cluster seed
		auto ch_seed = fadc_hits[i].getChannel();
		auto t_seed = fadc_hits[i].getTime();
		auto e_seed = fadc_hits[i].getEnergy();
		clus.addHit(ch_seed, e_seed, t_seed);

		auto clus_dt = vtp_cfg.cluster_hit_dt[ch_seed] / fadc_cfg.time_interval; // in unit of time bucket

		// add neighboring blocks in 3x3 grid around the seed
		for (int j = 0; j < fadc_hits.size(); j++) {
			if (i == j) {
				continue;
			}
			auto ch = fadc_hits[j].getChannel();
			auto e = fadc_hits[j].getEnergy();
			auto t = fadc_hits[j].getTime();

			if (std::abs(t - t_seed) > clus_dt) {
				continue;
			}

			if (m_service_geometry().isInsideGrid(ch_seed, ch, 3)) {
				clus.addHit(ch, e, t);
			}
		}
		candidates.push_back(clus);
	}
	return candidates;
}

bool VtpClusterFactory::isTriggered(const nps::Cluster &clus) {
	const auto &cfg = m_service_vtp().getConfig();

	auto channels = clus.getChannels();
	auto energies = clus.getEnergies();
	auto times = clus.getTimes();
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

	// seed energy threshold
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

bool VtpClusterFactory::isMatched(
	const nps::Cluster &clus, const nps::VtpSeed &seed, double de_thr, double tmin, double tmax
) {
	auto vtp_ch = seed.getChannel(); // vtp seed channel
	auto vtp_e = seed.getEnergy();	 // vtp cluster energy (total)
	auto vtp_time = seed.getTime();	 // vtp seed time
	auto vtp_size = seed.getSize();	 // vtp cluster size

	auto reco_size = clus.getChannels().size();							// reco cluster size
	auto reco_ch = clus.getChannels()[0];								// reco seed channel
	auto reco_time = clus.getTimes()[0];								// reco seed time
	auto reco_es = clus.getEnergies();									// reco cluster energy
	auto reco_e = std::accumulate(reco_es.begin(), reco_es.end(), 0.0); // total energy of reco cluster

	bool match = (vtp_ch == reco_ch);			  // same seed channel
	match &= (vtp_time == reco_time);			  // same rise time
	match &= (vtp_size == reco_size);			  // same cluster size
	match &= (std::abs(vtp_e - reco_e) < de_thr); // energy difference within threshold
	// time window requirement (incorrect integration near edge due to readout window)
	match &= (vtp_time >= tmin) && (vtp_time <= tmax);
	return match;
}
} // namespace nps::clustering
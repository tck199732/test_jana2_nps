

#include "OcInferenceFactory.hpp"

namespace nps::clustering {

void OcInferenceFactory::Configure() {
	if (m_beta_thres() < 0.0 || m_beta_thres() > 1.0) {
		throw std::runtime_error(
			"Invalid beta_thres parameter: " + std::to_string(m_beta_thres()) + ". Must be in [0, 1]."
		);
	}
	if (m_dist_thres() < 0.0) {
		throw std::runtime_error(
			"Invalid dist_thres parameter: " + std::to_string(m_dist_thres()) + ". Must be non-negative."
		);
	}
}

void OcInferenceFactory::ChangeRun(int32_t run_number) {}

void OcInferenceFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	if (m_oc_heads().empty()) {
		return;
	}

	std::unordered_map<uint64_t, std::vector<nps::oc_head>> event_oc_map;
	for (const auto &oc_head : m_oc_heads()) {
		event_oc_map[oc_head->event_index].push_back(*oc_head);
	}

	// per event inference, multi-graph inference is not supported yet
	for (const auto &[ev_index, oc_outputs] : event_oc_map) {
		Inference(oc_outputs);
	}
}

void OcInferenceFactory::Inference(const std::vector<nps::oc_head> &oc_heads) {
	if (m_use_dummy()) {
		InferenceDummy();
	} else {
		InferenceOc(oc_heads);
	}
}

void OcInferenceFactory::InferenceDummy() {

	constexpr size_t n_clusters = 100;
	constexpr size_t n_hits_per_cluster = 8;
	constexpr size_t work_iterations = 200;

	double acc = 0.0;
	// heavy dummy work
	for (size_t j = 0; j < work_iterations * n_clusters; ++j) {
		auto x = j * 0.001;
		acc += std::sin(x) * std::cos(x);
		acc += std::tanh(acc * 0.0001);
	}

	for (size_t i = 0; i < n_clusters; ++i) {
		nps::cluster clus;
		for (size_t h = 0; h < n_hits_per_cluster; ++h) {
			auto energy = std::abs(acc) * (h + 1) * 0.1; // dummy energy
			auto time = std::abs(acc) * (h + 1) * 0.01;	 // dummy time
			clus.channels.push_back(static_cast<int>(i * n_hits_per_cluster + h));
			clus.hit_indices.push_back(static_cast<int>(i * n_hits_per_cluster + h));
			clus.energies.push_back(energy);
			clus.times.push_back(static_cast<int>(time));
		}
		m_clusters().push_back(new nps::cluster(std::move(clus)));
	}
	thread_local volatile double sink = 0.0;
	sink += acc;
}

void OcInferenceFactory::InferenceOc(const std::vector<nps::oc_head> &oc_heads) {

	std::unordered_map<int, nps::cluster> clusters;

	for (size_t i = 0; i < oc_heads.size(); ++i) {

		auto ev = oc_heads[i].event_index;
		auto beta = oc_heads[i].beta;
		auto ch = oc_heads[i].channel;

		if (beta > m_beta_thres()) {
			nps::cluster clus;
			double e = 0.0; // currently, the model does not predict energy.
			double t = 0.0; // currently, the model does not predict pulse time.
			clus.channels.push_back(ch);
			clus.hit_indices.push_back(i);
			clus.energies.push_back(e);
			clus.times.push_back(t);
			clusters.emplace(i, std::move(clus));
		}
	}

	std::vector<std::vector<double>> cdist(oc_heads.size(), std::vector<double>(clusters.size(), 1e9));
	for (size_t i = 0; i < oc_heads.size(); ++i) {
		auto x_c = oc_heads[i].x_c;
		for (auto &[j, seed] : clusters) {
			auto seed_x_c = oc_heads[j].x_c;
			double dist = std::sqrt(std::pow(x_c[0] - seed_x_c[0], 2) + std::pow(x_c[1] - seed_x_c[1], 2));
			cdist[i][j] = dist;
		}
	}

	for (size_t i = 0; i < oc_heads.size(); ++i) {
		if (clusters.count(i) != 0) {
			continue;
		}

		// find the closest seed
		double min_dist = 1e9;
		int min_j = -1;
		for (auto &[j, seed] : clusters) {
			if (cdist[i][j] < min_dist) {
				min_dist = cdist[i][j];
				min_j = j;
			}
		}

		if (min_dist < m_dist_thres()) {
			auto ch = oc_heads[i].channel;
			clusters[min_j].channels.push_back(ch);
			clusters[min_j].hit_indices.push_back(i);
			clusters[min_j].energies.push_back(0.0); // currently, the model does not predict energy.
			clusters[min_j].times.push_back(0);		 // currently, the model does not predict pulse time.
		}
	}

	for (auto &[j, clus] : clusters) {
		m_clusters().push_back(new nps::cluster(std::move(clus)));
	}
}

void OcInferenceFactory::Describe() const {}
} // namespace nps::clustering
#include "AiVtpInferenceFactory.hpp"

namespace nps::clustering {

void AiVtpInferenceFactory::Configure() {
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

void AiVtpInferenceFactory::ChangeRun(int32_t run_number) {}

void AiVtpInferenceFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	if (m_oc_outputs().empty()) {
		return;
	}

	// first separate the oc outputs into differnet events and perform inference
	std::unordered_map<uint64_t, std::vector<clustering::ObjectCondensationOutput>> event_oc_map;
	for (const auto &oc_output : m_oc_outputs()) {
		uint64_t ev_index = oc_output->getEventIndex();
		event_oc_map[ev_index].push_back(*oc_output);
	}

	for (const auto &[ev_index, oc_outputs] : event_oc_map) {
		Inference(oc_outputs);
	}
}

void AiVtpInferenceFactory::Inference(const std::vector<nps::clustering::ObjectCondensationOutput> &oc_outputs) {
	if (m_use_dummy()) {
		InferenceDummy();
	} else {
		InferenceOc(oc_outputs);
	}
}

void AiVtpInferenceFactory::InferenceDummy() {

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
        auto clus = new nps::Cluster();
        clus->setEventIndex(0);
        clus->setClusterIndex(i);
        for (size_t h = 0; h < n_hits_per_cluster; ++h) {
            auto energy = std::abs(acc) * (h + 1) * 0.1; // dummy energy
            auto time = std::abs(acc) * (h + 1) * 0.01; // dummy time
            clus->addHit(static_cast<int>(i * n_hits_per_cluster + h), energy, time);
        }
        m_clusters().push_back(clus);
    }
    thread_local volatile double sink = 0.0;
    sink += acc;
}


void AiVtpInferenceFactory::InferenceOc(const std::vector<nps::clustering::ObjectCondensationOutput> &oc_outputs) {

	std::unordered_map<int, nps::Cluster *> clusters;

	for (size_t i = 0; i < oc_outputs.size(); ++i) {

		auto ev = oc_outputs[i].getEventIndex();
		auto beta = oc_outputs[i].getBeta();
		auto ch = oc_outputs[i].getChannel();

		if (beta > m_beta_thres()) {
			auto clus = new nps::Cluster();
			double e = 0.0; // currently, the model does not predict energy.
			double t = 0.0; // currently, the model does not predict pulse time.
			clus->addHit(ch, e, t);
			clus->setEventIndex(ev);
			clusters.emplace(i, clus);
		}
	}

	std::vector<std::vector<double>> cdist(oc_outputs.size(), std::vector<double>(clusters.size(), 1e9));
	for (size_t i = 0; i < oc_outputs.size(); ++i) {
		auto x_c = oc_outputs[i].getXc();
		for (auto &[j, seed] : clusters) {
			auto seed_x_c = oc_outputs[j].getXc();
			double dist = std::sqrt(std::pow(x_c[0] - seed_x_c[0], 2) + std::pow(x_c[1] - seed_x_c[1], 2));
			cdist[i][j] = dist;
		}
	}

	for (size_t i = 0; i < oc_outputs.size(); ++i) {
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
			auto ch = oc_outputs[i].getChannel();
			clusters[min_j]->addHit(ch, 0.0, 0.0);
		}
	}

	for (size_t j = 0; j < clusters.size(); ++j) {
		clusters[j]->setClusterIndex(j);
		m_clusters().push_back(clusters[j]);
	}
}

void AiVtpInferenceFactory::Describe() const {}
} // namespace nps::clustering
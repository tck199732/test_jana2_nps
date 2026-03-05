
#include "Hit.h"
#include "JFactory_VTPClusterFactory_VTPCluster.h"
#include "NPS.hh"
#include "ReplaySource.h"
#include "VTPClusterFactory.h"
#include "VTPClusterSeed.h"

#include <JANA/JEventProcessor.h>
#include <JANA/Services/JServiceLocator.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>

class RecoClusterVTPProcessor : public JEventProcessor {
private:
	std::unique_ptr<TH1D> m_h1_reco_clus_size;
	std::unique_ptr<TH1D> m_h1_reco_clus_energy;
	std::unique_ptr<TH2D> m_h2_reco_clus_pos;
	std::unique_ptr<TH1D> m_h1_reco_clus_time;

	std::string m_output_file = "output.root";
	std::mutex m_mutex;

public:
	RecoClusterVTPProcessor() {
		SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
	}

	void Init() override {
		auto app = GetApplication();

		m_h1_reco_clus_size = std::make_unique<TH1D>("hRecoClusSize", "", 20, 0, 20);
		m_h1_reco_clus_energy = std::make_unique<TH1D>("hRecoClusEnergy", "", 800, 0, 4000);
		m_h2_reco_clus_pos =
			std::make_unique<TH2D>("hRecoClusPos", "", NPS::NCOLS, 0, NPS::NCOLS, NPS::NROWS, 0, NPS::NROWS);
		m_h1_reco_clus_time = std::make_unique<TH1D>("hRecoClusTime", "", NPS::NTIME, 0, NPS::NTIME);

		/// Set parameters to control which JFactories you use
		app->SetDefaultParameter("output_file", m_output_file);
	}

	void Process(const std::shared_ptr<const JEvent> &event) override {
		/// Do everything we can in parallel
		/// Warning: We are only allowed to use local variables and `event` here

		auto hits = event->Get<Hit>();
		auto vtp_seeds = event->Get<VTPClusterSeed>();
		auto vtp_clusters = event->Get<VTPClusterFactory>("VTPCluster");

		/// Inside the lock, update any shared state, e.g. histograms
		std::lock_guard<std::mutex> lock(m_mutex);

		auto sum = [](const std::vector<double> &vec) { return std::accumulate(vec.begin(), vec.end(), 0.0); };
		for (auto clus : vtp_clusters) {

			auto size = clus->block_ids.size();

			auto e_tot = sum(clus->energies);
			auto t = clus->pulse_times[0];

			m_h1_reco_clus_size->Fill(size);
			m_h1_reco_clus_energy->Fill(e_tot);
			m_h1_reco_clus_time->Fill(t);

			for (int i = 0; i < size; i++) {
				auto col = clus->x_coords[i];
				auto row = clus->y_coords[i];
				m_h2_reco_clus_pos->Fill(col, row);
			}
		}
	}

	void Finish() override {
		auto out_file = std::make_unique<TFile>(m_output_file.c_str(), "RECREATE");
		out_file->cd();
		m_h1_reco_clus_size->Write();
		m_h1_reco_clus_energy->Write();
		m_h2_reco_clus_pos->Write();
		m_h1_reco_clus_time->Write();
		out_file->Close();
	}
};

extern "C" {
void InitPlugin(JApplication *app) {
	InitJANAPlugin(app);
	app->ProvideService(std::make_shared<NPSGeometryService>());
	app->Add(new RecoClusterVTPProcessor);
	app->Add(new JEventSourceGeneratorT<ReplaySource>);
	app->Add(new JFactoryGeneratorT<JFactory_VTPClusterFactory_VTPCluster>);
}
}
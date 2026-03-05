
#include "JFactory_VTPClusterFactory_VTPCluster.h"

JFactory_VTPClusterFactory_VTPCluster::JFactory_VTPClusterFactory_VTPCluster() { SetTag("VTPCluster"); }

void JFactory_VTPClusterFactory_VTPCluster::Init() {
	auto app = GetApplication();

	/// Set any factory flags
	// SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);

	app->SetDefaultParameter("vme_config_file", m_vmeConfigFile, "Path to fADC250 configuration file");
	app->SetDefaultParameter("vtp_config_file", m_vtpConfigFile, "Path to VTP configuration file");

	m_geometry = app->GetService<NPSGeometryService>()->getGeometry();
	m_fadcDevice = std::make_unique<fADC250>(NPS::NBLOCKS, m_vmeConfigFile);
	m_vtpDevice = std::make_unique<VTP>(NPS::NBLOCKS, NPS::NTIME, NPS::DELTA_T, m_vtpConfigFile);
}

//------------------------
// ChangeRun
//------------------------
void JFactory_VTPClusterFactory_VTPCluster::ChangeRun(const std::shared_ptr<const JEvent> &event) {
	/// This is automatically run before Process, when a new run number is seen
	/// Usually we update our calibration constants by asking a JService
	/// to give us the latest data for this run number

	auto run_nr = event->GetRunNumber();

	// update config file and reload the fADC250 and VTP devices if necessary
	m_fadcDevice->resetConfig();
	m_vtpDevice->resetConfig();
	m_fadcDevice->loadConfig(m_vmeConfigFile);
	m_vtpDevice->loadConfig(m_vtpConfigFile);
}

//------------------------
// Process
//------------------------
void JFactory_VTPClusterFactory_VTPCluster::Process(const std::shared_ptr<const JEvent> &event) {

	/// JFactories are local to a thread, so we are free to access and modify
	/// member variables here. However, be aware that events are _scattered_ to
	/// different JFactory instances, not _broadcast_: this means that JFactory
	/// instances only see _some_ of the events.

	/// Acquire inputs (This may recursively call other JFactories)
	auto hits = event->Get<Hit>();
	auto vtp_seeds = event->Get<VTPClusterSeed>();

	/// Publish outputs
	std::vector<VTPClusterFactory *> results;

	for (auto hit : hits) {
		int block_id = hit->channel;
		auto waveform = hit->waveform;
		m_fadcDevice->processRawWaveform(waveform, block_id, 0);
	}

	auto fadc250_evt = m_fadcDevice->getEvent();
	auto nhits = fadc250_evt.nhits;

	for (int i = 0; i < nhits; i++) {

		auto seedChannel = fadc250_evt.channels[i];
		auto seedTime = fadc250_evt.times[i];
		auto seedEnergy = fadc250_evt.energies[i];

		std::vector<int> gridTimes;		  // [num_blocks_in_grid]
		std::vector<double> gridEnergies; // [num_blocks_in_grid]
		std::vector<int> gridChannels;	  // [num_blocks_in_grid]

		for (int j = 0; j < nhits; j++) {
			if (i == j) {
				continue;
			}
			auto ch = fadc250_evt.channels[j];
			auto e = fadc250_evt.energies[j];
			auto t = fadc250_evt.times[j];
			if (m_geometry->isInsideGrid(seedChannel, ch, 3)) {
				gridTimes.push_back(t);
				gridEnergies.push_back(e);
				gridChannels.push_back(ch);
			}
		}
		m_vtpDevice->process(seedChannel, seedTime, seedEnergy, gridChannels, gridTimes, gridEnergies);
	}

	auto reco_event = m_vtpDevice->getEvent();

	std::unordered_set<int> usedRecoIndices;
	std::unordered_set<int> usedVtpIndices;

	auto sum = [](const std::vector<double> &vec) { return std::accumulate(vec.begin(), vec.end(), 0.0); };

	for (int iclus = 0; iclus < vtp_seeds.size(); iclus++) {

		if (usedVtpIndices.count(iclus)) {
			continue;
		}

		auto vtp_col = vtp_seeds[iclus]->col; // vtp seed column
		auto vtp_row = vtp_seeds[iclus]->row; // vtp seed row

		auto vtp_e = vtp_seeds[iclus]->E;		// vtp cluster energy
		auto vtp_time = vtp_seeds[iclus]->t;	// vtp seed time
		auto vtp_size = vtp_seeds[iclus]->size; // vtp cluster size

		for (int i_reco = 0; i_reco < reco_event.nseeds; i_reco++) {

			if (usedRecoIndices.count(i_reco)) {
				continue;
			}

			auto reco_size = reco_event.clus_sizes[i_reco]; // reco cluster size
			if (reco_size == 0) {
				continue;
			}
			assert(reco_event.channels[i_reco].size() == reco_size);
			assert(reco_event.energies[i_reco].size() == reco_size);
			assert(reco_event.times[i_reco].size() == reco_size);

			auto reco_ch = reco_event.channels[i_reco][0];	// reco seed channel
			auto reco_e = sum(reco_event.energies[i_reco]); // reco cluster energy

			auto reco_time = reco_event.times[i_reco][0]; // reco seed time

			auto [reco_row, reco_col] = m_geometry->getColRowFromBlock(reco_ch);

			bool match = (vtp_col == reco_col);
			match &= (vtp_row == reco_row);
			match &= (vtp_time == reco_time);
			match &= (vtp_size == reco_size);
			match &= (std::abs(vtp_e - reco_e) < 5);
			match &= (vtp_time >= 50) && (vtp_time <= 370);

			if (match) {
				usedRecoIndices.insert(i_reco);
				usedVtpIndices.insert(iclus);
				auto clus = new VTPClusterFactory();

				for (int j = 0; j < reco_size; j++) {

					auto ch = reco_event.channels[i_reco][j];
					auto [row, col] = m_geometry->getColRowFromBlock(ch);
					auto time = reco_event.times[i_reco][j];
					auto energy = reco_event.energies[i_reco][j];

					clus->AddHit(ch, col, row, time, energy);
				}
				results.push_back(clus);
			}
		}
	}

	m_vtpDevice->resetEvent();
	m_fadcDevice->resetEvent();

	Set(results);
}

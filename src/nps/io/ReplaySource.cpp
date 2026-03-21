#include "ReplaySource.hpp"

namespace nps::io {

ReplaySource::ReplaySource() : JEventSource() {
	SetTypeName(NAME_OF_THIS); // Provide JANA with class name
	SetCallbackStyle(CallbackStyle::ExpertMode);
}

void ReplaySource::Init() {}

void ReplaySource::Open() {
	std::string resource_name = GetResourceName();
	m_chain = std::make_unique<TChain>(m_tree().c_str());
	m_chain->Add(resource_name.c_str());
	setBranchAddresses(m_chain.get(), m_buffer);
}

void ReplaySource::Close() { m_chain->Reset(); }

JEventSource::Result ReplaySource::Emit(JEvent &event) {

	static size_t current_event_number = 0;
	if (current_event_number >= m_chain->GetEntries() || current_event_number >= m_max_events()) {
		return Result::FailureFinished;
	}

	event.SetEventNumber(current_event_number++);
	event.SetRunNumber(m_run_number());
	m_chain->GetEntry(event.GetEventNumber());

	std::vector<nps::RawHit *> hits;
	std::vector<nps::VtpSeed *> seeds;

	std::vector<std::vector<double>> signals; // [nblocks][ntime]
	std::vector<int> blocks;				  // [nblocks]

	auto signalFlag = unpackWaveform(
		m_buffer.Ndata_NPS_cal_fly_adcSampWaveform, MAX_BLOCKS, m_buffer.NPS_cal_fly_adcSampWaveform, blocks, signals
	);

	for (size_t i = 0; i < blocks.size(); i++) {
		auto hit = new nps::RawHit(blocks[i], std::move(signals[i]));
		hits.push_back(hit);
	}

	for (int iclus = 0; iclus < m_buffer.Ndata_NPS_cal_vtpClusX; iclus++) {
		auto col = m_buffer.NPS_cal_vtpClusX[iclus];
		auto row = m_buffer.NPS_cal_vtpClusY[iclus];
		auto ch = m_nps_geometry_service().getBlockFromColRow(col, row);
		auto e = m_buffer.NPS_cal_vtpClusE[iclus];
		auto t = m_buffer.NPS_cal_vtpClusTime[iclus];
		auto size = m_buffer.NPS_cal_vtpClusSize[iclus];

		auto seed = new nps::VtpSeed(ch, size, t, e);
		seeds.push_back(seed);
	}

	event.Insert(hits, "RawHits");
	event.Insert(seeds, "VtpSeeds");

	return Result::Success;
}

std::string ReplaySource::GetDescription() {

	std::stringstream ss;
	ss << "ReplaySource is a JEventSource that reads events from a ROOT file containing NPS data. It uses a TChain "
		  "to "
		  "read the data and fills Hit objects with the waveform information. The source can be configured with "
		  "parameters such as the maximum event emission frequency and the tree name to read from. It is designed "
		  "to "
		  "be used in a JANA application for processing NPS data."
	   << std::endl;
	ss << "Parameters:" << std::endl;
	ss << "  replay_source:max_events (int): Maximum number of events to emit from the source. Default is INT_MAX."
	   << std::endl;
	ss << "  replay_source:run_number (int): Run number to assign to events from the source. Default is 0."
	   << std::endl;
	ss << "  replay_source:tree (string): Tree name to read from in the ROOT file. Default is 'T'." << std::endl;
	ss << "Usage:" << std::endl;
	ss << "  To use this source, provide a ROOT file as a command line argument when running the JANA application. "
		  "The "
		  "source will automatically read events from the file and insert Hit objects into the JEvent for "
		  "processing "
		  "by other JANA components."
	   << std::endl;
	return ss.str();
}

int unpackWaveform(
	int NSampWaveForm, int max_blocks, std::span<const double> SampWaveForm, std::vector<int> &blocks,
	std::vector<std::vector<double>> &signals
) {
	if (NSampWaveForm > MAX_DATA) {
		std::cerr << "Error: NSampWaveForm (" << NSampWaveForm << ") exceeds the size of the input buffer (" << MAX_DATA
				  << "). Cannot read signal.\n";
		return 1;
	}

	signals.clear();
	blocks.clear();
	blocks.reserve(max_blocks);
	signals.reserve(max_blocks);

	std::unordered_set<int> block_seen;
	block_seen.reserve(max_blocks);

	int ns = 0;

	while (ns < NSampWaveForm) {
		int bloc = static_cast<int>(SampWaveForm[ns++]);
		int nsamp = static_cast<int>(SampWaveForm[ns++]);

		if (ns + nsamp > NSampWaveForm) {
			std::cerr << "Warning: not enough samples for block " << bloc << " (expected " << nsamp << ", available "
					  << (NSampWaveForm - ns) << "). Stopping unpackWaveform.\n";
			break;
		}

		if (bloc == 2000)
			bloc = 1080;
		else if (bloc == 2001)
			bloc = 1081;

		if (bloc < 0 || bloc >= max_blocks || block_seen.count(bloc)) {
			ns += nsamp;
			continue;
		}

		block_seen.insert(bloc);
		blocks.emplace_back(bloc);

		std::vector<double> sig;
		sig.reserve(nsamp);
		for (int it = 0; it < nsamp; ++it)
			sig.emplace_back(SampWaveForm[ns++]);

		signals.emplace_back(std::move(sig));
	}

	if (signals.size() != blocks.size()) {
		std::cerr << "Error: Mismatch between number of blocks (" << blocks.size() << ") and number of signals ("
				  << signals.size() << ").\n";
		return 1;
	}
	return 0;
}

void setBranchAddresses(TChain *chain, npsBranches &buffer) {
	chain->SetBranchAddress("g.evtime", &buffer.g_evtime);
	chain->SetBranchAddress("Ndata.NPS.cal.clusE", &buffer.Ndata_NPS_cal_clusE);
	chain->SetBranchAddress("Ndata.NPS.cal.clusSize", &buffer.Ndata_NPS_cal_clusSize);
	chain->SetBranchAddress("Ndata.NPS.cal.clusT", &buffer.Ndata_NPS_cal_clusT);
	chain->SetBranchAddress("Ndata.NPS.cal.clusX", &buffer.Ndata_NPS_cal_clusX);
	chain->SetBranchAddress("Ndata.NPS.cal.clusY", &buffer.Ndata_NPS_cal_clusY);
	chain->SetBranchAddress("Ndata.NPS.cal.clusZ", &buffer.Ndata_NPS_cal_clusZ);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcCounter", &buffer.Ndata_NPS_cal_fly_adcCounter);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcErrorFlag", &buffer.Ndata_NPS_cal_fly_adcErrorFlag);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPed", &buffer.Ndata_NPS_cal_fly_adcPed);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPedRaw", &buffer.Ndata_NPS_cal_fly_adcPedRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPulseAmp", &buffer.Ndata_NPS_cal_fly_adcPulseAmp);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPulseAmpRaw", &buffer.Ndata_NPS_cal_fly_adcPulseAmpRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPulseInt", &buffer.Ndata_NPS_cal_fly_adcPulseInt);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPulseIntRaw", &buffer.Ndata_NPS_cal_fly_adcPulseIntRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPulseTime", &buffer.Ndata_NPS_cal_fly_adcPulseTime);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcPulseTimeRaw", &buffer.Ndata_NPS_cal_fly_adcPulseTimeRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPed", &buffer.Ndata_NPS_cal_fly_adcSampPed);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPedRaw", &buffer.Ndata_NPS_cal_fly_adcSampPedRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseAmp", &buffer.Ndata_NPS_cal_fly_adcSampPulseAmp);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseAmpRaw", &buffer.Ndata_NPS_cal_fly_adcSampPulseAmpRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseInt", &buffer.Ndata_NPS_cal_fly_adcSampPulseInt);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseIntRaw", &buffer.Ndata_NPS_cal_fly_adcSampPulseIntRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseTime", &buffer.Ndata_NPS_cal_fly_adcSampPulseTime);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseTimeRaw", &buffer.Ndata_NPS_cal_fly_adcSampPulseTimeRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.adcSampWaveform", &buffer.Ndata_NPS_cal_fly_adcSampWaveform);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.block_clusterID", &buffer.Ndata_NPS_cal_fly_block_clusterID);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.e", &buffer.Ndata_NPS_cal_fly_e);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcMult", &buffer.Ndata_NPS_cal_fly_goodAdcMult);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcPed", &buffer.Ndata_NPS_cal_fly_goodAdcPed);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcPulseAmp", &buffer.Ndata_NPS_cal_fly_goodAdcPulseAmp);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcPulseInt", &buffer.Ndata_NPS_cal_fly_goodAdcPulseInt);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcPulseIntRaw", &buffer.Ndata_NPS_cal_fly_goodAdcPulseIntRaw);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcPulseTime", &buffer.Ndata_NPS_cal_fly_goodAdcPulseTime);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.goodAdcTdcDiffTime", &buffer.Ndata_NPS_cal_fly_goodAdcTdcDiffTime);
	chain->SetBranchAddress("Ndata.NPS.cal.fly.numGoodAdcHits", &buffer.Ndata_NPS_cal_fly_numGoodAdcHits);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.mult", &buffer.Ndata_NPS_cal_trk_mult);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.p", &buffer.Ndata_NPS_cal_trk_p);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.px", &buffer.Ndata_NPS_cal_trk_px);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.py", &buffer.Ndata_NPS_cal_trk_py);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.pz", &buffer.Ndata_NPS_cal_trk_pz);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.x", &buffer.Ndata_NPS_cal_trk_x);
	chain->SetBranchAddress("Ndata.NPS.cal.trk.y", &buffer.Ndata_NPS_cal_trk_y);
	chain->SetBranchAddress("Ndata.NPS.cal.vldColumn", &buffer.Ndata_NPS_cal_vldColumn);
	chain->SetBranchAddress("Ndata.NPS.cal.vldHiChannelMask", &buffer.Ndata_NPS_cal_vldHiChannelMask);
	chain->SetBranchAddress("Ndata.NPS.cal.vldLoChannelMask", &buffer.Ndata_NPS_cal_vldLoChannelMask);
	chain->SetBranchAddress("Ndata.NPS.cal.vldPMT", &buffer.Ndata_NPS_cal_vldPMT);
	chain->SetBranchAddress("Ndata.NPS.cal.vldRow", &buffer.Ndata_NPS_cal_vldRow);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &buffer.Ndata_NPS_cal_vtpClusE);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpClusSize", &buffer.Ndata_NPS_cal_vtpClusSize);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpClusTime", &buffer.Ndata_NPS_cal_vtpClusTime);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpClusX", &buffer.Ndata_NPS_cal_vtpClusX);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpClusY", &buffer.Ndata_NPS_cal_vtpClusY);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigCrate", &buffer.Ndata_NPS_cal_vtpTrigCrate);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigTime", &buffer.Ndata_NPS_cal_vtpTrigTime);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigType0", &buffer.Ndata_NPS_cal_vtpTrigType0);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigType1", &buffer.Ndata_NPS_cal_vtpTrigType1);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigType2", &buffer.Ndata_NPS_cal_vtpTrigType2);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigType3", &buffer.Ndata_NPS_cal_vtpTrigType3);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigType4", &buffer.Ndata_NPS_cal_vtpTrigType4);
	chain->SetBranchAddress("Ndata.NPS.cal.vtpTrigType5", &buffer.Ndata_NPS_cal_vtpTrigType5);

	// NPS branches
	chain->SetBranchAddress("NPS.cal.clusE", &buffer.NPS_cal_clusE[0]);
	chain->SetBranchAddress("NPS.cal.clusSize", &buffer.NPS_cal_clusSize[0]);
	chain->SetBranchAddress("NPS.cal.clusT", &buffer.NPS_cal_clusT[0]);
	chain->SetBranchAddress("NPS.cal.clusX", &buffer.NPS_cal_clusX[0]);
	chain->SetBranchAddress("NPS.cal.clusY", &buffer.NPS_cal_clusY[0]);
	chain->SetBranchAddress("NPS.cal.clusZ", &buffer.NPS_cal_clusZ[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcCounter", &buffer.NPS_cal_fly_adcCounter[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcErrorFlag", &buffer.NPS_cal_fly_adcErrorFlag[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPed", &buffer.NPS_cal_fly_adcPed[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPedRaw", &buffer.NPS_cal_fly_adcPedRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPulseAmp", &buffer.NPS_cal_fly_adcPulseAmp[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPulseAmpRaw", &buffer.NPS_cal_fly_adcPulseAmpRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPulseInt", &buffer.NPS_cal_fly_adcPulseInt[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPulseIntRaw", &buffer.NPS_cal_fly_adcPulseIntRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPulseTime", &buffer.NPS_cal_fly_adcPulseTime[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcPulseTimeRaw", &buffer.NPS_cal_fly_adcPulseTimeRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPed", &buffer.NPS_cal_fly_adcSampPed[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPedRaw", &buffer.NPS_cal_fly_adcSampPedRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPulseAmp", &buffer.NPS_cal_fly_adcSampPulseAmp[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPulseAmpRaw", &buffer.NPS_cal_fly_adcSampPulseAmpRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPulseInt", &buffer.NPS_cal_fly_adcSampPulseInt[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPulseIntRaw", &buffer.NPS_cal_fly_adcSampPulseIntRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPulseTime", &buffer.NPS_cal_fly_adcSampPulseTime[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampPulseTimeRaw", &buffer.NPS_cal_fly_adcSampPulseTimeRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.adcSampWaveform", &buffer.NPS_cal_fly_adcSampWaveform[0]);
	chain->SetBranchAddress("NPS.cal.fly.block_clusterID", &buffer.NPS_cal_fly_block_clusterID[0]);
	chain->SetBranchAddress("NPS.cal.fly.e", &buffer.NPS_cal_fly_e[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcMult", &buffer.NPS_cal_fly_goodAdcMult[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcPed", &buffer.NPS_cal_fly_goodAdcPed[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcPulseAmp", &buffer.NPS_cal_fly_goodAdcPulseAmp[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcPulseInt", &buffer.NPS_cal_fly_goodAdcPulseInt[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcPulseIntRaw", &buffer.NPS_cal_fly_goodAdcPulseIntRaw[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcPulseTime", &buffer.NPS_cal_fly_goodAdcPulseTime[0]);
	chain->SetBranchAddress("NPS.cal.fly.goodAdcTdcDiffTime", &buffer.NPS_cal_fly_goodAdcTdcDiffTime[0]);
	chain->SetBranchAddress("NPS.cal.fly.numGoodAdcHits", &buffer.NPS_cal_fly_numGoodAdcHits[0]);
	chain->SetBranchAddress("NPS.cal.trk.mult", &buffer.NPS_cal_trk_mult[0]);
	chain->SetBranchAddress("NPS.cal.trk.p", &buffer.NPS_cal_trk_p[0]);
	chain->SetBranchAddress("NPS.cal.trk.px", &buffer.NPS_cal_trk_px[0]);
	chain->SetBranchAddress("NPS.cal.trk.py", &buffer.NPS_cal_trk_py[0]);
	chain->SetBranchAddress("NPS.cal.trk.pz", &buffer.NPS_cal_trk_pz[0]);
	chain->SetBranchAddress("NPS.cal.trk.x", &buffer.NPS_cal_trk_x[0]);
	chain->SetBranchAddress("NPS.cal.trk.y", &buffer.NPS_cal_trk_y[0]);
	chain->SetBranchAddress("NPS.cal.vtpClusE", &buffer.NPS_cal_vtpClusE[0]);
	chain->SetBranchAddress("NPS.cal.vtpClusSize", &buffer.NPS_cal_vtpClusSize[0]);
	chain->SetBranchAddress("NPS.cal.vtpClusTime", &buffer.NPS_cal_vtpClusTime[0]);
	chain->SetBranchAddress("NPS.cal.vtpClusX", &buffer.NPS_cal_vtpClusX[0]);
	chain->SetBranchAddress("NPS.cal.vtpClusY", &buffer.NPS_cal_vtpClusY[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigCrate", &buffer.NPS_cal_vtpTrigCrate[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigTime", &buffer.NPS_cal_vtpTrigTime[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigType0", &buffer.NPS_cal_vtpTrigType0[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigType1", &buffer.NPS_cal_vtpTrigType1[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigType2", &buffer.NPS_cal_vtpTrigType2[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigType3", &buffer.NPS_cal_vtpTrigType3[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigType4", &buffer.NPS_cal_vtpTrigType4[0]);
	chain->SetBranchAddress("NPS.cal.vtpTrigType5", &buffer.NPS_cal_vtpTrigType5[0]);
	chain->SetBranchAddress("NPS.cal.etot", &buffer.NPS_cal_etot);
	chain->SetBranchAddress("NPS.cal.fly.earray", &buffer.NPS_cal_fly_earray);
	chain->SetBranchAddress("NPS.cal.fly.nclust", &buffer.NPS_cal_fly_nclust);
	chain->SetBranchAddress("NPS.cal.fly.ntracks", &buffer.NPS_cal_fly_ntracks);
	chain->SetBranchAddress("NPS.cal.fly.totNumAdcHits", &buffer.NPS_cal_fly_totNumAdcHits);
	chain->SetBranchAddress("NPS.cal.fly.totNumGoodAdcHits", &buffer.NPS_cal_fly_totNumGoodAdcHits);
	chain->SetBranchAddress("NPS.cal.nclust", &buffer.NPS_cal_nclust);
	chain->SetBranchAddress("NPS.cal.nhits", &buffer.NPS_cal_nhits);
}

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::ReplaySource>::CheckOpenable(std::string resource_name) {

	/// CheckOpenable() decides how confident we are that this EventSource can handle this resource.
	///    0.0        -> 'Cannot handle'
	///    (0.0, 1.0] -> 'Can handle, with this confidence level'

	/// To determine confidence level, feel free to open up the file and check for magic bytes or metadata.
	/// Returning a confidence <- {0.0, 1.0} is perfectly OK!

	if (resource_name.size() >= 5 && resource_name.substr(resource_name.size() - 5) == ".root") {
		return 1.0;
	}
	return 0.0;
}

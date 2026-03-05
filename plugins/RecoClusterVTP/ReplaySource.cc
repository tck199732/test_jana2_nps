

#include "ReplaySource.h"
#include "Hit.h"
#include "VTPClusterSeed.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>

#include <VTP.hh>
#include <fADC250.hh>

/// There are two different ways of instantiating JEventSources
/// 1. Creating them manually and registering them with the JApplication
/// 2. Creating a corresponding JEventSourceGenerator and registering that instead
///    If you have a list of files as command line args, JANA will use the JEventSourceGenerator
///    to find the most appropriate JEventSource corresponding to that filename, instantiate and register it.
///    For this to work, the JEventSource constructor has to have the following constructor arguments:

ReplaySource::ReplaySource() : JEventSource() {
	SetTypeName(NAME_OF_THIS); // Provide JANA with class name
	SetCallbackStyle(CallbackStyle::ExpertMode);
}

void ReplaySource::Open() {
	JApplication *app = GetApplication();

	app->SetDefaultParameter("replay_source:tree", m_tree, "Tree name to read from for ReplaySource");
	app->SetDefaultParameter(
		"replay_source:max_events", m_max_events,
		"Maximum number of events to emit from the source. Default is INT_MAX."
	);
	app->SetDefaultParameter(
		"replay_source:run_number", m_run_number, "Run number to assign to events from the source. Default is 0."
	);

	std::string resource_name = GetResourceName();
	m_chain = std::make_unique<TChain>(m_tree.c_str());
	m_chain->Add(resource_name.c_str());
}

void ReplaySource::Close() {

	/// Close is called exactly once when processing ends. This is where you should close your files or sockets.
	/// It is important to do that here instead of in Emit() because we want everything to be cleanly closed
	/// even when JANA is terminated via Ctrl-C or via a timeout.
	m_chain->Reset();
}

JEventSource::Result ReplaySource::Emit(JEvent &event) {
	NPS::npsBranches buffer;
	NPS::setBranchAddresses(m_chain.get(), buffer);

	/// Calls to GetEvent are synchronized with each other, which means they can
	/// read and write state on the JEventSource without causing race conditions.
	/// Configure event and run numbers
	static size_t current_event_number = 0;
	if (current_event_number >= m_chain->GetEntries() || current_event_number >= m_max_events) {
		return Result::FailureFinished;
	}

	event.SetEventNumber(current_event_number++);
	event.SetRunNumber(m_run_number);
	m_chain->GetEntry(event.GetEventNumber());

	std::vector<Hit *> hits;
	std::vector<VTPClusterSeed *> vtp_seeds;

	std::vector<std::vector<double>> signals; // [nblocks][ntime]
	std::vector<int> blocks;				  // [nblocks]
	auto signalFlag =
		NPS::readSignal(buffer.Ndata_NPS_cal_fly_adcSampWaveform, buffer.NPS_cal_fly_adcSampWaveform, blocks, signals);

	assert(signals.size() == blocks.size());

	for (size_t i = 0; i < blocks.size(); i++) {
		int idx = blocks[i];
		auto hit = new Hit(blocks[i], signals[i]);
		hits.push_back(hit);
	}

	for (int iclus = 0; iclus < buffer.Ndata_NPS_cal_vtpClusX; iclus++) {
		auto col = buffer.NPS_cal_vtpClusX[iclus];
		auto row = buffer.NPS_cal_vtpClusY[iclus];
		auto e = buffer.NPS_cal_vtpClusE[iclus];
		auto t = buffer.NPS_cal_vtpClusTime[iclus];
		auto size = buffer.NPS_cal_vtpClusSize[iclus];

		assert(size > 0); // if size is 0, then there is no cluster, so we should skip this entry
		assert(buffer.NPS_cal_vtpClusX.size() == buffer.NPS_cal_vtpClusY.size());
		assert(buffer.NPS_cal_vtpClusX.size() == buffer.NPS_cal_vtpClusE.size());
		assert(buffer.NPS_cal_vtpClusX.size() == buffer.NPS_cal_vtpClusTime.size());
		assert(buffer.NPS_cal_vtpClusX.size() == buffer.NPS_cal_vtpClusSize.size());

		auto seed = new VTPClusterSeed(col, row, size, e, t);
		vtp_seeds.push_back(seed);
	}

	event.Insert(hits);
	event.Insert(vtp_seeds);

	/// If you are streaming events and there are no new events in the message queue,
	/// tell JANA that Emit() was temporarily unsuccessful like this:
	// return Result::FailureTryAgain;

	return Result::Success;
}

std::string ReplaySource::GetDescription() {

	std::stringstream ss;
	ss << "ReplaySource is a JEventSource that reads events from a ROOT file containing NPS data. It uses a TChain to "
		  "read the data and fills Hit objects with the waveform information. The source can be configured with "
		  "parameters such as the maximum event emission frequency and the tree name to read from. It is designed to "
		  "be used in a JANA application for processing NPS data."
	   << std::endl;
	ss << "Parameters:" << std::endl;
	ss << "  replay_source:max_events (int): Maximum number of events to emit from the source. Default is INT_MAX."
	   << std::endl;
	ss << "  replay_source:run_number (int): Run number to assign to events from the source. Default is 0."
	   << std::endl;
	ss << "  replay_source:tree (string): Tree name to read from in the ROOT file. Default is 'T'." << std::endl;
	ss << "Usage:" << std::endl;
	ss << "  To use this source, provide a ROOT file as a command line argument when running the JANA application. The "
		  "source will automatically read events from the file and insert Hit objects into the JEvent for processing "
		  "by other JANA components."
	   << std::endl;
	return ss.str();
}

template <> double JEventSourceGeneratorT<ReplaySource>::CheckOpenable(std::string resource_name) {

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

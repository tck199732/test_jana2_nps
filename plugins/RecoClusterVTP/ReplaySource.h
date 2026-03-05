

#ifndef _ReplaySource_h_
#define _ReplaySource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <NPS.hh>
#include <climits>

class ReplaySource : public JEventSource {

	int m_max_events = INT_MAX; // Maximum number of events to emit, set via parameter
	int m_run_number = 0;		// Run number to assign to emitted events, set via parameter
	std::string m_tree = "T";
	std::unique_ptr<TChain> m_chain;
	// NPS::npsBranches m_nps_buffer;

public:
	ReplaySource();

	virtual ~ReplaySource() = default;

	void Open() override;

	void Close() override;

	Result Emit(JEvent &event) override;

	static std::string GetDescription();
};

template <> double JEventSourceGeneratorT<ReplaySource>::CheckOpenable(std::string);

#endif // _ReplaySource_h_

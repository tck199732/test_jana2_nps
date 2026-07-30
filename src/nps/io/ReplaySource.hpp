#pragma once

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

#include <array>
#include <climits>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geometry/NpsGeometryService.hpp"
// #include "nps/RawHit.hpp"
// #include "nps/VtpSeed.hpp"

#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

#include "TChain.h"

namespace nps::io {

// constants for defining std::array size, actual data sizes are bound by geometry and hardware config
constexpr int MAX_SAMPLES = 250;						// maximum number of time samples in the waveform (1\mu s)
constexpr int MAX_BLOCKS = 1080;						// maximum number of blocks with waveform data
constexpr int MAX_SLOTS = 1104;							// maximum number of slots
constexpr int MAX_DATA = MAX_SLOTS * (MAX_SAMPLES + 2); //(1104 slots fADC for each of 110 timestamp + 2 pmt
constexpr int MAX_CLUSTERS = 256;						// arbitrary limit on the number of clusters found in hcana
constexpr int MAX_VTP_TRIGGERS = 64;					// arbitrary limit on the number of VTP triggers

struct npsBranches {

	// Global branches
	double g_evtime;

	// Ndata.NPS branches
	int Ndata_NPS_cal_clusE;
	int Ndata_NPS_cal_clusSize;
	int Ndata_NPS_cal_clusT;
	int Ndata_NPS_cal_clusX;
	int Ndata_NPS_cal_clusY;
	int Ndata_NPS_cal_clusZ;
	int Ndata_NPS_cal_fly_adcCounter;
	int Ndata_NPS_cal_fly_adcErrorFlag;
	int Ndata_NPS_cal_fly_adcPed;
	int Ndata_NPS_cal_fly_adcPedRaw;
	int Ndata_NPS_cal_fly_adcPulseAmp;
	int Ndata_NPS_cal_fly_adcPulseAmpRaw;
	int Ndata_NPS_cal_fly_adcPulseInt;
	int Ndata_NPS_cal_fly_adcPulseIntRaw;
	int Ndata_NPS_cal_fly_adcPulseTime;
	int Ndata_NPS_cal_fly_adcPulseTimeRaw;
	int Ndata_NPS_cal_fly_adcSampPed;
	int Ndata_NPS_cal_fly_adcSampPedRaw;
	int Ndata_NPS_cal_fly_adcSampPulseAmp;
	int Ndata_NPS_cal_fly_adcSampPulseAmpRaw;
	int Ndata_NPS_cal_fly_adcSampPulseInt;
	int Ndata_NPS_cal_fly_adcSampPulseIntRaw;
	int Ndata_NPS_cal_fly_adcSampPulseTime;
	int Ndata_NPS_cal_fly_adcSampPulseTimeRaw;
	int Ndata_NPS_cal_fly_adcSampWaveform;
	int Ndata_NPS_cal_fly_block_clusterID;
	int Ndata_NPS_cal_fly_e;
	int Ndata_NPS_cal_fly_goodAdcMult;
	int Ndata_NPS_cal_fly_goodAdcPed;
	int Ndata_NPS_cal_fly_goodAdcPulseAmp;
	int Ndata_NPS_cal_fly_goodAdcPulseInt;
	int Ndata_NPS_cal_fly_goodAdcPulseIntRaw;
	int Ndata_NPS_cal_fly_goodAdcPulseTime;
	int Ndata_NPS_cal_fly_goodAdcTdcDiffTime;
	int Ndata_NPS_cal_fly_numGoodAdcHits;
	int Ndata_NPS_cal_trk_mult;
	int Ndata_NPS_cal_trk_p;
	int Ndata_NPS_cal_trk_px;
	int Ndata_NPS_cal_trk_py;
	int Ndata_NPS_cal_trk_pz;
	int Ndata_NPS_cal_trk_x;
	int Ndata_NPS_cal_trk_y;
	int Ndata_NPS_cal_vldColumn;
	int Ndata_NPS_cal_vldHiChannelMask;
	int Ndata_NPS_cal_vldLoChannelMask;
	int Ndata_NPS_cal_vldPMT;
	int Ndata_NPS_cal_vldRow;
	int Ndata_NPS_cal_vtpClusE;
	int Ndata_NPS_cal_vtpClusSize;
	int Ndata_NPS_cal_vtpClusTime;
	int Ndata_NPS_cal_vtpClusX;
	int Ndata_NPS_cal_vtpClusY;
	int Ndata_NPS_cal_vtpTrigCrate;
	int Ndata_NPS_cal_vtpTrigTime;
	int Ndata_NPS_cal_vtpTrigType0;
	int Ndata_NPS_cal_vtpTrigType1;
	int Ndata_NPS_cal_vtpTrigType2;
	int Ndata_NPS_cal_vtpTrigType3;
	int Ndata_NPS_cal_vtpTrigType4;
	int Ndata_NPS_cal_vtpTrigType5;

	// NPS branches
	std::array<double, MAX_CLUSTERS> NPS_cal_clusE;
	std::array<double, MAX_CLUSTERS> NPS_cal_clusSize;
	std::array<double, MAX_CLUSTERS> NPS_cal_clusT;
	std::array<double, MAX_CLUSTERS> NPS_cal_clusX;
	std::array<double, MAX_CLUSTERS> NPS_cal_clusY;
	std::array<double, MAX_CLUSTERS> NPS_cal_clusZ;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcCounter;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcErrorFlag;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPed;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPedRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPulseAmp;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPulseAmpRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPulseInt;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPulseIntRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPulseTime;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcPulseTimeRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPed;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPedRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPulseAmp;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPulseAmpRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPulseInt;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPulseIntRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPulseTime;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_adcSampPulseTimeRaw;
	std::array<double, MAX_DATA> NPS_cal_fly_adcSampWaveform;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_block_clusterID;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_e;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcMult;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcPed;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcPulseAmp;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcPulseInt;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcPulseIntRaw;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcPulseTime;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_goodAdcTdcDiffTime;
	std::array<double, MAX_BLOCKS> NPS_cal_fly_numGoodAdcHits;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_mult;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_p;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_px;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_py;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_pz;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_x;
	std::array<double, MAX_BLOCKS> NPS_cal_trk_y;

	// vtp
	std::array<double, MAX_BLOCKS> NPS_cal_vtpClusE;
	std::array<double, MAX_BLOCKS> NPS_cal_vtpClusSize;
	std::array<double, MAX_BLOCKS> NPS_cal_vtpClusTime;
	std::array<double, MAX_BLOCKS> NPS_cal_vtpClusX;
	std::array<double, MAX_BLOCKS> NPS_cal_vtpClusY;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigCrate;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigTime;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigType0;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigType1;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigType2;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigType3;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigType4;
	std::array<double, MAX_VTP_TRIGGERS> NPS_cal_vtpTrigType5;

	// Additional NPS branches
	double NPS_cal_etot;
	double NPS_cal_fly_earray;
	double NPS_cal_fly_nclust;
	double NPS_cal_fly_ntracks;
	double NPS_cal_fly_totNumAdcHits;
	double NPS_cal_fly_totNumGoodAdcHits;
	double NPS_cal_nclust;
	double NPS_cal_nhits;
	double NPS_cal_trk_vx;
	double NPS_cal_trk_vy;
	double NPS_cal_trk_vz;
	double NPS_cal_vldErrorFlag;
	double NPS_cal_vtpErrorFlag;
	double NPS_kin_secondary_Erecoil;
	double NPS_kin_secondary_MandelS;
	double NPS_kin_secondary_MandelT;
	double NPS_kin_secondary_MandelU;
	double NPS_kin_secondary_Mrecoil;
	double NPS_kin_secondary_Prec_x;
	double NPS_kin_secondary_Prec_y;
	double NPS_kin_secondary_Prec_z;
	double NPS_kin_secondary_emiss;
	double NPS_kin_secondary_emiss_nuc;
	double NPS_kin_secondary_ph_bq;
	double NPS_kin_secondary_ph_xq;
	double NPS_kin_secondary_phb_cm;
	double NPS_kin_secondary_phx_cm;
	double NPS_kin_secondary_pmiss;
	double NPS_kin_secondary_pmiss_x;
	double NPS_kin_secondary_pmiss_y;
	double NPS_kin_secondary_pmiss_z;
	double NPS_kin_secondary_px_cm;
	double NPS_kin_secondary_t_tot_cm;
	double NPS_kin_secondary_tb;
	double NPS_kin_secondary_tb_cm;
	double NPS_kin_secondary_th_bq;
	double NPS_kin_secondary_th_xq;
	double NPS_kin_secondary_thb_cm;
	double NPS_kin_secondary_thx_cm;
	double NPS_kin_secondary_tx;
	double NPS_kin_secondary_tx_cm;
	double NPS_kin_secondary_xangle;
	double NPScorrDS_measCurr;
	double NPScorrDS_setCurr;
	double NPScorrUS_measCurr;
	double NPScorrUS_setCurr;
};

void setBranchAddresses(TChain *chain, npsBranches &buffer);
int unpackWaveform(
	int NSampWaveForm, int max_blocks, std::span<const double> SampWaveForm, std::vector<int> &blocks,
	std::vector<std::vector<double>> &signals
);

class ReplaySource : public JEventSource {

	std::unique_ptr<TChain> m_chain;
	npsBranches m_buffer;

	Parameter<std::string> m_tree{this, "replay_source:tree", "T", "Tree name to read from for ReplaySource"};
	Parameter<int> m_max_events{
		this, "replay_source:max_events", INT_MAX,
		"Maximum number of events to emit from the source. Default is INT_MAX."
	};
	Parameter<int> m_run_number{
		this, "replay_source:run_number", 0, "Run number to assign to events from the source. Default is 0."
	};

	Service<nps::geo::NpsGeometryService> m_nps_geometry_service{this};

public:
	ReplaySource();

	virtual ~ReplaySource() = default;

	void Init() override;

	void Open() override;

	void Close() override;

	Result Emit(JEvent &event) override;

	static std::string GetDescription();
};

} // namespace nps::io

template <> double JEventSourceGeneratorT<nps::io::ReplaySource>::CheckOpenable(std::string resource_name);
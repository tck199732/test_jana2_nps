#pragma once
#include <vector>

namespace halld::evio {

struct CODAROCInfo {
	uint32_t rocid;
	uint64_t timestamp;
	std::vector<uint32_t> misc;
};

struct CODAEventInfo {
	uint32_t run_number;
	uint32_t run_type;
	uint64_t event_number;
	uint16_t event_type;
	uint64_t avg_timestamp;
};

struct CODAControlEvent {
	uint16_t event_type;
	uint32_t unix_time;
	std::vector<uint32_t> words;
};

struct DAQAddress {
	uint32_t rocid;	   // crate
	uint32_t slot;	   // slot
	uint32_t channel;  // channel
	uint32_t itrigger; // trigger number within block (starting from 0)

	DAQAddress() : rocid(0), slot(0), channel(0), itrigger(0) {}
	DAQAddress(uint32_t r, uint32_t s, uint32_t c, uint32_t t) : rocid(r), slot(s), channel(c), itrigger(t) {}
};

struct DAQConfig {
	uint32_t rocid;		// crate
	uint32_t slot_mask; // slots
};

struct f250AsyncPedestal {
	uint32_t nsync;
	uint32_t trig_number;
	int crate;
	std::vector<uint32_t> ped;
};

struct f250Config : public DAQConfig {

	uint16_t NSA;	  // Num. samples before threshold crossing sample
	uint16_t NSB;	  // Num. samples after  threshold crossing sample
	uint16_t NSA_NSB; // NSA+NSB = total number of samples in integration window
	uint16_t NPED;	  // Number of samples used to determine pedestal
};

struct f250PulseData : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t event_within_block;
	bool QF_pedestal;
	uint32_t pedestal;

	uint32_t integral;
	bool QF_NSA_beyond_PTW;
	bool QF_overflow;
	bool QF_underflow;
	uint32_t nsamples_over_threshold;

	uint32_t course_time; //< 4 ns/count
	uint32_t fine_time;	  //< 0.0625 ns/count
	uint32_t pulse_peak;
	bool QF_vpeak_beyond_NSA;
	bool QF_vpeak_not_found;
	bool QF_bad_pedestal;

	uint32_t pulse_number;		///< pulse number for this channel, this event starting from 0
	uint32_t nsamples_integral; ///< number of samples used in integral
	uint32_t nsamples_pedestal; ///< number of samples used in pedestal

	bool emulated;				   ///< true if made from Window Raw Data
	uint32_t integral_emulated;	   ///< Value calculated from raw data (if available)
	uint32_t pedestal_emulated;	   ///< Value calculated from raw data (if available)
	uint32_t time_emulated;		   ///< Value calculated from raw data (if available)
	uint32_t course_time_emulated; ///< Value calculated from raw data (if available) - debug
	uint32_t fine_time_emulated;   ///< Value calculated from raw data (if available) - debug
	uint32_t pulse_peak_emulated;  ///< Value calculated from raw data (if available)
	uint32_t QF_emulated;
};

struct f250PulseIntegral : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t pulse_number;		///< from Pulse Integral Data word
	uint32_t quality_factor;	///< from Pulse Integral Data word
	uint32_t integral;			///< from Pulse Integral Data word
	uint32_t pedestal;			///< from Pulse Integral Data word (future)
	uint32_t nsamples_integral; ///< number of samples used in integral
	uint32_t nsamples_pedestal; ///< number of samples used in pedestal
	bool emulated;				///< true if made from Window Raw Data
	uint32_t integral_emulated; ///< Value calculated from raw data (if available)
	uint32_t pedestal_emulated; ///< Value calculated from raw data (if available)
};

struct f250PulsePedestal : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t pulse_number;		  ///< from Pulse Pedestal Data word
	uint32_t pedestal;			  ///< from Pulse Pedestal Data word
	uint32_t pulse_peak;		  ///< from Pulse Pedestal Data word
	bool emulated;				  ///< true if made from Window Raw Data
	uint32_t pedestal_emulated;	  ///< Calculated from raw data (when available)
	uint32_t pulse_peak_emulated; ///< Calculated from raw data (when available)
};

struct f250PulseTime : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t pulse_number;			  ///< from Pulse Time Data word
	uint32_t quality_factor;		  ///< from Pulse Time Data word
	uint32_t time;					  ///< from Pulse Time Data word
	bool emulated;					  ///< true if made from Window Raw Data
	uint32_t quality_factor_emulated; ///< Calculated from raw data if available
	uint32_t time_emulated;			  ///< Calculated from raw data if available
};
struct f250TriggerTime : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint64_t time; // from Trigger Time words
};

struct f250WindowRawData : public DAQAddress {
	using DAQAddress::DAQAddress;
	std::vector<uint16_t> samples; // from Window Raw Data words 2-N (each word contains 2 samples)
	bool invalid_samples;		   // true if any sample's "not valid" bit set
	bool overflow;				   // true if any sample's "overflow" bit set
};

struct f250WindowSum : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t sum;  // from Window Sum Data word
	bool overflow; // true if "overflow" bit set
};

struct EPICSvalue : public DAQAddress {
	using DAQAddress::DAQAddress;
	std::time_t timestamp;
	std::string nameval;
	std::string name;
	std::string sval;
	int ival;
	uint32_t uval;
	double fval;
};

struct f125CDCPulse : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t NPK;						///< from first word
	uint32_t le_time;					///< from first word
	uint32_t time_quality_bit;			///< from first word
	uint32_t overflow_count;			///< from first word
	uint32_t pedestal;					///< from second word
	uint32_t integral;					///< from second word
	uint32_t first_max_amp;				///< from second word
	uint32_t word1;						///< first word
	uint32_t word2;						///< second word
	uint32_t nsamples_pedestal;			///< number of samples used in integral
	uint32_t nsamples_integral;			///< number of samples used in pedestal
	bool emulated;						///< true if emulated values are copied to the main input
	uint32_t le_time_emulated;			///< emulated from raw data when available
	uint32_t time_quality_bit_emulated; ///< emulated from raw data when available
	uint32_t overflow_count_emulated;	///< emulated from raw data when available
	uint32_t pedestal_emulated;			///< emulated from raw data when available
	uint32_t integral_emulated;			///< emulated from raw data when available
	uint32_t first_max_amp_emulated;	///< emulated from raw data when available
};

struct f125Config : public DAQConfig {
	uint16_t NSA;	   // Num. samples before threshold crossing sample
	uint16_t NSB;	   // Num. samples after  threshold crossing sample
	uint16_t NSA_NSB;  // NSA+NSB = total number of samples in integration window
	uint16_t NPED;	   // Number of samples used to determine pedestal
	uint16_t WINWIDTH; // maximum integration window size (in samples)

	// See GlueX-doc-2274
	uint16_t PL;
	uint16_t NW;
	uint16_t NPK;
	uint16_t P1;
	uint16_t P2;
	uint16_t PG;
	uint16_t IE;
	uint16_t H;
	uint16_t TH;
	uint16_t TL;
	uint16_t IBIT;
	uint16_t ABIT;
	uint16_t PBIT;
};

struct f125FDCPulse : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t NPK;						///< from first word
	uint32_t le_time;					///< from first word
	uint32_t time_quality_bit;			///< from first word
	uint32_t overflow_count;			///< from first word
	uint32_t pedestal;					///< from second word
	uint32_t integral;					///< from second word (type 6)
	uint32_t peak_amp;					///< from second word (type 9)
	uint32_t peak_time;					///< from second word
	uint32_t word1;						///< first word
	uint32_t word2;						///< second word
	uint32_t nsamples_pedestal;			///< number of samples used in integral
	uint32_t nsamples_integral;			///< number of samples used in pedestal
	bool emulated;						///< true if emulated values are copied to the main input
	uint32_t le_time_emulated;			///< emulated from raw data when available
	uint32_t time_quality_bit_emulated; ///< emulated from raw data when available
	uint32_t overflow_count_emulated;	///< emulated from raw data when available
	uint32_t pedestal_emulated;			///< emulated from raw data when available
	uint32_t integral_emulated;			///< emulated from raw data when available
	uint32_t peak_amp_emulated;			///< emulated from raw data when available
	uint32_t peak_time_emulated;		///< emulated from raw data when available
};

struct f125PulseIntegral : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t pulse_number;		///< from Pulse Integral Data word
	uint32_t quality_factor;	///< from Pulse Integral Data word
	uint32_t integral;			///< from Pulse Integral Data word
	uint32_t pedestal;			///< from Pulse Integral Data word (future)
	uint32_t nsamples_integral; ///< number of samples used in integral
	uint32_t nsamples_pedestal; ///< number of samples used in pedestal
	bool emulated;				///< true if made from Window Raw Data
};

struct f125PulsePedestal : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t pulse_number; ///< from Pulse Pedestal Data word
	uint32_t pedestal;	   ///< from Pulse Pedestal Data word
	uint32_t pulse_peak;   ///< from Pulse Pedestal Data word
	uint32_t nsamples;	   ///< number of samples used in pedestal
	bool emulated;		   ///< true if made from Window Raw Data
};

struct f125PulseTime : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t pulse_number;	 ///< from Pulse Time Data word
	uint32_t quality_factor; ///< from Pulse Time Data word
	uint32_t time;			 ///< from Pulse Time Data word
	uint32_t overflows;		 ///< (future expansion. "7" means "7 or more" samples overflowed
	uint32_t peak_time;		 ///< from 2nd word for FDC data only (type 6 or 9)
	bool emulated;			 ///< true if made from Window Raw Data
};

struct f125TriggerTime : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t itrigger; // from Event Header
	uint64_t time;	   // from Trigger Time words
};

struct f125WindowRawData : public DAQAddress {
	using DAQAddress::DAQAddress;
	std::vector<uint16_t> samples; // from Window Raw Data words 2-N (each word contains 2 samples)
	bool invalid_samples;		   // true if any sample's "not valid" bit set
	bool overflow;				   // true if any sample's "overflow" bit set
};

struct F1TDCConfig : public DAQConfig {
	uint16_t REFCNT;
	uint16_t TRIGWIN;
	uint16_t TRIGLAT;
	uint16_t HSDIV;
	uint16_t BINSIZE;
	uint16_t REFCLKDIV;
};

struct GEMSRSWindowRawData : public DAQAddress {
	using DAQAddress::DAQAddress;
	uint32_t apv_id; ///< APV Identifier number on the FEC card (0 to 15)
	// uint32_t fec_id;         ///< FEC Identifier number (always 0?)
	uint32_t channel_apv;		   ///< APV physical channels are 0 to 127
	std::vector<uint16_t> samples; ///< ADC samples
};

struct TScaler {
	uint32_t nsync_event;
	uint32_t int_count;
	uint32_t live_time;		// in clock counts (integrated)
	uint32_t busy_time;		// in clock counts (integrated)
	uint32_t inst_livetime; // in percent x10 (instantaneous)
	uint32_t time;			// unix time in sec
	uint32_t gtp_scalers[32];
	uint32_t fp_scalers[16];
	uint32_t gtp_rate[32];
	uint32_t fp_rate[16];
};

struct TSGBORConfig {
	uint32_t rocid; // always 81. Needed for SortByModule in LinAssociations.h
	uint32_t slot;	// always 1
	uint32_t run_number;
	uint32_t unix_time;
	std::vector<uint32_t> misc_words; // extra words that may be added later
};

struct ParsedEvent {
	uint32_t run_num;
	uint64_t event_num;
	CODAEventInfo event_info;

	std::vector<CODAROCInfo> roc_infos;
	std::vector<CODAControlEvent> control_events;

	std::vector<f250AsyncPedestal> f250_async_pedestals;
	std::vector<f250Config> f250_configs;
	std::vector<f250PulseData> f250_pulse_data;
	std::vector<f250PulseIntegral> f250_pulse_integrals;
	std::vector<f250PulsePedestal> f250_pulse_pedestals;
	std::vector<f250PulseTime> f250_pulse_times;
	std::vector<f250TriggerTime> f250_trigger_times;
	std::vector<f250WindowRawData> f250_window_raw_data;
	std::vector<f250WindowSum> f250_window_sums;

	std::vector<f125CDCPulse> f125_cdc_pulses;
	std::vector<f125Config> f125_configs;
	std::vector<f125FDCPulse> f125_fdc_pulses;
	std::vector<f125PulseIntegral> f125_pulse_integrals;
	std::vector<f125PulsePedestal> f125_pulse_pedestals;
	std::vector<f125PulseTime> f125_pulse_times;
	std::vector<f125TriggerTime> f125_trigger_times;
	std::vector<f125WindowRawData> f125_window_raw_data;

	std::vector<F1TDCConfig> f1tdc_configs;
	std::vector<GEMSRSWindowRawData> gemsrs_window_raw_data;
	std::vector<TScaler> t_scalers;
	std::vector<EPICSvalue> epics_values;
};

} // namespace halld::evio
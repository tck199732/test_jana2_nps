#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace nps {

struct fadc_hit {
	int channel;   // channel no. / block ID (0-1079)
	double charge; // integrated charge before pedestal
	double time;   // rise time determined by leading edge
};

struct fadc_waveform {
	int channel;					// channel no. / block ID (0-1079)
	std::vector<double> samples;	// ADC samples of the waveform
	std::vector<double> timestamps; // timestamps of the waveform samples
};

struct fadc_cfg {
	int mode;
	int compression;
	int vxsreadout;
	int width;
	int offset;
	int nsa;
	int nsb;
	int npeak;
	int trg_mask;
	int trg_width;
	int trg_mintot;
	int trg_minmult;
	int adc_mask;
	int tet_mask;
	int invert_mask;
	int playback_disable_mask;
	int trg_mode_mask;
	std::array<int, 16> dac;
	std::array<int, 16> ped;
	std::array<int, 16> tet;
	std::array<int, 16> delay;
	std::array<float, 16> gain;
	int sparsification;
	int accumulator_scaler_mode_mask;
};

// hall D triggered data structure for waveform
struct fadc_window_raw_record {
	uint32_t roc;
	uint32_t slot;
	uint32_t channel;
	bool invalid_samples;
	bool overflow;
	uint32_t itrigger;
	std::vector<uint16_t> samples;
};

// hall D triggered data structure for pulse
struct fadc_pulse_record {

	uint32_t roc;
	uint32_t slot;
	uint32_t channel;
	uint32_t event_within_block;
	bool qf_pedestal;
	uint32_t pedestal;
	uint32_t integral;
	bool qf_nsa_beyond_ptw;
	bool qf_overflow;
	bool qf_underflow;
	uint32_t nsamples_over_threshold;
	uint32_t course_time;
	uint32_t fine_time;
	uint32_t pulse_peak;
	bool qf_vpeak_beyond_nsa;
	bool qf_vpeak_not_found;
	bool qf_bad_pedestal;
	uint32_t pulse_number;
	uint32_t nsamples_integral;
	uint32_t nsamples_pedestal;
	bool emulated;
	uint32_t integral_emulated;
	uint32_t pedestal_emulated;
	uint32_t time_emulated;
	uint32_t course_time_emulated;
	uint32_t fine_time_emulated;
	uint32_t pulse_peak_emulated;
	uint32_t qf_emulated;
};

} // namespace nps
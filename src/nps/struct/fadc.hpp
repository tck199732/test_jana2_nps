#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace nps {

struct fadc_hit {
	int channel;				  // channel no. / block ID (0-1079)
	double charge;				  // integrated charge before pedestal
	double time;				  // rise time determined by leading edge
	std::vector<double> waveform; // ADC samples of the waveform
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

} // namespace nps
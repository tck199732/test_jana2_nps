#include "EvioTrgParsingFactory.hpp"

namespace nps::clustering {

void EvioTrgParsingFactory::Configure() {}

void EvioTrgParsingFactory::ChangeRun(int32_t run_number) {}

void EvioTrgParsingFactory::Execute(int32_t /*run_nr*/, uint64_t event_index) {

	for (const auto *event : m_parsed_events()) {
		PopulateFadcHits(event);
		PopulateFadcWaveforms(event);
	}
	return;
}

void EvioTrgParsingFactory::PopulateFadcHits(const halld::evio::ParsedEvent *event) {
	for (auto evio_pulse : event->f250_pulse_data) {
		m_f250_pulse().push_back(new nps::fadc_pulse_record{
			.roc = evio_pulse.rocid,
			.slot = evio_pulse.slot,
			.channel = evio_pulse.channel,
			.event_within_block = evio_pulse.event_within_block,
			.qf_pedestal = evio_pulse.QF_pedestal,
			.pedestal = evio_pulse.pedestal,
			.integral = evio_pulse.integral,
			.qf_nsa_beyond_ptw = evio_pulse.QF_NSA_beyond_PTW,
			.qf_overflow = evio_pulse.QF_overflow,
			.qf_underflow = evio_pulse.QF_underflow,
			.nsamples_over_threshold = evio_pulse.nsamples_over_threshold,
			.course_time = evio_pulse.course_time,
			.fine_time = evio_pulse.fine_time,
			.pulse_peak = evio_pulse.pulse_peak,
			.qf_vpeak_beyond_nsa = evio_pulse.QF_vpeak_beyond_NSA,
			.qf_vpeak_not_found = evio_pulse.QF_vpeak_not_found,
			.qf_bad_pedestal = evio_pulse.QF_bad_pedestal,
			.pulse_number = evio_pulse.pulse_number,
			.nsamples_integral = evio_pulse.nsamples_integral,
			.nsamples_pedestal = evio_pulse.nsamples_pedestal,

			.emulated = evio_pulse.emulated,
			.integral_emulated = evio_pulse.integral_emulated,
			.pedestal_emulated = evio_pulse.pedestal_emulated,
			.time_emulated = evio_pulse.time_emulated,
			.course_time_emulated = evio_pulse.course_time_emulated,
			.fine_time_emulated = evio_pulse.fine_time_emulated,
			.pulse_peak_emulated = evio_pulse.pulse_peak_emulated,
			.qf_emulated = evio_pulse.QF_emulated

		});
	}
}

void EvioTrgParsingFactory::PopulateFadcWaveforms(const halld::evio::ParsedEvent *event) {

	for (auto evio_fadc : event->f250_window_raw_data) {
		m_f250_wraw().push_back(new nps::fadc_window_raw_record{
			.roc = evio_fadc.rocid,
			.slot = evio_fadc.slot,
			.channel = evio_fadc.channel,
			.invalid_samples = evio_fadc.invalid_samples,
			.overflow = evio_fadc.overflow,
			.itrigger = evio_fadc.itrigger,
			.samples = std::move(evio_fadc.samples),
		});
	}
}

} // namespace nps::clustering
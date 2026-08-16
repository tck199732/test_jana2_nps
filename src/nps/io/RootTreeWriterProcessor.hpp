#pragma once

#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>
#include <JANA/Services/JServiceLocator.h>

#include <TFile.h>
#include <TTree.h>

#include <evio/halld_modules/data_struct.hpp>

#include "struct/cluster.hpp"
#include "struct/fadc.hpp"
#include "struct/vtp.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

namespace nps::io {

class BaseRootTreeWriterProcessor : public JEventProcessor {

	Parameter<std::string> m_filename{this, "root:file_name", "output.root"};
	Parameter<std::string> m_treename{this, "root:tree_name", "tree"};

private:
	std::unique_ptr<TFile> m_file;
	TTree *m_tree;

public:
	BaseRootTreeWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void Init() override {
		m_file = std::make_unique<TFile>(m_filename().c_str(), "RECREATE");
		if (m_file->IsZombie()) {
			throw std::runtime_error("Unable to create ROOT file: " + m_filename());
		}
		m_tree = new TTree(m_treename().c_str(), "Hall D Trigger NPS Data");
		SetBranch(m_tree);
	}

	void ProcessSequential(const JEvent &event) {
		Clear();
		PopulateEntries(event);
		m_tree->Fill();
	}

	void Finish() override {
		if (m_file) {
			m_file->cd();

			if (m_tree) {
				m_tree->Write();
			}
			m_file->Close();
		}
		m_tree = nullptr;
	}

	virtual void SetBranch(TTree *tree) = 0;
	virtual void PopulateEntries(const JEvent &event) = 0;
	virtual void Clear() = 0;
};

class HallDTriggerRootTreeWriterProcessor : public BaseRootTreeWriterProcessor {

	struct f250_wraw_buffer {
		uint64_t m_count;
		std::vector<uint32_t> m_vect_roc;
		std::vector<uint32_t> m_vect_slot;
		std::vector<uint32_t> m_vect_channel;
		std::vector<bool> m_vect_invalid_samples;
		std::vector<bool> m_vect_overflow;
		std::vector<uint32_t> m_vect_itrigger;
		std::vector<uint16_t> m_vect_samples_count;
		std::vector<uint16_t> m_vect_samples_index;
		std::vector<uint16_t> m_vect_samples;
		void clear() {
			m_count = 0;
			m_vect_roc.clear();
			m_vect_slot.clear();
			m_vect_channel.clear();
			m_vect_invalid_samples.clear();
			m_vect_overflow.clear();
			m_vect_itrigger.clear();
			m_vect_samples_count.clear();
			m_vect_samples_index.clear();
			m_vect_samples.clear();
		}
	};
	struct f250_pulse_buffer {
		uint64_t m_count;
		std::vector<uint32_t> m_vect_roc;
		std::vector<uint32_t> m_vect_slot;
		std::vector<uint32_t> m_vect_channel;
		std::vector<uint32_t> m_vect_event_within_block;
		std::vector<bool> m_vect_qf_pedestal;
		std::vector<uint32_t> m_vect_pedestal;
		std::vector<uint32_t> m_vect_integral;
		std::vector<bool> m_vect_qf_nsa_beyond_ptw;
		std::vector<bool> m_vect_qf_overflow;
		std::vector<bool> m_vect_qf_underflow;
		std::vector<uint32_t> m_vect_nsamples_over_threshold;
		std::vector<uint32_t> m_vect_course_time;
		std::vector<uint32_t> m_vect_fine_time;
		std::vector<uint32_t> m_vect_pulse_peak;
		std::vector<bool> m_vect_qf_vpeak_beyond_nsa;
		std::vector<bool> m_vect_qf_vpeak_not_found;
		std::vector<bool> m_vect_qf_bad_pedestal;
		std::vector<uint32_t> m_vect_pulse_number;
		std::vector<uint32_t> m_vect_nsamples_integral;
		std::vector<uint32_t> m_vect_nsamples_pedestal;
		std::vector<bool> m_vect_emulated;
		std::vector<uint32_t> m_vect_integral_emulated;
		std::vector<uint32_t> m_vect_pedestal_emulated;
		std::vector<uint32_t> m_vect_time_emulated;
		std::vector<uint32_t> m_vect_course_time_emulated;
		std::vector<uint32_t> m_vect_fine_time_emulated;
		std::vector<uint32_t> m_vect_pulse_peak_emulated;
		std::vector<uint32_t> m_vect_qf_emulated;
		void clear() {
			m_count = 0;
			m_vect_roc.clear();
			m_vect_slot.clear();
			m_vect_channel.clear();
			m_vect_event_within_block.clear();
			m_vect_qf_pedestal.clear();
			m_vect_pedestal.clear();
			m_vect_integral.clear();
			m_vect_qf_nsa_beyond_ptw.clear();
			m_vect_qf_overflow.clear();
			m_vect_qf_underflow.clear();
			m_vect_nsamples_over_threshold.clear();
			m_vect_course_time.clear();
			m_vect_fine_time.clear();
			m_vect_pulse_peak.clear();
			m_vect_qf_vpeak_beyond_nsa.clear();
			m_vect_qf_vpeak_not_found.clear();
			m_vect_qf_bad_pedestal.clear();
			m_vect_pulse_number.clear();
			m_vect_nsamples_integral.clear();
			m_vect_nsamples_pedestal.clear();
			m_vect_emulated.clear();
			m_vect_integral_emulated.clear();
			m_vect_pedestal_emulated.clear();
			m_vect_time_emulated.clear();
			m_vect_course_time_emulated.clear();
			m_vect_fine_time_emulated.clear();
			m_vect_pulse_peak_emulated.clear();
			m_vect_qf_emulated.clear();
		}
	};

	Input<nps::fadc_window_raw_record> m_f250_wraw{this, {"f250_wraw", JEventLevel::None, true}};
	Input<nps::fadc_pulse_record> m_f250_pulse{this, {"f250_pulse", JEventLevel::None, true}};

public:
	HallDTriggerRootTreeWriterProcessor() {
		SetTypeName(NAME_OF_THIS);
		SetCallbackStyle(CallbackStyle::ExpertMode);
	}

	void SetBranch(TTree *tree) override {
		tree->Branch("f250_wraw_count", &m_fadc_wraw_vec.m_count, "f250_wraw_count/l");
		tree->Branch("f250_wraw_roc", &m_fadc_wraw_vec.m_vect_roc);
		tree->Branch("f250_wraw_slot", &m_fadc_wraw_vec.m_vect_slot);
		tree->Branch("f250_wraw_channel", &m_fadc_wraw_vec.m_vect_channel);
		tree->Branch("f250_wraw_invalid_samples", &m_fadc_wraw_vec.m_vect_invalid_samples);
		tree->Branch("f250_wraw_overflow", &m_fadc_wraw_vec.m_vect_overflow);
		tree->Branch("f250_wraw_itrigger", &m_fadc_wraw_vec.m_vect_itrigger);
		tree->Branch("f250_wraw_samples_index", &m_fadc_wraw_vec.m_vect_samples_index);
		tree->Branch("f250_wraw_samples_count", &m_fadc_wraw_vec.m_vect_samples_count);
		tree->Branch("f250_wraw_samples", &m_fadc_wraw_vec.m_vect_samples);

		tree->Branch("f250_pulse_count", &m_fadc_pulse_vec.m_count, "f250_pulse_count/l");
		tree->Branch("f250_pulse_roc", &m_fadc_pulse_vec.m_vect_roc);
		tree->Branch("f250_pulse_slot", &m_fadc_pulse_vec.m_vect_slot);
		tree->Branch("f250_pulse_channel", &m_fadc_pulse_vec.m_vect_channel);
		tree->Branch("f250_pulse_event_within_block", &m_fadc_pulse_vec.m_vect_event_within_block);
		tree->Branch("f250_pulse_qf_pedestal", &m_fadc_pulse_vec.m_vect_qf_pedestal);
		tree->Branch("f250_pulse_pedestal", &m_fadc_pulse_vec.m_vect_pedestal);
		tree->Branch("f250_pulse_integral", &m_fadc_pulse_vec.m_vect_integral);
		tree->Branch("f250_pulse_qf_nsa_beyond_ptw", &m_fadc_pulse_vec.m_vect_qf_nsa_beyond_ptw);
		tree->Branch("f250_pulse_qf_overflow", &m_fadc_pulse_vec.m_vect_qf_overflow);
		tree->Branch("f250_pulse_qf_underflow", &m_fadc_pulse_vec.m_vect_qf_underflow);
		tree->Branch("f250_pulse_nsamples_over_threshold", &m_fadc_pulse_vec.m_vect_nsamples_over_threshold);
		tree->Branch("f250_pulse_course_time", &m_fadc_pulse_vec.m_vect_course_time);
		tree->Branch("f250_pulse_fine_time", &m_fadc_pulse_vec.m_vect_fine_time);
		tree->Branch("f250_pulse_pulse_peak", &m_fadc_pulse_vec.m_vect_pulse_peak);
		tree->Branch("f250_pulse_qf_vpeak_beyond_nsa", &m_fadc_pulse_vec.m_vect_qf_vpeak_beyond_nsa);
		tree->Branch("f250_pulse_qf_vpeak_not_found", &m_fadc_pulse_vec.m_vect_qf_vpeak_not_found);
		tree->Branch("f250_pulse_qf_bad_pedestal", &m_fadc_pulse_vec.m_vect_qf_bad_pedestal);
		tree->Branch("f250_pulse_pulse_number", &m_fadc_pulse_vec.m_vect_pulse_number);
		tree->Branch("f250_pulse_nsamples_integral", &m_fadc_pulse_vec.m_vect_nsamples_integral);
		tree->Branch("f250_pulse_nsamples_pedestal", &m_fadc_pulse_vec.m_vect_nsamples_pedestal);
		tree->Branch("f250_pulse_emulated", &m_fadc_pulse_vec.m_vect_emulated);
		tree->Branch("f250_pulse_integral_emulated", &m_fadc_pulse_vec.m_vect_integral_emulated);
		tree->Branch("f250_pulse_pedestal_emulated", &m_fadc_pulse_vec.m_vect_pedestal_emulated);
		tree->Branch("f250_pulse_time_emulated", &m_fadc_pulse_vec.m_vect_time_emulated);
		tree->Branch("f250_pulse_course_time_emulated", &m_fadc_pulse_vec.m_vect_course_time_emulated);
		tree->Branch("f250_pulse_fine_time_emulated", &m_fadc_pulse_vec.m_vect_fine_time_emulated);
		tree->Branch("f250_pulse_pulse_peak_emulated", &m_fadc_pulse_vec.m_vect_pulse_peak_emulated);
		tree->Branch("f250_pulse_qf_emulated", &m_fadc_pulse_vec.m_vect_qf_emulated);
	}

	void PopulateEntries(const JEvent &event) override {
		for (const auto *f250_wraw : m_f250_wraw()) {
			m_fadc_wraw_vec.m_count++;
			m_fadc_wraw_vec.m_vect_roc.push_back(f250_wraw->roc);
			m_fadc_wraw_vec.m_vect_slot.push_back(f250_wraw->slot);
			m_fadc_wraw_vec.m_vect_channel.push_back(f250_wraw->channel);
			m_fadc_wraw_vec.m_vect_invalid_samples.push_back(f250_wraw->invalid_samples);
			m_fadc_wraw_vec.m_vect_overflow.push_back(f250_wraw->overflow);
			m_fadc_wraw_vec.m_vect_itrigger.push_back(f250_wraw->itrigger);

			for (auto item : f250_wraw->samples) {
				m_fadc_wraw_vec.m_vect_samples.push_back(item);
			}

			// First record, samples index = 0
			if (m_fadc_wraw_vec.m_vect_samples_count.size() == 0) {
				m_fadc_wraw_vec.m_vect_samples_index.push_back(0);
			} else {
				auto last_count = m_fadc_wraw_vec.m_vect_samples_count[m_fadc_wraw_vec.m_vect_samples_count.size() - 1];
				auto last_index = m_fadc_wraw_vec.m_vect_samples_index[m_fadc_wraw_vec.m_vect_samples_index.size() - 1];
				m_fadc_wraw_vec.m_vect_samples_index.push_back(last_index + last_count);
			}
			m_fadc_wraw_vec.m_vect_samples_count.push_back(f250_wraw->samples.size());
		}

		for (const auto *f250_pulse : m_f250_pulse()) {
			m_fadc_pulse_vec.m_count++;

			m_fadc_pulse_vec.m_vect_roc.push_back(f250_pulse->roc);
			m_fadc_pulse_vec.m_vect_slot.push_back(f250_pulse->slot);
			m_fadc_pulse_vec.m_vect_channel.push_back(f250_pulse->channel);
			m_fadc_pulse_vec.m_vect_event_within_block.push_back(f250_pulse->event_within_block);
			m_fadc_pulse_vec.m_vect_qf_pedestal.push_back(f250_pulse->qf_pedestal);
			m_fadc_pulse_vec.m_vect_pedestal.push_back(f250_pulse->pedestal);
			m_fadc_pulse_vec.m_vect_integral.push_back(f250_pulse->integral);
			m_fadc_pulse_vec.m_vect_qf_nsa_beyond_ptw.push_back(f250_pulse->qf_nsa_beyond_ptw);
			m_fadc_pulse_vec.m_vect_qf_overflow.push_back(f250_pulse->qf_overflow);
			m_fadc_pulse_vec.m_vect_qf_underflow.push_back(f250_pulse->qf_underflow);
			m_fadc_pulse_vec.m_vect_nsamples_over_threshold.push_back(f250_pulse->nsamples_over_threshold);
			m_fadc_pulse_vec.m_vect_course_time.push_back(f250_pulse->course_time);
			m_fadc_pulse_vec.m_vect_fine_time.push_back(f250_pulse->fine_time);
			m_fadc_pulse_vec.m_vect_pulse_peak.push_back(f250_pulse->pulse_peak);
			m_fadc_pulse_vec.m_vect_qf_vpeak_beyond_nsa.push_back(f250_pulse->qf_vpeak_beyond_nsa);
			m_fadc_pulse_vec.m_vect_qf_vpeak_not_found.push_back(f250_pulse->qf_vpeak_not_found);
			m_fadc_pulse_vec.m_vect_qf_bad_pedestal.push_back(f250_pulse->qf_bad_pedestal);
			m_fadc_pulse_vec.m_vect_pulse_number.push_back(f250_pulse->pulse_number);
			m_fadc_pulse_vec.m_vect_nsamples_integral.push_back(f250_pulse->nsamples_integral);
			m_fadc_pulse_vec.m_vect_nsamples_pedestal.push_back(f250_pulse->nsamples_pedestal);
			m_fadc_pulse_vec.m_vect_emulated.push_back(f250_pulse->emulated);
			m_fadc_pulse_vec.m_vect_integral_emulated.push_back(f250_pulse->integral_emulated);
			m_fadc_pulse_vec.m_vect_pedestal_emulated.push_back(f250_pulse->pedestal_emulated);
			m_fadc_pulse_vec.m_vect_time_emulated.push_back(f250_pulse->time_emulated);
			m_fadc_pulse_vec.m_vect_course_time_emulated.push_back(f250_pulse->course_time_emulated);
			m_fadc_pulse_vec.m_vect_fine_time_emulated.push_back(f250_pulse->fine_time_emulated);
			m_fadc_pulse_vec.m_vect_pulse_peak_emulated.push_back(f250_pulse->pulse_peak_emulated);
			m_fadc_pulse_vec.m_vect_qf_emulated.push_back(f250_pulse->qf_emulated);
		}
	}

	void Clear() override {
		m_fadc_wraw_vec.clear();
		m_fadc_pulse_vec.clear();
	}

private:
	f250_wraw_buffer m_fadc_wraw_vec;
	f250_pulse_buffer m_fadc_pulse_vec;
};

} // namespace nps::io
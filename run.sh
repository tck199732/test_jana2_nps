#!/bin/bash

# Sample run script for RecoClusterVTP plugin. Adjust the parameters as needed.

jana -Pplugins=RecoClusterVTP \
    -Pvtp_config_file=database/jlog/nps_run_4599_vtp_config.csv \
    -Pvme_config_file=database/jlog/nps_run_4599_vme_config.csv \
    -Pnps:geo_config_file=database/geo/channel_map.csv \
    -Preplay_source:max_events=10000 \
    -Pjana:plugin_path=./build/plugins/RecoClusterVTP \
    "/lustre24/expphy/volatile/hallc/nps/nps-ana/wf_test/ROOTfiles/nps_hms_coin_4599_0_1_-1.root"

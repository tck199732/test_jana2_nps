#!/bin/bash

set -e

USER=$(whoami)
WORKDIR=$(pwd)

# modify image pth accordingly
IMAGE="/lustre24/expphy/volatile/eic/$USER/images/jana2root.sif"
RUN_LOCAL=false

while getopts "l" opt; do
  case $opt in
    l)
      RUN_LOCAL=true
      ;;
  esac
done

# Sample run script for RecoClusterVTP plugin. Adjust the parameters as needed.
INPUT="/lustre24/expphy/volatile/hallc/nps/nps-ana/wf_test/ROOTfiles/nps_hms_coin_4599_0_1_-1.root"
JANA_CMD="jana \
    -Pplugins=RecoClusterVTP \
    -Pvtp_config_file=database/jlog/nps_run_4599_vtp_config.csv \
    -Pvme_config_file=database/jlog/nps_run_4599_vme_config.csv \
    -Pnps:geo_config_file=database/geo/channel_map.csv \
    -Preplay_source:max_events=1000 \
    -Pjana:plugin_path=./build/plugins/RecoClusterVTP \
    ${INPUT}"
    
if [ "$RUN_LOCAL" = true ]; then
    echo "Running locally..."
    $JANA_CMD

else

    if [ ! -f "$IMAGE" ]; then
        echo "Image not found at $IMAGE. Abort."
        exit 1
    fi

    echo "Using Singularity image at $IMAGE"
    singularity exec \
        --bind ${WORKDIR} \
        --bind /lustre24 \
        $IMAGE \
        bash -c "
        ${JANA_CMD}
        "   
fi






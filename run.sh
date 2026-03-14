#!/bin/bash

USER=$(whoami)

DOCKER_CONTAINER="onnx_cuda_root_jana2"
SINGULARITY_IMAGE="image.sif"

USE_DOCKER=true
USE_SINGULARITY=false

# parse options
while getopts "s" opt; do
  case $opt in
    s)
        if [ ! -f "$SINGULARITY_IMAGE" ]; then
            echo "Singularity image not found at $SINGULARITY_IMAGE. Abort."
            exit 1
        else
            USE_SINGULARITY=true
            USE_DOCKER=false
        fi
        ;;
  esac
done

# Sample run script for RecoClusterVTP plugin. Adjust the parameters as needed.
DATA_DIR="/lustre24/expphy/volatile/hallc/nps/nps-ana/wf_test/ROOTfiles"
INPUT="${DATA_DIR}/nps_hms_coin_4599_0_1_-1.root"

JANA_CMD="jana \
    -Pplugins=RecoClusterVTP \
    -Pvtp_config_file=database/jlog/nps_run_4599_vtp_config.csv \
    -Pvme_config_file=database/jlog/nps_run_4599_vme_config.csv \
    -Pnps:geo_config_file=database/geo/channel_map.csv \
    -Preplay_source:max_events=1000 \
    -Pjana:plugin_path=./build/plugins/RecoClusterVTP \
    ${INPUT}"
    


if [ "$USE_DOCKER" = true ]; then
    podman run \
        --userns=keep-id \
        --security-opt label=disable \
        -v $(pwd):/workspace \
        -v ${DATA_DIR}:${DATA_DIR} \
        -w /workspace \
        $DOCKER_CONTAINER \
        bash -lc "$JANA_CMD"

else

    singularity exec \
        --bind $(pwd) \
        --bind ${DATA_DIR} \
        $SINGULARITY_IMAGE \
        bash -c "$JANA_CMD"

fi






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

DATA_DIR="/lustre24/expphy/volatile/hallc/nps/nps-ana/wf_test/ROOTfiles"
INPUT="${DATA_DIR}/nps_hms_coin_4599_0_1_-1.root"

JANA_CMD="./build/src/nps/nps_onnx_app \
    -pjana:timeout=0 \
    -pjana:nevents=10000 \
    -pnps:output_tag=4599 \
    -pgeo:config_file=database/geo/channel_map.csv \
    -pcalib:fadc_config_file=database/jlog/nps_run_4599_vme_config.csv \
    -pcalib:vtp_config_file=database/jlog/nps_run_4599_vtp_config.csv \
    -pWaveformClusterFactory:model_path=database/models/vtp_reco/my_model.onnx \
    -pWaveformClusterFactory:session_name=waveform_clustering_session \
    -pWaveformClusterFactory:batch_size=1 \
    -pWaveformClusterFactory:use_cuda=false \
    -pHitClusterFactory:model_path=database/models/sim/my_model.onnx \
    -pHitClusterFactory:session_name=hit_clustering_session \
    -pHitClusterFactory:batch_size=1 \
    -pHitClusterFactory:use_cuda=false \
    -pevent_source_type=nps::io::RandomSource \
    -penable_waveform_clustering=false \
    -penable_vtp_clustering=false \
    -penable_hit_clustering=true \
    -pnthreads=1 \
    $INPUT"


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





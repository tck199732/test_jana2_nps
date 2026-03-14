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


CMD="cmake -B build -S . && cmake --build build -j8"

if [ "$USE_DOCKER" = true ]; then
    podman run \
        --userns=keep-id \
        --security-opt label=disable \
        -v $(pwd):/workspace \
        -w /workspace \
        $DOCKER_CONTAINER \
        bash -lc "$CMD"

else

    singularity exec \
        --bind $(pwd) \
        $SINGULARITY_IMAGE \
        bash -c "$CMD"

fi
#!/bin/bash

# modify image pth accordingly
USER=$(whoami)
IMAGE="/lustre24/expphy/volatile/eic/$USER/images/jana2root.sif"
BUILD_LOCAL=false

while getopts "l" opt; do
  case $opt in
    l)
      BUILD_LOCAL=true
      ;;
  esac
done


if [ "$BUILD_LOCAL" = true ]; then
    echo "Building locally..."
    cmake -B build -S . -DJANA_DIR=$JANA_HOME/lib/JANA/cmake
    cmake --build build -j 8
else
    if [ ! -f "$IMAGE" ]; then
        echo "Image not found at $IMAGE. Cannot build."
        exit 1
    fi
    echo "Using Singularity image at $IMAGE"
    singularity exec \
        --bind $(pwd) \
        $IMAGE \
        bash -c "
        cmake -B build -S .
        cmake --build build -j 8
        "   
fi

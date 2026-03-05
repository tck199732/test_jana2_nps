#!/bin/bash

cmake -B build -S . -DJANA_DIR=$JANA_HOME/lib/JANA/cmake
cmake --build build -j 8

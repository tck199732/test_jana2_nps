#!/bin/bash

module purge
module use /group/halla/modulefiles
module load root/6.30.04

# modify the path to your JANA2 installation here
set parent = `dirname $PWD`
setenv JANA_HOME "$parent/software/JANA2"
set path = ( $JANA_HOME/bin $path )


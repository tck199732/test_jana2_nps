# test_jana2_nps

This repository serves as a validation and testing suite for integrating Neutral Pole Spectrometer (**NPS**) reconstruction utilities into the **JANA2** multi-threaded framework. Specifically, it ports existing logic from the [nps-sro-ml](https://github.com/JeffersonLab/nps-sro-ml) converter to evaluate performance within a JANA2-based workflow.

## 🛠 Setup & Installation

You can build and run this project either using **Docker/Podman container** or **Singularity/Apptainer container**. 

> [!CAUTION]
> **Avoid Environment Mixing:** Do not attempt to run a local build inside a container or vice-versa. Always clean your build directory when switching environments to prevent library conflicts.

### Option 1: Docker / Podman on iFarm
Build the docker image with the following command
```bash
cd containers/docker
docker build --userns=keep-id --security-opt label=disable -t ${tag} .
```

### Option 2: Singularity
You are recommend to set up the CACHE and TMP directories for apptainer as the default location `$HOME/.apptainer` do not have enough space on ifarm. 
```bash
# e.g. in your .cscsh
setenv APPTAINER_TMPDIR    "/scratch/$USER/.apptainer/tmp"
setenv APPTAINER_CACHEDIR  "/scratch/$USER/.apptainer/cache"
setenv APPTAINER_CONFIGDIR "$HOME/.apptainer"
```
After setting up the paths, restart the terminal and build the provided Singularity image:
```bash
cd containers/singularity
singularity build ${image_dir}/image.sif onnx_cuda_root_jana2.def
ln -s ${image_dir}/image.sif ./image.sif
```
Note this process takes several minutes and raises some harmless warnings. 


## 📊 Running the Benchmarks
The core logic resides in `plugins/RecoClusterVTP`. This plugin performs the following pipeline:

1. Data Ingestion: Loads raw/simulated events from a ROOT Tree.
2. VTP Reconstruction: Invokes C++ utility classes to reconstruct clusters using logic mirroring the FPGA hardware.
3. Monitoring: Registers and fills histograms.

### Build and Execution
Use the provided wrapper scripts. By default, these scripts use the docker container. Use the `-s` flag for singularity usage.
```
./build.sh [-s]
./run.sh [-s]
```

## Directory strucutre
- [**`plugins/RecoClusterVTP`**] : The JANA2 plugin wrapper and event processor logic.
- [**`src`**] : Core reconstruction utilities and VTP emulation logic.
- [**`images`**] : Definition files for Singularity/Apptainer containers.
- [**`database`**] : input files required for defining NPS geometry, module configurations.

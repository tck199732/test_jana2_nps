# test_jana2_nps

This repository serves as a validation and testing suite for integrating Neutral Pole Spectrometer (**NPS**) reconstruction utilities into the **JANA2** multi-threaded framework. Specifically, it ports existing logic from the [nps-sro-ml](https://github.com/JeffersonLab/nps-sro-ml) converter to evaluate performance within a JANA2-based workflow.

## 🛠 Setup & Installation

You can build and run this project either natively on the **JLab ifarm** or via a **Singularity/Apptainer container**. 

> [!CAUTION]
> **Avoid Environment Mixing:** Do not attempt to run a local build inside a container or vice-versa. Always clean your build directory when switching environments to prevent library conflicts.

### Option 1: Local Build (JLab ifarm)
1. Ensure [JANA2](https://github.com/JeffersonLab/JANA2) is installed on your system.
2. Open `ifarm_local.sh` and update the `JANA_HOME` path to your local installation.
3. Initialize the environment:
   ```bash
   source ifarm_local.sh
   ```

### Option 2: Containerized Build
To ensure a reproducible environment with ROOT and JANA2 pre-installed, build the provided Singularity image:
```bash
# Recommended: Run this on a high-capacity filesystem (e.g., /volatile or /work)
singularity build ${image_dir}/jana2root.sif images/jana2root.def
```
Note this process takes several minutes and raises some harmless warnings. 


## 📊 Running the Benchmarks
The core logic resides in `plugins/RecoClusterVTP`. This plugin performs the following pipeline:

1. Data Ingestion: Loads raw/simulated events from a ROOT Tree.
2. VTP Reconstruction: Invokes C++ utility classes to reconstruct clusters using logic mirroring the FPGA hardware.
3. Monitoring: Registers and fills histograms.

### Build and Execution
Use the provided wrapper scripts. By default, these scripts use the Singularity container. Use the `-l` flag for local builds.
```
./build.sh [-l]
./run.sh [-l]
```

## Directory strucutre
- [**`plugins/RecoClusterVTP`**] : The JANA2 plugin wrapper and event processor logic.
- [**`src`**] : Core reconstruction utilities and VTP emulation logic.
- [**`images`**] : Definition files for Singularity/Apptainer containers.
- [**`database`**] : input files required for defining NPS geometry, module configurations.

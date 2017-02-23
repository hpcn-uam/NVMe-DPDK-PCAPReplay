#/bin/bash

# Sync submodules with NVMe DPDK-PCAPReplay tested version
git submodule update --init

# Compile dependencies
./buildScripts/buildDeps.sh

# Compile Application
./buildScripts/buildApp.sh



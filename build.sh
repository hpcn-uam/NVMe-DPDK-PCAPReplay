#/bin/bash

# Sync submodules with NVMe DPDK-PCAPReplay tested version
git pull
git submodule update --init --recursive

# Compile dependencies
./buildScripts/buildDeps.sh

# Compile Application
./buildScripts/buildApp.sh



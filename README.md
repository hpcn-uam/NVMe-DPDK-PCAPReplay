[In Development] NVMe-DPDK-Replay 
=================

This application allows to reproduce HUGE PCAP files at line rate for 10, 40 and 100 Gbps Links.

Clonning
-----------------
To clone this project execute:

````
git clone https://github.com/hpcn-uam/NVMe-DPDK-PCAPReplay.git
git submodule update --init --recursive
````

if you want to update (pull) to a newer commit, you should execute:

````
git pull
git submodule update --init --recursive
````

Dependencies-Compilation
----------------

### Automatic Compilation

Just execute:

`./buildDeps.sh`

### Manual Compilation

#### DPDK-Compilation
The latest tested DPDK-repository with this application is included in the `dpdk` folder.
Howerver, any other compatible-version could be used by exporting `RTE_SDK` variable.

To compile the included DPDK-release, it is recomended to execute and follow the basic `dpdk-setup.sh` script, example:

````
cd dpdk
./tools/dpdk-setup.sh
cd ..
````

#### SPDK-Compilation
Execute the following commands:

````
cd spdk
make DPDK_DIR=${RTE_SDK}/${RTE_TARGET} -j
sudo scripts/setup.sh
cd ..
````

APP-Compilation
-----------------
The application is compiled automatically when executing one of the provided scripts.
If you prefere to compile it manually, in the current folder there is a `Makefile` to do it.

Execution
-----------------
In `script` folder, there are some example scripts:

- `scripts/listFiles.sh` List the PCAP-files loaded in NVME raid
- `scripts/RemoveFile.sh` Remove a file from the NVME raid
- `scripts/AddFile.sh` Adds a file from the NVME raid
- `scripts/ReplayFile.sh` Replays a file from the NVME raid

#/bin/bash

##### TODO: ERROR-CONTROL

export RTE_SDK=$(pwd)/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc

cd src
make DPDK_DIR=${RTE_SDK}/${RTE_TARGET} 
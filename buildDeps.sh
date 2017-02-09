#/bin/bash

##### TODO: ERROR-CONTROL

export RTE_SDK=$(pwd)/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc

#DPDK
pushd ${RTE_SDK}

#Compile
make install T=${RTE_TARGET} -j

#LOAD IGB MODULE
sudo /sbin/modprobe uio
sudo /sbin/insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko

#BIND bind_devices
${RTE_SDK}/tools/dpdk-devbind.py --status
echo ""
echo -n "Enter PCI address of device to bind to IGB UIO driver: "
read PCI_PATH
sudo ${RTE_SDK}/tools/dpdk-devbind.py -b igb_uio $PCI_PATH && echo "OK"


#MOUNT HUGEPAGES
grep -s '/mnt/huge' /proc/mounts > /dev/null
if [ $? -ne 0 ] ; then
    sudo mount -t hugetlbfs nodev /mnt/huge
fi

popd

#SPDK
pushd spdk
make DPDK_DIR=${RTE_SDK}/${RTE_TARGET} -j
sudo scripts/setup.sh
popd
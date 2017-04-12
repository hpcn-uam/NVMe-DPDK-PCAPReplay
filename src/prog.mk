#
#  BSD LICENSE
#
#  Copyright (c) Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

SPDK_ROOT_DIR := $(abspath $(CURDIR)/../../spdk)
INCLUDE_DIR := $(abspath $(CURDIR)/../../include)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk

MECS =  $(wildcard *.c)
MEOBJ =  $(MECS:.c=)

C_SRCS = $(wildcard ../*.c) $(MECS:.c=.o)

APP = $(MEOBJ)

CFLAGS += -I $(INCLUDE_DIR) -g
LDFLAGS += -lpcap

#from SPDK
NVME_DIR := $(SPDK_ROOT_DIR)/lib/nvme
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.app.mk
include $(SPDK_ROOT_DIR)/lib/env_dpdk/env.mk

# DPDK deps
DPDK_LIB_LIST = rte_mbuf rte_eal rte_mempool rte_ring rte_pipeline rte_port rte_table rte_sched rte_timer rte_acl rte_hash rte_lpm rte_kni rte_net rte_ethdev rte_reorder rte_kvargs rte_ip_frag
# Drivers
DPDK_LIB_LIST += rte_pmd_fm10k rte_pmd_i40e rte_pmd_ixgbe

CFLAGS += -I. $(ENV_CFLAGS)
SPDK_LIB_LIST = nvme util log
LIBS += $(SPDK_LIB_LINKER_ARGS) $(ENV_LINKER_ARGS) -lrt -lm #-lrte_pmd_mlx5

ifeq ($(CONFIG_RDMA),y)
LIBS += -libverbs -lrdmacm
endif

all: $(APP)

$(APP) : $(OBJS) $(SPDK_LIB_FILES) $(ENV_LIBS)
	$(LINK_C)

clean:
	$(CLEAN_C) $(APP)

#include $(DPDK_DIR)/../mk/rte.app.mk
include $(SPDK_ROOT_DIR)/mk/spdk.deps.mk
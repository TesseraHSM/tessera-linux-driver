obj-m += tsra.o
tsra-y += tsra_main.o
tsra-y += tsra_ctrl.o
tsra-y += tsra_ctrl_cmd.o
tsra-y += tsra_ioq.o
tsra-y += tsra_ioq_pool.o
tsra-y += tsra_hsm.o
tsra-y += tsra_hsm_dev.o
tsra-y += tsra_hsm_cmd.o
tsra-y += tsra_aes.o
tsra-y += tsra_aes_dev.o
tsra-y += tsra_aes_cmd.o
tsra-y += tsra_dma_io.o
tsra-y += tsra_abort.o
tsra-y += tsra_ioq_util.o
tsra-y += tsra_mgmt_dev.o
tsra-y += tsra_ioq_perf.o

ccflags-y += -Werror

# Uncomment to enable the verbose ENTRY/EXIT/DEBUG driver logging in
# tsra_log.h for local debug work. Leave commented out for
# production builds.
# ccflags-y += -DDEBUG

# Kernel build tree to build against. Defaults to the running kernel's
# build directory. Override on the command line for cross-builds, e.g.
#   make KDIR=/path/to/kernel/source
# KERNEL_SRC is still honoured for backward compatibility.
KDIR ?= $(KERNEL_SRC)
KDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(CURDIR)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all modules_install clean

/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_TSRA_AES_H
#define _LINUX_TSRA_AES_H

#include <linux/kernel.h>
#include <linux/xarray.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/pci.h>

#include "tsra_ioq_pool.h"

struct tsra_ctrl;
#define TSRA_AES_DEV_NAME "tsra-aes"

#define AES_MIN_ID 256
extern int tsra_num_aes_slots;
#define AES_IOQ_SZ (tsra_num_aes_slots)
/* For now we are hardcoding the MSIx start to 1
 * assuming that we will always get 32 MSIx. This
 * will change if we need to be able to work with
 * less than 32 MSIx vectors.
 */

#define AEX_MSIX_START 16
struct tsra_aes_cfg {
	struct tsra_ctrl *ctrl;
	u16 ioq_id_start;
	u16 ioq_size;
	void *ioq_db;
	u16 msix_start;
	u16 msix_max_cnt;
};

struct tsra_aes {
	struct tsra_ctrl *ctrl;
	struct pci_dev *pdev;
	struct tsra_ioq_pool ioq_pool;
	struct mutex aes_lock;
	/*
	 * @dev_kobj: Pointer to kobject
	 * that we get after creating kobject
	 * and sysfs file
	 * This is specific to aes perf counters
	 */
	struct kobject *dev_kobj;
};

int tsra_aes_init(struct tsra_aes *aes, struct tsra_aes_cfg *aes_cfg,
		  const bool abort);

void tsra_aes_deinit(struct tsra_aes *aes, const bool abort);

#endif // _LINUX_TSRA_AES_H

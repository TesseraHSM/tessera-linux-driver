/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_TSRA_IOQ_POOL_H
#define _LINUX_TSRA_IOQ_POOL_H

#include <linux/kernel.h>
#include <linux/xarray.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "tsra_ioq.h"

struct tsra_ctrl;
struct tsra_ioq_pool;

struct tsra_ioq_pool_irq_ctx {
	struct tsra_ioq_pool *pool;
	int irq;
	struct tasklet_struct tasklet;
	struct xarray ioqs;
};

struct tsra_ioq_pool_cfg {
	struct tsra_ctrl *ctrl;
	struct pci_dev *pdev;
	const char *name;
	enum tsra_ioq_type ioq_type;
	u16 ioq_id_start;
	u16 ioq_size;
	void *ioq_db;
	struct tsra_ioq_ops *ioq_ops;
	u16 msix_start;
	u16 msix_max_cnt;
};

struct tsra_ioq_pool {
	struct tsra_ctrl *ctrl;
	struct pci_dev *pdev;
	const char *name;
	enum tsra_ioq_type ioq_type;
	/*
	 * ioq_select_id is used to
	 * select the next SQ to send
	 * the command out on
	 */
	u16 ioq_select_id;
	u16 ioq_start_idx;
	u16 ioq_size;
	void *ioq_db;
	struct tsra_ioq_ops *ioq_ops;
	u16 ioq_max_cnt;
	u16 ioq_cnt;
	u16 msix_start;
	u16 msix_max_cnt;
	u16 msix_cnt;
	struct xarray irqs;
	struct xarray ioqs;

	struct kobject *parent_kobj;
};

int tsra_ioq_pool_init(struct tsra_ioq_pool *ioq_pool,
		       struct tsra_ioq_pool_cfg *cfg);

void tsra_ioq_pool_deinit(struct tsra_ioq_pool *ioq_pool, const bool abort);

u16 tsra_ioq_pool_queue_cnt(struct tsra_ioq_pool *ioq_pool);

int tsra_ioq_pool_submit_cmd(struct tsra_ioq_pool *ioq_pool, u16 id, void *cmd);
int tsra_ioq_pool_create_queue_pair(struct tsra_ctrl *ctrl,
				    struct tsra_ioq *ioq);
void tsra_ioq_pool_delete_queue_pair(struct tsra_ctrl *ctrl,
				     struct tsra_ioq *ioq, const bool abort);

bool tsra_sq_is_full(struct tsra_ioq_sq *sq);

#endif // _LINUX_TSRA_IOQ_POOL_H

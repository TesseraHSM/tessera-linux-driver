/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_TSRA_HSM_DEV_H
#define _LINUX_TSRA_HSM_DEV_H

#include <linux/idr.h>
#include <linux/uaccess.h>
#include <linux/version.h>

struct tsra_hsm;

int tsra_hsm_dev_init(struct tsra_hsm *hsm, const bool abort);
int tsra_hsm_dev_alloc_minor(struct tsra_hsm *hsm);
int tsra_hsm_dev_dealloc_minor(struct tsra_hsm *hsm);

void tsra_hsm_dev_deinit(struct tsra_hsm *hsm, const bool abort);

#endif //_LINUX_TSRA_HSM_DEV_H

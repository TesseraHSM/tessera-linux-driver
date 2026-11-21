/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_TSRA_MGMT_IF_DEV_H
#define _LINUX_TSRA_MGMT_IF_DEV_H

#include <linux/idr.h>
#include <linux/uaccess.h>
#include <linux/version.h>

struct tsra_mgmt;

int tsra_mgmt_if_dev_init(struct tsra_mgmt *mgmt, const bool abort);

void tsra_mgmt_if_dev_deinit(struct tsra_mgmt *mgmt, const bool abort);

#endif //_LINUX_TSRA_MGMT_IF_DEV_H

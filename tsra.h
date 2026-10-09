/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_TSRA_H
#define _LINUX_TSRA_H

#include <linux/kernel.h>
#include <linux/pci.h>
#include "tsra_ctrl.h"
#include "tsra_mgmt.h"
#include "tsra_mgmt_if_dev.h"
#include "tsra_log.h"
#include "tsra_abort.h"

#define TSRA_DRIVER_VERSION "3.2.120.1"

struct tsra_dev {
	struct pci_dev *pdev;
	void __iomem *bar0;
	void __iomem *bar2;
	void __iomem *bar4;
	size_t irq_cnt;
	struct tsra_ctrl ctrl;
	struct tsra_mgmt mgmt;
};

int __init tsra_ctrl_dev_mod_init(void);
void __exit tsra_ctrl_dev_mod_exit(void);

int __init tsra_hsm_dev_mod_init(void);
void __exit tsra_hsm_dev_mod_exit(void);

int __init tsra_aes_dev_mod_init(void);

int __init tsra_mgmt_if_dev_mod_init(void);
void __exit tsra_mgmt_if_dev_mod_exit(void);

#endif // _LINUX_TSRA_H

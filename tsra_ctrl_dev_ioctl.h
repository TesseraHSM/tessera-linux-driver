/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_TSRA_CTRL_DEV_IOCTL_H
#define _LINUX_TSRA_CTRL_DEV_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define TSRA_CTRL_DEV_INFO_PI_LEN (32 + 1)
#define TSRA_CTRL_DEV_INFO_SN_LEN (32 + 1)
#define TSRA_CTRL_DEV_INFO_MN_LEN (4 + 1)
#define TSRA_CTRL_DEV_INFO_FR_LEN (32 + 1)
#define TSRA_CTRL_DEV_INFO_DR_LEN (15 + 1)

#define TSRA_CTRL_DEV_INFO_ENTROPY_LENGTH 32

struct tsra_ctrl_dev_info {
	__u32 argsz;
	__u16 id;
	char pci_info[TSRA_CTRL_DEV_INFO_PI_LEN];
	char serial_num[TSRA_CTRL_DEV_INFO_SN_LEN];
	char model_num[TSRA_CTRL_DEV_INFO_MN_LEN];
	char firmware_rev[TSRA_CTRL_DEV_INFO_FR_LEN];
	char driver_rev[TSRA_CTRL_DEV_INFO_DR_LEN];
	char rsvd[17];
	char device_entropy[TSRA_CTRL_DEV_INFO_ENTROPY_LENGTH];
};

struct tsra_ctrl;
int tsra_ctrl_dev_get_dev_info(struct tsra_ctrl *ctrl,
			       struct tsra_ctrl_dev_info *dev_info);

#endif //_LINUX_TSRA_CTRL_DEV_IOCTL_H

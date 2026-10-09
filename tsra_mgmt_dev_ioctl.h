/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_TSRA_MGMT_DEV_IOCTL_H
#define _LINUX_TSRA_MGMT_DEV_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include "tsra_ctrl_dev_ioctl.h"

#define TSRA_MGMT_STATUS_SUCCESS 0
#define TSRA_MGMT_STATUS_NOT_PF 1
#define TSRA_MGMT_STATUS_INVALID_VF_IDX 2
#define TSRA_MGMT_STATUS_RES_CNT_ALREADY_SET 3
#define TSRA_MGMT_STATUS_SET_RES_CNT_FAILED 4
#define TSRA_MGMT_STATUS_RES_CNT_NOT_SET 5
#define TSRA_IOCTL_MGMT_STATUS_DEVICE_INFO_INVALID_BUF_SIZE 6
#define TSRA_IOCTL_MGMT_STATUS_GET_RES_CNT_FAILED 7

struct tsra_mgmt_if_res_cnt {
	__u32 argsz;
	__u16 vf_idx;
	__u16 res_cnt;
	__u32 status;
};

/*
 * IOCTL TSRA_CTRL_DEV_IOCTL_SET_RES_CNT to set resource count
 *  for a given VF
 *  This ioctl is only available for management plane applications
 * This ioctl is only valid for a PF
 */

#define TSRA_MGMT_IF_DEV_IOCTL_SET_RES_CNT \
	_IOWR('A', 0x2, struct tsra_mgmt_if_res_cnt)

/*
 * IOCTL TSRA_MGMT_IF_DEV_IOCTL_GET_RES_CNT
 * returns the resource count allocated for a VF
 * Only PF (for now) can issue this call.
 * Only available on the management interface
 */
#define TSRA_MGMT_IF_DEV_IOCTL_GET_RES_CNT \
	_IOWR('A', 0x3, struct tsra_mgmt_if_res_cnt)

struct tsra_mgmt_if_get_vf_count {
	__u32 argsz;
	__u16 vf_count;
};

/*
 * TSRA_MGMT_IF_DEV_IOCTL_GET_VF_COUNT
 * Returns the number of VFs configured in the PF
 * Only available on the management interface
 * Only PF driver can issue this ioctl
 */
#define TSRA_MGMT_IF_DEV_IOCTL_GET_VF_COUNT \
	_IOWR('A', 0x4, struct tsra_mgmt_if_get_vf_count)

/*
 * TSRA_MGMT_IF_DEV_IOCTL_GET_DEV_INFO
 * Available on the management interface
 * Only PF can issue this call.
 *
 * Note that there is a similar ioctl available
 * on the general interface that a VF driver can
 * call
 */
#define TSRA_MGMT_IF_DEV_IOCTL_GET_DEV_INFO \
	_IOWR('A', 0x5, struct tsra_ctrl_dev_info)
#endif //_LINUX_TSRA_MGMT_DEV_IOCTL_H

/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_TSRA_HSM_H
#define _LINUX_TSRA_HSM_H

#include <linux/kernel.h>
#include <linux/xarray.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "tsra_ioq_pool.h"
#include "tsra_hsm_cmd.h"

#define TSRA_HSM_DEV_NAME "tsra-hsm"
extern int tsra_num_hsm_slots;
#define HSM_IOQ_SZ (tsra_num_hsm_slots)
#define HSM_MIN_ID 1

struct tsra_ctrl;

struct tsra_hsm_cfg {
	struct tsra_ctrl *ctrl;
	u16 ioq_id_start;
	u16 ioq_size;
	void *ioq_db;
	u16 msix_start;
	u16 msix_max_cnt;
};

#define TSRA_HSM_ATTRIBUTE_COUNT 3
#define TSRA_GLOBAL_ATTRIBUTE_COUNT 6

/*
 * Definition of indices into
 * the admin and global attributes
 */
#define TSRA_HSM_ADMIN_ATTRIBUTE_NUM_CPQS_CREATED 0
#define TSRA_HSM_ADMIN_ATTRIBUTE_NUM_FPQS_CREATED 1
#define TSRA_HSM_ADMIN_ATTRIBUTE_NUM_Q_DELETE_CMDS 2

/*
 * global attribute
 * Number of completions with errors (across all
 * queues)
 */
#define TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_ERROR_COMPLETIONS 0

/*
 * Total submissions to HW across all queues
 */
#define TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_SUBMISSIONS_TO_HW 1

/*
 * Total completions from HW across all queues
 */
#define TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_COMPLETIONS_FROM_HW 2

/*
 * total aborts that are elevated to level 2
 */
#define TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_LVL_2_ABORTS 3

/*
 * Total commands that are aborted
 */
#define TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_CMDS_ABORTED 4

/*
 * Total level1 aborts (across all IOQs)
 */
#define TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_LVL_1_ABORTS 5

struct hsm_attribute_info {
	struct kobj_attribute attribute;
	void *context;
	atomic_t counter;
};

struct tsra_hsm {
	struct pci_dev *pdev;
	struct tsra_ctrl *ctrl;
	struct tsra_ioq_pool ioq_pool;
	struct cdev cdev;
	struct device *cdev_dev;
	unsigned int major;
	unsigned int minor;
	struct dma_pool *page_pool;

	/*
	 * @sysfs_name: name as it appears
	 * in sysfs
	 */
	char sysfs_name[32];
	/*
	 * @dev_kobj: Pointer to kobject
	 * that we get after creating kobject
	 * and sysfs file
	 */
	struct kobject *dev_kobj;

	/*
	 * @admin_group_kobj: kobject for representing
	 * admin attributes
	 */
	struct kobject *admin_group_kobj;

	/*
	 * @global_group_kobj: kobject for representing
	 * global attributes
	 */
	struct kobject *global_group_kobj;

	/*
	 * @admin_kobject_array: array of kobjects for
	 *    surfacing admin counters
	 */
	struct hsm_attribute_info
		hsm_admin_attribute_array[TSRA_HSM_ATTRIBUTE_COUNT];
	/* 1 + total attributes. The last one is NULL when creating a group*/
	struct attribute *p_admin_attrib_array[TSRA_HSM_ATTRIBUTE_COUNT + 1];
	struct attribute_group admin_attribute_group;

	/*
	 * Array of kobjects for surfacing global counters
	 */
	struct hsm_attribute_info
		hsm_global_attribute_array[TSRA_GLOBAL_ATTRIBUTE_COUNT];
	/* 1 + total attributes. The last one is NULL when creating a group*/
	struct attribute *p_global_attrib_array[TSRA_GLOBAL_ATTRIBUTE_COUNT + 1];
	struct attribute_group global_attribute_group;

	/* values for global times */
	s64 global_min_completion_time;
	s64 global_max_completion_time;
	s64 global_average_completion_time;

	struct mutex hsm_lock;
};

int tsra_hsm_init(struct tsra_hsm *hsm, struct tsra_hsm_cfg *cfg,
		  const bool abort);

void tsra_hsm_deinit(struct tsra_hsm *hsm, const bool abort);

int tsra_hsm_generic_cmd_process(struct tsra_hsm *hsm,
				 struct tsra_hsm_generic_cmd *cmd);

void tsra_hsm_force_close_session(struct tsra_hsm *hsm, const u16 session_id);

#define TSRA_MAX_SESSIONS_PER_FD 1

/*
 * Structure holding per session information
 * id : 16-bit session id
 * valid : Indicates if session id is valid
 * short_app_id :- Short app id
 * short_app_id_is_valid :- If 1 indicates short app id
 *   is valid
 */
struct tsra_per_session_info {
	u16 id;
	bool valid;
	u8 short_app_id;
	bool short_app_id_is_valid;
};

/*
 * structure tsra_hsm_fd_ctxt
 * Context that is created and maintained on
 * a per file handle basis
 *
 * If a task wants to create and process more
 * than one session, it will have to open multiple
 * handles
 *
 * hsm ---> Pointer to hsm
 * owning_task ---> PID of the task which created the context
 * sessions --> Array of sessions that are opened in the context
 *     of this file handle.
 *     Only one outstanding session is allowed per file context
 * lock        ---> lock to protect the context
 */
struct tsra_hsm_fd_ctxt {
	struct tsra_hsm *hsm;
	pid_t owning_task;
	struct tsra_per_session_info sessions[TSRA_MAX_SESSIONS_PER_FD];
	struct mutex lock;
};

#endif // _LINUX_TSRA_HSM_H

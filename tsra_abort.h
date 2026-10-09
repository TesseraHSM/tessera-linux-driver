/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_TSRA_ABORT_H
#define _LINUX_TSRA_ABORT_H

#include <linux/kernel.h>
#include <linux/sbitmap.h>
#include <linux/xarray.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include "tsra_ioq.h"
#include "tsra_ctrl.h"
#include "tsra_hsm_dev_ioctl.h"

// Internal Status For Handling Command Aborts
enum IOQ_CMD_INTERNAL_STS {
	TSRA_IOQ_CMD_STS_SUCCESS = 0,
	TSRA_IOQ_CMD_STS_ABORTED = 1,
	TSRA_IOQ_CMD_STS_ABORT_IN_PROGRESS = 2,
	TSRA_IOQ_CMD_STS_QSELECT_FAILED = 3,
	TSRA_IOQ_CMD_STS_QDISABLED = 4,
	TSRA_IOQ_CMD_STS_UNDEFINED = 5,
	TSRA_CTRL_NOT_READY = 6, // Controller is Not in ready state
};

#define ABORT_TYPE_TIMEOUT ABORT_TYPE_RESERVED
#define PERFORM_L1_ABORT(_abort_type) (_abort_type == ABORT_TYPE_TIMEOUT)

/*
 * The timer resolution of the health monitor timer
 */
#define TSRA_HEALTH_MON_TIME msecs_to_jiffies(6000)

/* abort status codes*/
#define TSRA_IOQ_LEVEL_ONE_ABORT_SUCCESS 0
#define TSRA_IOQ_ABORT_IN_PROGRESS 1
#define TSRA_IOQ_ABORT_LEVEL_ONE_FAILED 2
#define TSRA_ABORT_LEVEL_TWO_FAILED 3

int tsra_abort(struct tsra_ctrl *ctrl, struct tsra_ioq *ioq,
	       struct completion *completion_object, bool crash, u32 abort_type,
	       bool called_from_hmon);

/**
 * tsra_ctrl_cmd_delete_sq(). Delete a SQ
 * @ctrl : Ctrl interface
 * @id : Id of the queue
 */
int tsra_ctrl_cmd_delete_sq(struct tsra_ctrl *ctrl, u16 id);

void *tsra_ioq_store_ctx(struct tsra_ioq_store *store, u16 tag);

void tsra_ioq_store_free_ctx(struct tsra_ioq_store *store, u16 tag);

void tsra_disable_cp_queues(struct tsra_ctrl *ctrl);
void tsra_disable_fp_queues(struct tsra_ctrl *ctrl);

void tsra_ctrl_flush_cmds_from_cp_queues(struct tsra_ctrl *ctrl);
void tsra_ctrl_flush_cmds_from_fp_queues(struct tsra_ctrl *ctrl);
void tsra_disable_all_queues_in_pool(struct device *dev,
				     struct tsra_ioq_pool *pool);
void tsra_flush_all_commands_on_ioq(struct tsra_ctrl *ctrl,
				    struct tsra_ioq *ioq,
				    const int completion_status);
void tsra_ctrl_flush_cmds_from_ioqs(struct tsra_ctrl *ctrl,
				    struct tsra_ioq_pool *pool);
/*
 * Check whether ioq is disabled or not
 * Caller must be holding the ioq->submit_lock
 * before calling this function
 */
bool tsra_is_ioq_disabled(struct tsra_ioq *ioq);

void tsra_health_monitor(struct work_struct *work);

#endif // _LINUX_TSRA_ABORT_H

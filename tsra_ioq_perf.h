/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_TSRA_IOQ_PERF_H
#define _LINUX_TSRA_IOQ_PERF_H

#include "tsra_hsm.h"
#include "tsra_hsm_cmd.h"
#include "tsra_hsm_dev_ioctl.h"
#include "tsra_abort.h"
#include <linux/dmapool.h>
#include "tsra_ioq_util.h"

void tsra_ioq_perf_update_cntrs_before_submission(struct tsra_hsm *hsm,
						  struct tsra_ioq *ioq);

void tsra_ioq_perf_update_cntrs_after_submission(struct tsra_hsm *hsm,
						 struct tsra_ioq *ioq,
						 ktime_t *submission_time,
						 ktime_t *completion_time);

#endif // _LINUX_TSRA_IOQ_PERF_H

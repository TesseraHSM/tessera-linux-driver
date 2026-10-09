// SPDX-License-Identifier: GPL-2.0-or-later

#include "tsra_aes.h"
#include "tsra_aes_cmd.h"
#include "tsra_aes_dev.h"
#include "tsra_ctrl.h"
#include "tsra_log.h"

#define TSRA_AES_DMA_PAGE_SIZE SZ_4K

/*
 * Functions Local to This File
 */
static size_t tsra_aes_ioq_ops_page_size(void)
{
	return TSRA_AES_DMA_PAGE_SIZE;
}

static void tsra_aes_ioq_ops_set_io_data(void *cmd, unsigned int data)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	aes_cmd->io_data = data;
}

static unsigned int tsra_aes_ioq_ops_get_io_data(void *cmd)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	return aes_cmd->io_data;
}

static u16 tsra_aes_ioq_ops_cqe_size(void)
{
	return TSRA_AES_CQE_SZ;
}

static void *tsra_aes_ioq_ops_cqe(void *cmd)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	return &aes_cmd->cqe;
}

static u16 tsra_aes_ioq_ops_cqe_get_cid(void *cqe)
{
	struct tsra_aes_cqe *aes_cqe = cqe;

	return aes_cqe->cmd_id;
}

static u16 tsra_aes_ioq_ops_cqe_get_phase(void *cqe)
{
	struct tsra_aes_cqe *aes_cqe = cqe;

	return aes_cqe->ph_sts.ph_sts_bits.phase;
}

static u16 tsra_aes_ioq_ops_cqe_get_sq_head(void *cqe)
{
	struct tsra_aes_cqe *aes_cqe = cqe;

	return aes_cqe->sq_head;
}

static u16 tsra_aes_ioq_ops_sqe_size(void)
{
	return TSRA_AES_SQE_SZ;
}

static void *tsra_aes_ioq_ops_sqe(void *cmd)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	return &aes_cmd->sqe;
}

static void tsra_aes_ioq_ops_sqe_set_cid(void *sqe, u16 cid)
{
	struct tsra_aes_sqe *aes_sqe = sqe;

	aes_sqe->cmd_id = cid;
}

static int tsra_aes_ioq_ops_complete_cmd(void *cmd, const int completion_status)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	aes_cmd->completion_status = completion_status;
	complete(&aes_cmd->cmpl);
	return 0;
}

static int tsra_aes_ioq_ops_get_cmd_tag(void *cmd)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	return aes_cmd->tag;
}

static void tsra_aes_ioq_ops_set_cmd_tag(void *cmd, const int tag)
{
	struct tsra_aes_cmd *aes_cmd = cmd;

	aes_cmd->tag = tag;
}

/*
 * Functions Exposed From This File
 */

static struct tsra_ioq_ops tsra_aes_ioq_ops = {
	.page_size = tsra_aes_ioq_ops_page_size,
	.set_io_data = tsra_aes_ioq_ops_set_io_data,
	.get_io_data = tsra_aes_ioq_ops_get_io_data,
	.cqe_size = tsra_aes_ioq_ops_cqe_size,
	.cqe = tsra_aes_ioq_ops_cqe,
	.cqe_get_cid = tsra_aes_ioq_ops_cqe_get_cid,
	.cqe_get_phase = tsra_aes_ioq_ops_cqe_get_phase,
	.cqe_get_sq_head = tsra_aes_ioq_ops_cqe_get_sq_head,
	.sqe_size = tsra_aes_ioq_ops_sqe_size,
	.sqe = tsra_aes_ioq_ops_sqe,
	.sqe_set_cid = tsra_aes_ioq_ops_sqe_set_cid,
	.complete_cmd = tsra_aes_ioq_ops_complete_cmd,
	.get_tag = tsra_aes_ioq_ops_get_cmd_tag,
	.set_tag = tsra_aes_ioq_ops_set_cmd_tag,
};

int tsra_aes_init(struct tsra_aes *aes, struct tsra_aes_cfg *aes_cfg,
		  const bool abort)
{
	int err;
	struct tsra_ioq_pool_cfg ioq_pool_cfg = { 0 };

	aes->pdev = aes_cfg->ctrl->pdev;

	ioq_pool_cfg.name = TSRA_AES_DEV_NAME;
	ioq_pool_cfg.ctrl = aes_cfg->ctrl;
	ioq_pool_cfg.pdev = aes_cfg->ctrl->pdev;
	ioq_pool_cfg.ioq_db = aes_cfg->ioq_db;
	ioq_pool_cfg.ioq_id_start = aes_cfg->ioq_id_start;
	ioq_pool_cfg.ioq_size = aes_cfg->ioq_size;
	ioq_pool_cfg.ioq_type = TSRA_IOQ_TYPE_AES;
	ioq_pool_cfg.ioq_ops = &tsra_aes_ioq_ops;
	ioq_pool_cfg.msix_start = aes_cfg->msix_start;
	ioq_pool_cfg.msix_max_cnt = aes_cfg->msix_max_cnt;

	TSRA_DEV_LOG_ENTRY(
		&aes_cfg->ctrl->pdev->dev,
		"[ENTRY] %s aes:%p ioq_id_start:%d ioq_size:%d MSI-X start:%d MSI-X max:%d\n",
		__func__, aes, aes_cfg->ioq_id_start, aes_cfg->ioq_size,
		aes_cfg->msix_start, aes_cfg->msix_max_cnt);

	if (false == abort)
		mutex_init(&aes->aes_lock);

	aes->ioq_pool.parent_kobj = aes->dev_kobj;

	err = tsra_ioq_pool_init(&aes->ioq_pool, &ioq_pool_cfg);
	if (err) {
		TSRA_DEV_LOG_ERROR(&aes_cfg->ctrl->pdev->dev,
				   "Failed To Create IOQ Pool");
		return err;
	}

	/* do not create soft device if the number of queues is 0*/
	if (aes->ioq_pool.ioq_max_cnt == 0) {
		TSRA_DEV_LOG_ERROR(
			&aes->pdev->dev,
			"Device has 0 AES queues. Not creating AES device link\n");
		return 0;
	}

	atomic_add(aes->ioq_pool.ioq_max_cnt,
		   &aes->ctrl->hsm
			    .hsm_admin_attribute_array
				    [TSRA_HSM_ADMIN_ATTRIBUTE_NUM_FPQS_CREATED]
			    .counter);

	TSRA_DEV_LOG_EXIT(&aes_cfg->ctrl->pdev->dev, "[EXIT] %s aes:%p\n",
			  __func__, aes);

	return 0;
}

void tsra_aes_deinit(struct tsra_aes *aes, const bool abort)
{
	TSRA_LOG_ENTRY("%s aes:%p\n", __func__, aes);
	tsra_ioq_pool_deinit(&aes->ioq_pool, abort);
	TSRA_LOG_EXIT("%s aes:%p\n", __func__, aes);
}

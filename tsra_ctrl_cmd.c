// SPDX-License-Identifier: GPL-2.0-or-later

#include "tsra_ctrl_cmd.h"
#include "tsra_ctrl.h"
#include "tsra_abort.h"
#include <linux/dmapool.h>
#include "tsra_log.h"

#define TSRA_CTRL_CMD_TIME_OUT (30 * HZ)

static void tsra_ctrl_cmd_init(struct tsra_ctrl_cmd *cmd, u8 op,
			       dma_addr_t prp1, dma_addr_t prp2)
{
	init_completion(&cmd->cmpl);
	cmd->sqe.any.hdr.opc = op;
	cmd->sqe.any.hdr.psdt = 0;
	cmd->sqe.any.hdr.dptr.prp.fst = prp1;
	cmd->sqe.any.hdr.dptr.prp.snd = prp2;
}

static void tsra_ctrl_dump_ctrl_sqe(struct tsra_ctrl *ctrl,
				    struct tsra_ctrl_cmd *cmd)
{
	struct device *dev = &ctrl->pdev->dev;

	TSRA_DEV_LOG_ENTRY(
		dev,
		"CmdId:0x%x Opc:0x%x Prp[1st,2nd]:0x%llx 0x%llx mptr:0x%llx ",
		cmd->sqe.any.hdr.cid, cmd->sqe.any.hdr.opc,
		cmd->sqe.any.hdr.dptr.prp.fst, cmd->sqe.any.hdr.dptr.prp.snd,
		cmd->sqe.any.hdr.mptr);
}

static int tsra_ctrl_cmd_process(struct tsra_ctrl *ctrl,
				 struct tsra_ctrl_cmd *cmd)
{
	int err;
	u16 tag;
	u8 op = cmd->sqe.any.hdr.opc;
	struct device *dev = &ctrl->pdev->dev;
	unsigned long timeout = TSRA_CTRL_CMD_TIME_OUT;
	ktime_t start_time, end_time;
	s64 elapsed_time;

	cmd->completion_status = TSRA_IOQ_CMD_STS_UNDEFINED;

	atomic_inc(
		&ctrl->hsm
			 .hsm_global_attribute_array
				 [TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_SUBMISSIONS_TO_HW]
			 .counter);
	start_time = ktime_get();

	/* Acquire the lock on the SQ */
	mutex_lock(&ctrl->ioq.submit_lock);

	if (tsra_sq_is_full(&ctrl->ioq.sq)) {
		/*
		 * Controller [Admin] Submission Queue is full.
		 * This should never happen ever. Admin commands
		 * fired are single threaded in nature hence we
		 * should never hit the queue full condition ever.
		 * This could indicate that  the head pointer is not
		 * incremented correctly or rolled backwards.
		 */
		TSRA_DEV_LOG_ERROR(
			dev,
			"Controller Queue Is Full. Failed To Fire The Command\n");

		tsra_ctrl_dump_ctrl_sqe(ctrl, cmd);
		mutex_unlock(&ctrl->ioq.submit_lock);
		return err;
	}

	err = tsra_ioq_submit_cmd(&ctrl->ioq, cmd, &tag);
	mutex_unlock(&ctrl->ioq.submit_lock);

	if (err) {
		TSRA_DEV_LOG_ERROR(dev,
				   "Command submission failure. op=%d err=%d\n",
				   op, err);
		goto submit_fail;
	}

	/*
	 * NOTE Using The Interruptable versions of functions.
	 * At this point we are waiting for the command to be completed
	 * by the hardware. If we use the interruptable versions the wait
	 * will be interrupted and we will release the hardware buffers.
	 * The two contending parts of the code are the freeing of the dma
	 * buffers and the copy_to_user.
	 * Consider the following points:-
	 *
	 * 1. The freeing of dma buffers can only happen after the hardware
	 *    is done with dma and the copy to user is completed. copy_to_user
	 *    cannot be done in the bottom half and needs to be done in the
	 *    context of the user thread.
	 * 2. If the wait is interrupted in between, we will still need to keep
	 *    the buffers around as the hardware is touching the buffers. We can
	 *    keep the buffers around, and use a lazy free scheme to free those
	 *    but we still cannot return from this function because the
	 *    command information [tsra_hsm_generic_cmd] is allocated on the stack.
	 *    The completion is the part of this cmd structure.
	 * 3. Theinterruptable versions of these functions are usually used
	 *	  for hardware where the commands can be aborted deterministically.
	 *	  When the wait is interrupted, the commands will be aborted and then the
	 *    ioctl will be completed.
	 * /linux-source-5.15.0/drivers/spi/spi-tegra20-slink.c
	 */

	err = wait_for_completion_timeout(&cmd->cmpl, timeout);
	if (err == 0) {
		err = -ETIMEDOUT;
		TSRA_DEV_LOG_ERROR(dev,
				   "Command timeout. op=%d cid=%d err=%d\n", op,
				   tag, err);
		goto wait_fail;
	}

	if (cmd->completion_status == TSRA_IOQ_CMD_STS_ABORTED) {
		/* This command was aborted on this SQ
		 *  so return it back to caller
		 */
		TSRA_DEV_LOG_ERROR(dev, "Control Command aborted on IOQ:%d\n",
				   ctrl->ioq.id);

		/* TODO what do we do here.
		 *  Control IOQ is aborted.
		 */
		return -EAGAIN;
	} /* Else command completed successfully */

	end_time = ktime_get();
	elapsed_time = ktime_to_ns(ktime_sub(end_time, start_time));

	if (elapsed_time >= ctrl->hsm.global_max_completion_time)
		ctrl->hsm.global_max_completion_time = elapsed_time;

	if (!ctrl->hsm.global_min_completion_time)
		ctrl->hsm.global_min_completion_time = elapsed_time;
	else if (elapsed_time <= ctrl->hsm.global_min_completion_time)
		ctrl->hsm.global_min_completion_time = elapsed_time;

	atomic_inc(
		&ctrl->hsm
			 .hsm_global_attribute_array
				 [TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_COMPLETIONS_FROM_HW]
			 .counter);

	if (cmd->cqe.psf.fld.sc != 0) {
		atomic_inc(
			&ctrl->hsm
				 .hsm_global_attribute_array
					 [TSRA_HSM_GLOBAL_ATTRIBUTE_TOTAL_ERROR_COMPLETIONS]
				 .counter);
		err = -EFAULT;
		TSRA_DEV_LOG_ERROR(
			dev, "Command unsuccessful. op=%d cid=%d status=%d\n",
			op, tag, cmd->cqe.psf.fld.sc);
		goto dev_fault;
	}

	return 0;

wait_fail:
	tsra_ioq_cancel_cmd(&ctrl->ioq, tag);
	err = -EAGAIN;
submit_fail:
dev_fault:
	return err;
}

int tsra_ctrl_cmd_ident(struct tsra_ctrl *ctrl, dma_addr_t prp1)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret = 0;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev, "[ENTRY] %s tsra_ctrl:%p\n",
			   __func__, ctrl);

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_IDENT, prp1, 0);
	cmd.sqe.ident.cns = TSRA_CTRL_CMD_CNS_CTRL;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);

	TSRA_DEV_LOG_EXIT(&ctrl->pdev->dev, "[EXIT] %s tsra_ctrl:%p ret:%d\n",
			  __func__, ctrl, ret);
	return ret;
}

int tsra_ctrl_cmd_set_feat(struct tsra_ctrl *ctrl, u32 feat_id,
			   union tsra_ctrl_cmd_feat_data data)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret = 0;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev,
			   "[ENTRY] %s tsra_ctrl:%p feature id:%d\n", __func__,
			   ctrl, feat_id);

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_SET_FEAT, 0, 0);
	cmd.sqe.set_feat.feat_id = feat_id;
	cmd.sqe.set_feat.data = data;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	TSRA_DEV_LOG_EXIT(&ctrl->pdev->dev, "[EXIT] %s tsra_ctrl:%p ret:%d\n",
			  __func__, ctrl, ret);
	return ret;
}

int tsra_ctrl_cmd_get_feat(struct tsra_ctrl *ctrl, u32 feat_id,
			   union tsra_ctrl_cmd_feat_data *data)
{
	int err;
	struct tsra_ctrl_cmd cmd = { 0 };

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_GET_FEAT, 0, 0);
	cmd.sqe.set_feat.feat_id = feat_id;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev,
			   "[ENTRY] %s tsra_ctrl:%p feat_id:%d\n", __func__,
			   ctrl, feat_id);

	err = tsra_ctrl_cmd_process(ctrl, &cmd);
	if (err)
		goto err;

	data->val = cmd.cqe.cs.val;

	TSRA_DEV_LOG_EXIT(&ctrl->pdev->dev,
			  "[SUCCESS:EXIT] %s tsra_ctrl:%p feat_id:%d val:%d\n",
			  __func__, ctrl, feat_id, data->val);

	return 0;

err:

	return err;
}

int tsra_ctrl_cmd_set_res_cnt(struct tsra_ctrl *ctrl, u16 ctrl_id, u16 cnt)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret = 0;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev,
			   "[ENTRY] %s tsra_ctrl:%p ctrl_id:%d cnt:%d\n",
			   __func__, ctrl, ctrl_id, cnt);

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_SET_RES_CNT, 0, 0);
	cmd.sqe.set_res_cnt.ctrl_id = ctrl_id;
	cmd.sqe.set_res_cnt.cnt = cnt;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	TSRA_DEV_LOG_EXIT(&ctrl->pdev->dev,
			  "[EXIT] %s tsra_ctrl:%p ctrl_id:%d cnt:%d ret:%d\n",
			  __func__, ctrl, ctrl_id, cnt, ret);
	return ret;
}

int tsra_ctrl_cmd_get_res_cnt(struct tsra_ctrl *ctrl, u16 ctrl_id, u16 *cnt)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret = 0;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev,
			   "[ENTRY] %s tsra_ctrl:%p ctrl_id:%d\n", __func__,
			   ctrl, ctrl_id);

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_GET_RES_CNT, 0, 0);
	cmd.sqe.set_res_cnt.ctrl_id = ctrl_id;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	if (ret) {
		/* failure is logged by caller */
		return ret;
	}
	*cnt = cmd.cqe.cs.val;
	return ret;
}

int tsra_ctrl_cmd_set_hsm_queue_cnt(struct tsra_ctrl *ctrl, u16 *cnt)
{
	int err;
	struct tsra_ctrl_cmd cmd = { 0 };
	struct device *dev = &ctrl->pdev->dev;

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_SET_FEAT, 0, 0);
	cmd.sqe.set_feat.feat_id = TSRA_CTRL_CMD_FEAT_ID_HSM_QUEUE_CNT;
	cmd.sqe.set_feat.data.queue_cnt.cq_cnt = *cnt;
	cmd.sqe.set_feat.data.queue_cnt.sq_cnt = *cnt;

	*cnt = 0; // Clear the return queue count

	TSRA_DEV_LOG_ENTRY(dev, "[ENTRY] %s tsra_ctrl:%p HSM queue count:%d\n",
			   __func__, ctrl, *cnt);

	err = tsra_ctrl_cmd_process(ctrl, &cmd);
	if (err) {
		TSRA_DEV_LOG_ERROR(dev, "set hsm queue count failed. err=%d",
				   err);
		goto set_queue_cnt_fail;
	}

	//
	// Firmware returns a zero based queue count so lets add 1 to this
	// and return so that the caller does not need to adjust the
	// returned queue count
	//
	*cnt = TSRA_MIN((u16)cmd.cqe.cs.queue_cnt.cq,
			(u16)cmd.cqe.cs.queue_cnt.sq);
	*cnt += 1;

	TSRA_DEV_LOG_EXIT(dev, "[EXIT] %s tsra_ctrl:%p HSM Queue Count %d",
			  __func__, ctrl, *cnt);

	return 0;

set_queue_cnt_fail:
	return err;
}

int tsra_ctrl_cmd_set_aes_queue_cnt(struct tsra_ctrl *ctrl, u16 *cnt)
{
	int err;
	struct tsra_ctrl_cmd cmd = { 0 };
	struct device *dev = &ctrl->pdev->dev;

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_SET_FEAT, 0, 0);
	cmd.sqe.set_feat.feat_id = TSRA_CTRL_CMD_FEAT_ID_AES_QUEUE_CNT;
	cmd.sqe.set_feat.data.queue_cnt.cq_cnt = *cnt;
	cmd.sqe.set_feat.data.queue_cnt.sq_cnt = *cnt;

	TSRA_DEV_LOG_ENTRY(dev, "[ENTRY] %s tsra_ctrl:%p AES queue count:%d\n",
			   __func__, ctrl, *cnt);

	*cnt = 0; // Set the output queue count to zero
	err = tsra_ctrl_cmd_process(ctrl, &cmd);
	if (err) {
		TSRA_DEV_LOG_ERROR(dev, "set aes queue count failed. err=%d",
				   err);
		goto set_queue_cnt_fail;
	}

	//
	// Firmware returns a zero based queue count so lets add 1 to this
	// and return so that the caller does not need to adjust the
	// returned queue count

	*cnt = TSRA_MIN((u16)cmd.cqe.cs.queue_cnt.cq,
			(u16)cmd.cqe.cs.queue_cnt.sq);
	*cnt += 1;

	TSRA_DEV_LOG_EXIT(dev, "[EXIT] %s tsra_ctrl:%p AES Queue count %d",
			  __func__, ctrl, *cnt);
	return 0;

set_queue_cnt_fail:
	return err;
}

int tsra_ctrl_cmd_create_cq(struct tsra_ctrl *ctrl, dma_addr_t prp1, u16 id,
			    u16 size, u16 vec)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret;
	struct device *dev = &ctrl->pdev->dev;

	TSRA_DEV_LOG_ENTRY(
		dev, "[ENTRY] %s tsra_ctrl:%p cq id:%d cq size:%d vec:%d\n",
		__func__, ctrl, id, size, vec);

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_CREATE_CQ, prp1, 0);
	cmd.sqe.create_cq.id = id;
	cmd.sqe.create_cq.size = size - 1;
	cmd.sqe.create_cq.ien = true;
	cmd.sqe.create_cq.ivec = vec;
	cmd.sqe.create_cq.pc = true;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	TSRA_DEV_LOG_EXIT(dev, "[ENTRY] %s tsra_ctrl:%p cq id:%d ret:%d\n",
			  __func__, ctrl, id, ret);
	return ret;
}

int tsra_ctrl_cmd_delete_cq(struct tsra_ctrl *ctrl, u16 id)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev,
			   "[ENTRY] %s tsra_ctrl:%p Deleting cq with id[%d]\n",
			   __func__, ctrl, id);
	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_DELETE_CQ, 0, 0);
	cmd.sqe.delete_cq.id = id;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	TSRA_DEV_LOG_EXIT(
		&ctrl->pdev->dev,
		"[EXIT] %s tsra_ctrl:%p Deleting cq with id[%d] ret:%d\n",
		__func__, ctrl, id, ret);
	return ret;
}

int tsra_ctrl_cmd_create_sq(struct tsra_ctrl *ctrl, dma_addr_t prp1, u16 id,
			    u16 cq_id, u16 size, enum tsra_ioq_pri pri)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret;
	struct device *dev = &ctrl->pdev->dev;

	TSRA_DEV_LOG_ENTRY(
		dev, "[ENTRY] %s tsra_ctrl:%p sq id:%d sq size:%d Cq id:%d\n",
		__func__, ctrl, id, size, cq_id);

	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_CREATE_SQ, prp1, 0);
	cmd.sqe.create_sq.id = id;
	cmd.sqe.create_sq.cqid = cq_id;
	cmd.sqe.create_sq.size = size - 1;
	cmd.sqe.create_sq.qprio = pri;
	cmd.sqe.create_sq.pc = true;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	TSRA_DEV_LOG_EXIT(
		dev,
		"[EXIT] %s tsra_ctrl:%p sq id:%d sq size:%d Cq id:%d ret:%d\n",
		__func__, ctrl, id, size, cq_id, ret);
	return ret;
}

int tsra_ctrl_cmd_delete_sq(struct tsra_ctrl *ctrl, u16 id)
{
	struct tsra_ctrl_cmd cmd = { 0 };
	int ret;

	TSRA_DEV_LOG_ENTRY(&ctrl->pdev->dev,
			   "[ENTRY] %s tsra_ctrl:%p Deleting sq with id[%d]\n",
			   __func__, ctrl, id);
	tsra_ctrl_cmd_init(&cmd, TSRA_CTRL_CMD_OP_DELETE_SQ, 0, 0);
	cmd.sqe.delete_sq.id = id;

	ret = tsra_ctrl_cmd_process(ctrl, &cmd);
	TSRA_DEV_LOG_EXIT(
		&ctrl->pdev->dev,
		"[EXIT] %s tsra_ctrl:%p Deleting sq with id[%d] ret:%d\n",
		__func__, ctrl, id, ret);
	return ret;
}

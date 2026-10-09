// SPDX-License-Identifier: GPL-2.0-or-later

#include "tsra_hsm.h"
#include "tsra_hsm_cmd.h"
#include "tsra_hsm_dev.h"
#include "tsra_ctrl.h"
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include "tsra_log.h"

#define TSRA_HSM_DMA_PAGE_SIZE SZ_4K

const char *admin_attributes_name_array[TSRA_HSM_ATTRIBUTE_COUNT] = {
	"num_cpqs_created", "num_fpqs_created", "num_q_delete_cmds"
};

const char *global_attributes_name_array[TSRA_GLOBAL_ATTRIBUTE_COUNT] = {
	"total_error_completions", "total_submissions",	 "total_completions",
	"total_lvl2_aborts",	   "total_cmds_aborted", "total_lvl1_aborts"
};

static ssize_t tsra_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	struct hsm_attribute_info *info =
		container_of(attr, struct hsm_attribute_info, attribute);
	return sprintf(buf, "%d", atomic_read(&info->counter));
}

static size_t tsra_hsm_ioq_ops_page_size(void)
{
	return TSRA_HSM_DMA_PAGE_SIZE;
}

static void tsra_hsm_ioq_ops_set_io_data(void *cmd, unsigned int data)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	hsm_cmd->io_data = data;
}

static unsigned int tsra_hsm_ioq_ops_get_io_data(void *cmd)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	return hsm_cmd->io_data;
}

static u16 tsra_hsm_ioq_ops_cqe_size(void)
{
	return TSRA_HSM_CMD_CQE_SIZE;
}

static void *tsra_hsm_ioq_ops_cqe(void *cmd)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	return &hsm_cmd->cqe;
}

static u16 tsra_hsm_ioq_ops_cqe_get_cid(void *cqe)
{
	struct tsra_hsm_cmd_generic_cqe *hsm_cqe = cqe;

	return hsm_cqe->cid;
}

static u16 tsra_hsm_ioq_ops_cqe_get_phase(void *cqe)
{
	struct tsra_hsm_cmd_generic_cqe *hsm_cqe = cqe;

	return hsm_cqe->psf.fld.p;
}

static u16 tsra_hsm_ioq_ops_cqe_sq_head(void *cqe)
{
	struct tsra_hsm_cmd_generic_cqe *hsm_cqe = cqe;

	return hsm_cqe->sqhd;
}

static u16 tsra_hsm_ioq_ops_sqe_size(void)
{
	return TSRA_HSM_CMD_SQE_SIZE;
}

static void *tsra_hsm_ioq_ops_sqe(void *cmd)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	return &hsm_cmd->sqe;
}

static void tsra_hsm_ioq_ops_sqe_set_cid(void *sqe, u16 cid)
{
	struct tsra_hsm_cmd_generic_sqe *hsm_sqe = sqe;

	hsm_sqe->cid = cid;
}

static int tsra_hsm_ioq_ops_complete_cmd(void *cmd, const int completion_status)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	hsm_cmd->completion_status = completion_status;
	complete(&hsm_cmd->cmpl);
	return 0;
}

static int tsra_hsm_ioq_ops_get_cmd_tag(void *cmd)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	return hsm_cmd->tag;
}

static void tsra_hsm_ioq_ops_set_cmd_tag(void *cmd, const int tag)
{
	struct tsra_hsm_generic_cmd *hsm_cmd = cmd;

	hsm_cmd->tag = tag;
}

static struct tsra_ioq_ops tsra_hsm_ioq_ops = {
	.page_size = tsra_hsm_ioq_ops_page_size,
	.set_io_data = tsra_hsm_ioq_ops_set_io_data,
	.get_io_data = tsra_hsm_ioq_ops_get_io_data,
	.cqe_size = tsra_hsm_ioq_ops_cqe_size,
	.cqe = tsra_hsm_ioq_ops_cqe,
	.cqe_get_cid = tsra_hsm_ioq_ops_cqe_get_cid,
	.cqe_get_phase = tsra_hsm_ioq_ops_cqe_get_phase,
	.cqe_get_sq_head = tsra_hsm_ioq_ops_cqe_sq_head,
	.sqe_size = tsra_hsm_ioq_ops_sqe_size,
	.sqe = tsra_hsm_ioq_ops_sqe,
	.sqe_set_cid = tsra_hsm_ioq_ops_sqe_set_cid,
	.complete_cmd = tsra_hsm_ioq_ops_complete_cmd,
	.get_tag = tsra_hsm_ioq_ops_get_cmd_tag,
	.set_tag = tsra_hsm_ioq_ops_set_cmd_tag,
};

static int tsra_hsm_attribute_init(struct tsra_hsm *hsm)
{
	int i;
	int err = 0;

	/*
	 * Create a top level folder in sysfs called tsra
	 */
	snprintf(hsm->sysfs_name, sizeof(hsm->sysfs_name), "%s", "tsra");

	/*
	 * create the top level kobject
	 * This should show as /sys/bus/pci/devices/<TSRA BDF>/tsra
	 */
	hsm->dev_kobj =
		kobject_create_and_add(hsm->sysfs_name, &hsm->pdev->dev.kobj);
	if (!hsm->dev_kobj) {
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[%s]: kobject_create_and_add failed for main group\n",
			__func__);
		return -ENOMEM;
	}

	/*
	 * Initialize the attribute array and attribute group
	 */

	for (i = 0; i < TSRA_HSM_ATTRIBUTE_COUNT; i++) {
		sysfs_attr_init(&hsm->hsm_attribute_array[i].attribute);
		hsm->hsm_admin_attribute_array[i].attribute.attr.name =
			admin_attributes_name_array[i];
		hsm->hsm_admin_attribute_array[i].attribute.attr.mode = 0664;
		hsm->hsm_admin_attribute_array[i].attribute.show =
			tsra_attr_show;
		hsm->hsm_admin_attribute_array[i].attribute.store = NULL;
		hsm->hsm_admin_attribute_array[i].context = (void *)hsm;
		hsm->p_admin_attrib_array[i] =
			&hsm->hsm_admin_attribute_array[i].attribute.attr;
		atomic_set(&hsm->hsm_admin_attribute_array[i].counter, 0);
	}

	/* The last one is null-terminated */
	hsm->p_admin_attrib_array[TSRA_HSM_ATTRIBUTE_COUNT] = NULL;
	hsm->admin_attribute_group.attrs = hsm->p_admin_attrib_array;

	hsm->admin_group_kobj = kobject_create_and_add("admin", hsm->dev_kobj);
	if (!hsm->admin_group_kobj) {
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[%s]: kobject_create_and_add failed for admin attributes\n",
			__func__);
		kobject_put(hsm->dev_kobj);
		hsm->dev_kobj = NULL;
		return -ENOMEM;
	}

	err = sysfs_create_group(hsm->admin_group_kobj,
				 &hsm->admin_attribute_group);
	if (err) {
		/* TODO does failure in allocating attributes
		 * result in failure loading
		 */
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[%s]: sysfs_create_group failed for admin attributes\n",
			__func__);
		kobject_put(hsm->dev_kobj);
		hsm->dev_kobj = NULL;
		kobject_put(hsm->admin_group_kobj);
		hsm->admin_group_kobj = NULL;
		return err;
	}

	for (i = 0; i < TSRA_GLOBAL_ATTRIBUTE_COUNT; i++) {
		sysfs_attr_init(&hsm->hsm_global_attribute_array[i].attribute);
		hsm->hsm_global_attribute_array[i].attribute.attr.name =
			global_attributes_name_array[i];
		hsm->hsm_global_attribute_array[i].attribute.attr.mode = 0664;
		hsm->hsm_global_attribute_array[i].attribute.show =
			tsra_attr_show;
		hsm->hsm_global_attribute_array[i].attribute.store = NULL;
		hsm->hsm_global_attribute_array[i].context = (void *)hsm;
		hsm->p_global_attrib_array[i] =
			&hsm->hsm_global_attribute_array[i].attribute.attr;
		atomic_set(&hsm->hsm_global_attribute_array[i].counter, 0);
	}

	/* The last one is null-terminated */
	hsm->p_global_attrib_array[TSRA_GLOBAL_ATTRIBUTE_COUNT] = NULL;
	hsm->global_attribute_group.attrs = hsm->p_global_attrib_array;

	hsm->global_group_kobj =
		kobject_create_and_add("global", hsm->dev_kobj);
	if (!hsm->global_group_kobj) {
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[%s]: kobject_create_and_add failed for global attributes\n",
			__func__);
		kobject_put(hsm->admin_group_kobj);
		hsm->admin_group_kobj = NULL;
		kobject_put(hsm->dev_kobj);
		hsm->dev_kobj = NULL;
		return -ENOMEM;
	}

	err = sysfs_create_group(hsm->global_group_kobj,
				 &hsm->global_attribute_group);
	if (err) {
		/*
		 * TODO does failure in allocating attributes
		 * result in failure loading
		 */
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[%s]: sysfs_create_group failed for admin attributes\n",
			__func__);
		sysfs_remove_group(hsm->admin_group_kobj,
				   &hsm->admin_attribute_group);
		kobject_put(hsm->admin_group_kobj);
		hsm->admin_group_kobj = NULL;

		kobject_put(hsm->global_group_kobj);
		hsm->global_group_kobj = NULL;

		kobject_put(hsm->dev_kobj);
		hsm->dev_kobj = NULL;
		return err;
	}

	return 0;
}

static void tsra_hsm_attribute_deinit(struct tsra_hsm *hsm)
{
	TSRA_DEV_LOG_ENTRY(&hsm->pdev->dev, "[%s]: hsm:%p\n", __func__, hsm);
	if (hsm->global_group_kobj) {
		TSRA_DEV_LOG_INFO(
			&hsm->pdev->dev,
			"[%s]: hsm:%p Removing global attribute group\n",
			__func__, hsm);
		sysfs_remove_group(hsm->global_group_kobj,
				   &hsm->global_attribute_group);
		kobject_put(hsm->global_group_kobj);
		hsm->global_group_kobj = NULL;
	}

	if (hsm->admin_group_kobj) {
		TSRA_DEV_LOG_INFO(
			&hsm->pdev->dev,
			"[%s]: hsm:%p Removing admin attribute group\n",
			__func__, hsm);
		sysfs_remove_group(hsm->admin_group_kobj,
				   &hsm->admin_attribute_group);
		kobject_put(hsm->admin_group_kobj);
		hsm->admin_group_kobj = NULL;
	}
	if (hsm->dev_kobj) {
		TSRA_DEV_LOG_INFO(&hsm->pdev->dev,
				  "[%s]: hsm:%p Removing dev_kobj\n", __func__,
				  hsm);
		kobject_put(hsm->dev_kobj);
		hsm->dev_kobj = NULL;
	}
	TSRA_DEV_LOG_EXIT(&hsm->pdev->dev, "[%s]: hsm:%p\n", __func__, hsm);
}

int tsra_hsm_init(struct tsra_hsm *hsm, struct tsra_hsm_cfg *cfg,
		  const bool abort)
{
	int err;
	struct tsra_ioq_pool_cfg ioq_pool_cfg = { 0 };

	hsm->pdev = cfg->ctrl->pdev;
	ioq_pool_cfg.name = TSRA_HSM_DEV_NAME;
	ioq_pool_cfg.ctrl = cfg->ctrl;
	ioq_pool_cfg.pdev = cfg->ctrl->pdev;
	ioq_pool_cfg.ioq_db = cfg->ioq_db;
	ioq_pool_cfg.ioq_id_start = cfg->ioq_id_start;
	ioq_pool_cfg.ioq_size = cfg->ioq_size;
	ioq_pool_cfg.ioq_type = TSRA_IOQ_TYPE_HSM;
	ioq_pool_cfg.ioq_ops = &tsra_hsm_ioq_ops;
	ioq_pool_cfg.msix_start = cfg->msix_start;
	ioq_pool_cfg.msix_max_cnt = cfg->msix_max_cnt;

	/*
	 * Allocating a minor number must be done first since performance counters
	 * are placed with this minor number
	 */
	err = tsra_hsm_dev_alloc_minor(hsm);

	if (err) {
		TSRA_DEV_LOG_ERROR(&hsm->pdev->dev,
				   "[%s] allocating minor number failed\n",
				   __func__);
		return err;
	}

	TSRA_DEV_LOG_INFO(
		&hsm->pdev->dev,
		"[%s] SUCCESSFULLY allocated major:%d minor:%d for HSM interface\n",
		__func__, hsm->major, hsm->minor);

	err = tsra_hsm_attribute_init(hsm);

	if (err) {
		goto tsra_hsm_attribute_init_failed;
		return err;
	}

	TSRA_DEV_LOG_INFO(
		&hsm->pdev->dev,
		"[ENTRY] %s hsm:%p ioq_id_start:%d ioq_size:%d msix_start:%d msix_max_cnt:%d\n",
		__func__, hsm, ioq_pool_cfg.ioq_id_start, ioq_pool_cfg.ioq_size,
		ioq_pool_cfg.msix_start, ioq_pool_cfg.msix_max_cnt);

	if (false == abort) {
		mutex_init(&hsm->hsm_lock);
		hsm->page_pool = dma_pool_create(
			ioq_pool_cfg.name, &hsm->pdev->dev, SZ_4K, SZ_4K, 0);

		if (!hsm->page_pool) {
			TSRA_DEV_LOG_ERROR(
				&hsm->pdev->dev,
				"[%s]: hsm:%p HSM page pool creation failure",
				__func__, hsm);
			err = -ENOMEM;
			sysfs_remove_group(hsm->dev_kobj,
					   &hsm->admin_attribute_group);
			kobject_put(hsm->dev_kobj);
			hsm->dev_kobj = NULL;
			goto dma_pool_creation_failed;
		}
	}

	hsm->ioq_pool.parent_kobj = hsm->dev_kobj;

	err = tsra_ioq_pool_init(&hsm->ioq_pool, &ioq_pool_cfg);
	if (err) {
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[ENTRY] %s hsm:%p tsra_ioq_pool_init failed err:%d\n",
			__func__, hsm, err);
		goto pool_init_fail;
	}

	/*
	 * do not create soft device if the number of queues is 0
	 *  (resource groups not allocated for this function?)
	 */
	if (hsm->ioq_pool.ioq_max_cnt == 0) {
		err = -ENOMEM;
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"Device has 0 HSM queues. Not creating HSM device link\n");
		goto zero_qs_configured;
	}

	atomic_add(hsm->ioq_pool.ioq_max_cnt,
		   &hsm->hsm_admin_attribute_array
			    [TSRA_HSM_ADMIN_ATTRIBUTE_NUM_CPQS_CREATED]
				    .counter);

	err = tsra_hsm_dev_init(hsm, abort);
	if (err) {
		TSRA_DEV_LOG_ERROR(
			&hsm->pdev->dev,
			"[ENTRY] %s hsm:%p tsra_hsm_dev_init failed err:%d\n",
			__func__, hsm, err);
		goto dev_init_fail;
	}

	TSRA_DEV_LOG_EXIT(&hsm->pdev->dev, "%s hsm:%p\n", __func__, hsm);
	return 0;

dev_init_fail:
zero_qs_configured:
	tsra_ioq_pool_deinit(&hsm->ioq_pool, abort);
pool_init_fail:
	dma_pool_destroy(hsm->page_pool);
	hsm->page_pool = NULL;
dma_pool_creation_failed:
	tsra_hsm_attribute_deinit(hsm);
tsra_hsm_attribute_init_failed:
	tsra_hsm_dev_dealloc_minor(hsm);
	return err;
}

void tsra_hsm_deinit(struct tsra_hsm *hsm, const bool abort)
{
	TSRA_DEV_LOG_ENTRY(&hsm->pdev->dev, "%s hsm:%p\n", __func__, hsm);
	tsra_hsm_attribute_deinit(hsm);
	tsra_hsm_dev_deinit(hsm, abort);
	tsra_ioq_pool_deinit(&hsm->ioq_pool, abort);

	if (true == abort) {
		TSRA_DEV_LOG_INFO(&hsm->pdev->dev,
				  "[EXIT] %s hsm:%p. In abort. doing nothing\n",
				  __func__, hsm);
		return;
	}
	/*
	 * destroy the pools only as part of normal shutdown.
	 * During abort, these pools are untouched
	 */
	dma_pool_destroy(hsm->page_pool);
	hsm->page_pool = NULL;
	TSRA_DEV_LOG_EXIT(&hsm->pdev->dev, "%s hsm:%p\n", __func__, hsm);
}

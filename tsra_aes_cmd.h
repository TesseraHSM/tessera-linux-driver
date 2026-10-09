/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_TSRA_AES_CMDS_H
#define _LINUX_TSRA_AES_CMDS_H

#include <linux/kernel.h>
#include <linux/completion.h>
#include "tsra_aes_dev_ioctl.h"
#include "tsra_dma_io.h"

enum tsra_aes_cmd_type { TSRA_AES_CMD_TYPE_AES = 5 };

enum tsra_aes_psdt_type {
	TSRA_AES_PSDT_TYPE_PRP = 0,
	TSRA_AES_PSDT_TYPE_SGL = 1
};

// Enabled Byte Packing from Here
#pragma pack(push)
#pragma pack(1)

/*
 * This structure defines the command attributes
 * field of the CQE
 */
struct tsra_aes_cmd_attr {
	u16 cmd_opc : 1; // Operation Code
	u16 rsvd1 : 1;
	u16 psdt : 1; // PRP or SGL enum PSDT_TYPE
	u16 cmd_type : 3; // Command type
	u16 rsvd2 : 2;
	u16 cipher : 1; // Cipher
	u16 rsvd3 : 5;
	u16 dul : 2; // Data Unit len
};

static_assert(sizeof(struct tsra_aes_cmd_attr) == sizeof(u16));

#define TSRA_AES_SGL_DESC_SZ 16

struct tsra_aes_sgl_desc {
	u64 addr;
	u32 len;
	u8 Rsvd[3];
	u8 desc_sub_type : 4;
	u8 desc_type : 4;
};

static_assert(sizeof(struct tsra_aes_sgl_desc) == TSRA_AES_SGL_DESC_SZ);

#define TSRA_AES_SQE_DATA_SZ 20

/*
 * This structure describes the single data transfer parameters
 * for a sgl/prp transfer for the AES path data transfer
 */
struct tsra_aes_sqe_data {
	u32 len;

	union {
		u64 fst_snd[2];
		struct {
			u64 fst; //First PRP Entry
			u64 snd; //Second PRP Entry
		} prp;

		struct {
			struct tsra_aes_sgl_desc sgl_desc;
		} sgl;

	} dptr;
};

static_assert(sizeof(struct tsra_aes_sqe_data) == TSRA_AES_SQE_DATA_SZ);

#define TSRA_AES_TAG_LEN 16
#define TSRA_AES_KEY_LEN 32
#define TSRA_AES_IV_LEN 12
#define TSRA_AES_TWEAK_LEN 16

#define TSRA_AES_SQE_CMD_SPECIFIC_SZ 63

struct tsra_aes_sqe_cmd_specific {
	union {
		struct {
			u8 rsvd1[3];
			u32 key_id; // Key Identifier
			u32 actual_aad_len; // Unaligned AAD Length
			u8 tag[TSRA_AES_TAG_LEN]; // AES Tag
			u8 iv[TSRA_AES_IV_LEN]; // AES Initialization Vector
			u32 aligned_aad_len; // Total Data Length (Aligned)
			u8 unaligned_src_data_len; 
			u8 unaligned_dst_data_len; 
			u16 rsvd2;
			u32 unaligned_src_data_addr_low;
			u32 unaligned_src_data_addr_high;
			u32 unaligned_dst_data_addr_low;
			u32 unaligned_dst_data_addr_high;
		} gcm;

		struct {
			u8 rsvd1[3];
			u32 key_id1; // Key Identifier 1
			u32 key_id2; // Key Identifier 2
			u8 tweak[TSRA_AES_TWEAK_LEN];
			u32 rsvd2[9];
		} xts;

	} u;
};

static_assert(sizeof(struct tsra_aes_sqe_cmd_specific) ==
	      TSRA_AES_SQE_CMD_SPECIFIC_SZ);

#define TSRA_AES_SQE_SZ 128

struct tsra_aes_sqe {
	u32 rsvd1;
	struct tsra_aes_cmd_attr attr;
	u16 cmd_id;
	u32 session_id : 16;
	u32 short_app_id : 8;
	u32 rsvd2 : 8;
	u32 rsvd3[3];
	struct tsra_aes_sqe_data src_data;
	struct tsra_aes_sqe_data dst_data;
	u8 frame_type; // tsra_aes_frame_type
	struct tsra_aes_sqe_cmd_specific cmd_spec;
};

static_assert(sizeof(struct tsra_aes_sqe) == TSRA_AES_SQE_SZ);

#pragma pack(pop)
//Byte packing Disabled. For 64 bit compilers the packing is 8 bytes

#define TSRA_AES_CQE_CMD_SPEC_SZ 16

struct tsra_aes_cqe_cmd_spec {
	union {
		u8 val[16];
		u8 tag[16];
	} u;
};

static_assert(sizeof(struct tsra_aes_cqe_cmd_spec) ==
	      TSRA_AES_CQE_CMD_SPEC_SZ);

#define TSRA_AES_CQE_SZ 64

struct tsra_aes_cqe {
	struct tsra_aes_cmd_attr attr; /**< Attributes */
	u16 cmd_id; /**< Command ID */
	struct tsra_aes_cqe_cmd_spec cmd_spec; /**< Command specific data */
	u8  iv_from_fw[TSRA_AES_IV_LEN];
	u32 rsvd[3];
	u32 unaligned_dst_data_len;
	u32 fips_approved;
	u32 len;
	u16 sq_head; /**< Submission queue head */
	u16 sq_id; /**< Submission queue ID */
	u16 err;
	union {
		u16 ph_sts_val; /**< Phase & Status field value */
		struct {
			u16 phase : 1; /**< Phase */
			u16 sts : 15; /**< Status code */
		} ph_sts_bits;
	} ph_sts;
};

static_assert(sizeof(struct tsra_aes_cqe) == TSRA_AES_CQE_SZ);

struct tsra_aes_cmd {
	struct tsra_aes_sqe sqe;
	struct tsra_aes_cqe cqe;
	struct tsra_dma_io dma_io_src;
	struct tsra_dma_io dma_io_dst;
	unsigned int io_data;
	struct completion cmpl;

	/*
	 * Status indicates whether command is
	 * completed normally by device or
	 * completed due to command being aborted
	 */
	int completion_status;

	/*
	 * A tag identifies this context in the
	 * context store where this command is submitted
	 * Initialized to -1 and then to a valid value
	 */
	int tag;

	/*
	 * The single buffer for Unaligned data is kept here
	 */
	void *aux_buffer_kva;
	dma_addr_t hw_addr;
	u32 unaligned_data_size;
};

void tsra_aes_cmd_init(struct tsra_aes_cmd *cmd, const u8 opc,
			 const u8 psdt, const u8 cmd_type, const u8 frame_type,
			 const u8 cipher);

int tsra_aes_cmd_process(struct tsra_aes *aes, struct tsra_aes_cmd *cmd);

/*
 * These dump routines log full command buffers (keys, IVs, tags,
 * addresses) and must not be compiled into production builds. See the
 * DEBUG gating comment above TSRA_LOG_DEBUG in tsra_log.h.
 */
#ifdef DEBUG
void dump_aes_cmd_sqe(struct tsra_aes_cmd *cmd);
void dump_aes_cmd_cqe(struct tsra_aes_cmd *cmd);
#else
static inline void dump_aes_cmd_sqe(struct tsra_aes_cmd *cmd) { }
static inline void dump_aes_cmd_cqe(struct tsra_aes_cmd *cmd) { }
#endif

#endif // _LINUX_TSRA_AES_CMDS_H

/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_TSRA_ERROR_CODES_H_
#define _LINUX_TSRA_ERROR_CODES_H_

enum tsra_sts_source {
	TSRA_STS_SRC_NONE =
		0, // If the IOCTL is malformed and fails even before the function is identified
	TSRA_STS_SRC_DEVICE = 1, // Error code is a device generated error code
	TSRA_STS_SRC_CP = 2, // Error code is a CP PATH generated error code
	TSRA_STS_SRC_FP = 3, // ERROR code is FP generated error code
	TSRA_STS_SRC_ABORT =
		4, // ERROR code is a ABORT path generated error code
};

/*
 * Enumeration for all possible error codes
 * on the fast path ioctl interface
 * Starts at offset 100 to keep it consistent with
 * Windows.
 */
enum TSRA_FP_IOCTL_ERROR_STATUS {
	TSRA_FP_IOCTL_NO_MEMORY = 100,
	TSRA_FP_IOCTL_INVALID_INPUT_BUFFER = 101,
	TSRA_FP_IOCTL_INPUT_BUFFER_ACCESS_ERROR = 102,
	TSRA_FP_IOCTL_INVALID_OUTPUT_BUFFER = 103,
	TSRA_FP_IOCTL_OUTPUT_BUFFER_ACCESS_ERROR = 104,
	TSRA_FP_IOCTL_PROCESS_NOT_OWNER_OF_FD = 105,
	TSRA_FP_IOCTL_DEVICE_ERROR = 106,
	TSRA_FP_IOCTL_SESSION_ID_DOES_NOT_MATCH = 107,
	TSRA_FP_IOCTL_SHORTAPP_ID_DOES_NOT_MATCH = 108,
	TSRA_FP_IOCTL_NO_VALID_SESSION_ID = 109,
	TSRA_FP_IOCTL_NO_VALID_SHORT_APP_ID = 110,
	TSRA_FP_IOCTL_DEVICE_NO_FP_QUEUES = 111,
	TSRA_FP_IOCTL_INVALID_CIPHER_TYPE = 112,
	TSRA_FP_IOCTL_INVALID_FRAME_TYPE = 113,
	TSRA_FP_IOCTL_INVALID_OPCODE = 114,
	TSRA_FP_IOCTL_INPUT_BUFFER_ABOVE_MAX = 115,
	TSRA_FP_IOCTL_OUTPUT_BUFFER_ABOVE_MAX = 116,
	TSRA_FP_IOCTL_AES_GCM_IOCTL_VALIDATION_FAILED = 117,
	TSRA_FP_IOCTL_AES_XTS_IOCTL_VALIDATION_FAILED = 118,
};

enum TSRA_CP_GENERIC_IOCTL_ERROR_STATUS {
	TSRA_CP_GENERIC_IOCTL_NO_MEMORY = 1,
	TSRA_CP_GENERIC_IOCTL_INVALID_CMDSET = 2,
	TSRA_CP_GENERIC_IOCTL_INPUT_BUFFER_ABOVE_8K = 3,
	TSRA_CP_GENERIC_IOCTL_OUTPUT_BUFFER_ABOVE_8K = 4,
	TSRA_CP_GENERIC_IOCTL_INVALID_INPUT_BUFFER = 5,
	TSRA_CP_GENERIC_IOCTL_INPUT_BUFFER_ACCESS_ERROR = 6,
	TSRA_CP_GENERIC_IOCTL_INVALID_OUTPUT_BUFFER = 7,
	TSRA_CP_GENERIC_IOCTL_OUTPUT_BUFFER_ACCESS_ERROR = 8,
	TSRA_CP_GENERIC_IOCTL_PROCESS_NOT_OWNER_OF_FD = 9,
	TSRA_CP_GENERIC_IOCTL_DEVICE_ERROR = 10,
	TSRA_CP_GENERIC_IOCTL_OPEN_SESSION_SESSION_LIMIT_REACHED = 11,
	TSRA_CP_GENERIC_IOCTL_NO_EXISTING_SESSION = 12,
	TSRA_CP_GENERIC_IOCTL_INVALID_SESSION_OPCODE = 13,
	TSRA_CP_GENERIC_IOCTL_SESSION_ID_MISMATCH = 14,

	/*
	 * This status code indicates that the command could not
	 * be submitted to the device because either all queues are
	 * full or all devices are disabled
	 * Normally these conditions are transient and expected to clear
	 * up. The application can choose to wait for some time and resubmit
	 * the same command
	 */
	TSRA_CP_GENERIC_IOCTL_RETRY_CMD = 15,
};

enum TSRA_ABORT_IOCTL_ERROR_STATUS {
	TSRA_ABORT_IN_PROGRESS = 1,
	TSRA_ABORT_CMD_ABORTED = 2,
};

/*
 * This macro returns a 32 bit error code which is upper 8-bits is the
 * Source Of The Status code and the lower 24 bits is the actual code.
 * A SOURCE from where the error code is generated.
 * Note: We are just using this for ABORT path right now.
 * To Do :- We will cleanup the error codes in all the paths step by step
 * as all the test cases will need to be updated.
 */
#define GENERATE_STATUS_CODE(_sts_src, _sts) (((_sts_src) << 24) | (_sts))

#endif

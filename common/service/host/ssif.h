/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SSIF_H
#define SSIF_H

#include "plat_def.h"
//#ifdef ENABLE_SSIF

#include <stdbool.h>
#include <stdint.h>
#include <zephyr.h>

#include "hal_i2c_target.h"

#define SSIF_THREAD_STACK_SIZE 3072
#define SSIF_POLLING_INTERVAL 100
#define SSIF_MAX_IPMI_DATA_SIZE 32
#define SSIF_BUFF_SIZE 512

#define SSIF_TASK_NAME_LEN 32

#define SSIF_MULTI_RD_KEY 0x0001

#define CMD_SYS_INFO_FW_VERSION 0x01

typedef enum ssif_status {
	SSIF_STATUS_IDLE,

	SSIF_STATUS_WR_SINGLE = 0x01,
	SSIF_STATUS_WR_MULTI_START = 0x02,
	SSIF_STATUS_WR_MULTI_MIDDLE = 0x04,
	SSIF_STATUS_WR_MULTI_END = 0x08,

	SSIF_STATUS_RD_SINGLE = 0x10,
	SSIF_STATUS_RD_MULTI_START = 0x20,
	SSIF_STATUS_RD_MULTI_MIDDLE = 0x40,
	SSIF_STATUS_RD_MULTI_END = 0x80,
} ssif_status_t;

typedef enum ssif_err_status {
	SSIF_STATUS_NO_ERR,
	SSIF_STATUS_INVALID_CMD,
	SSIF_STATUS_INVALID_CMD_IN_CUR_STATUS,
	SSIF_STATUS_INVALID_PEC,
	SSIF_STATUS_INVALID_LEN,
	SSIF_STATUS_TIMEOUT,
	SSIF_STATUS_UNKNOWN_ERR = 0xFF,
} ssif_err_status_t;

enum ssif_cmd {
	SSIF_WR_SINGLE = 0x02,
	SSIF_WR_MULTI_START = 0x06,
	SSIF_WR_MULTI_MIDDLE = 0x07,
	SSIF_WR_MULTI_END = 0x08,

	SSIF_RD_SINGLE = 0x03, // same as SSIF_RD_MULTI_START
	SSIF_RD_MULTI_MIDDLE = 0x09, // same as SSIF_RD_MULTI_END
	SSIF_RD_MULTI_RETRY = 0x0A,
};

typedef enum ssif_action {
	SSIF_DO_NOTHING,
	SSIF_SEND_IPMI,
	SSIF_COLLECT_DATA,
} ssif_action_t;

typedef struct _ssif_dev {
	uint8_t index;
	uint8_t i2c_bus;
	uint8_t addr; // bic itself, 7bit
	bool addr_lock;
	int64_t exp_to_ms;
	k_tid_t ssif_task_tid;
	K_KERNEL_STACK_MEMBER(ssif_task_stack, SSIF_THREAD_STACK_SIZE);
	uint8_t task_name[SSIF_TASK_NAME_LEN];
	struct k_thread task_thread;
	struct k_mutex rd_buff_mutex;
	struct k_sem rd_buff_sem;
	uint8_t rd_buff[SSIF_BUFF_SIZE];
	uint16_t rd_len;
} ssif_dev;

struct ssif_wr_start {
	uint8_t len;
	uint8_t netfn; // netfn(6bit) + lun(2bit)
	uint8_t cmd;
	uint8_t data[SSIF_MAX_IPMI_DATA_SIZE - 2]; // -netfn -cmd
} __attribute__((packed));

struct ssif_wr_middle {
	uint8_t len;
	uint8_t data[SSIF_MAX_IPMI_DATA_SIZE];
}  __attribute__((packed));

struct ssif_rd_single {
	uint8_t len;
	uint8_t netfn; // netfn(6bit) + lun(2bit)
	uint8_t cmd;
	uint8_t cmplt_code;
	uint8_t data[SSIF_MAX_IPMI_DATA_SIZE - 3]; // -netfn -cmd -cc
}  __attribute__((packed));

struct ssif_rd_start {
	uint8_t len;
	uint16_t start_key; // should equal 0x0001
	uint8_t netfn; // netfn(6bit) + lun(2bit)
	uint8_t cmd;
	uint8_t cmplt_code;
	uint8_t data[SSIF_MAX_IPMI_DATA_SIZE - 5]; // -netfn -cmd -cc -key(2bytes)
}  __attribute__((packed));

struct ssif_rd_middle {
	uint8_t len;
	uint8_t block;
	uint8_t data[0];
}  __attribute__((packed));

void ssif_device_init(uint8_t *config, uint8_t size);
ssif_err_status_t ssif_get_error_status();
bool ssif_set_data(uint8_t channel, ipmi_msg_cfg *msg_cfg);
void ssif_collect_data(uint8_t smb_cmd, uint8_t bus);
bool ssif_lock_ctl(ssif_dev *ssif_inst, bool lck_flag);
ssif_dev *ssif_inst_get_by_bus(uint8_t bus);

//#endif /* ENABLE_SSIF */

#endif /* SSIF_H */

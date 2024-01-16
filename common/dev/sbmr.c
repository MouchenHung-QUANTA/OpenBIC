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

#include "plat_def.h"
#ifdef ENABLE_SBMR

#include <zephyr.h>
#include <stdlib.h>
#include "libutil.h"
#include <logging/log.h>
#include "ipmi.h"
#include "ipmb.h"
#include "sensor.h"
#include "sbmr.h"

LOG_MODULE_REGISTER(sbmr);

#define SBMR_POSTCODE_LEN 128
#define PROCESS_POSTCODE_STACK_SIZE 2048

#define SMBR_CC_ERR 0x80
#define SMBR_GROUP_DEF_BODE_CODE 0xAE

K_THREAD_STACK_DEFINE(sbmr_process_postcode_thread, PROCESS_POSTCODE_STACK_SIZE);
static struct k_thread sbmr_process_postcode_thread_handler;

static struct sbmr_boot_progress_code sbmr_cmplete_read_buffer[SBMR_POSTCODE_LEN];
static uint32_t sbmr_read_buffer[SBMR_POSTCODE_LEN];
static uint16_t sbmr_read_len = 0, sbmr_read_index = 0;
static bool sbmr_proc_4byte_postcode_ok = false;
static struct k_sem sbmr_get_postcode_sem;

uint16_t copy_sbmr_read_buffer(uint16_t start, uint16_t length, uint8_t *buffer,
			       uint16_t buffer_len)
{
	CHECK_NULL_ARG_WITH_RETURN(buffer, 0);

	if (buffer_len < (length * 4))
		return 0;

	uint16_t current_index, i = 0;
	uint16_t current_read_len = sbmr_read_len;
	uint16_t current_read_index = sbmr_read_index;
	if (start < current_read_index) {
		current_index = current_read_index - start - 1;
	} else {
		current_index = current_read_index + SBMR_POSTCODE_LEN - start - 1;
	}

	for (; (i < length) && ((i + start) < current_read_len); i++) {
		buffer[4 * i] = sbmr_read_buffer[current_index] & 0xFF;
		buffer[(4 * i) + 1] = (sbmr_read_buffer[current_index] >> 8) & 0xFF;
		buffer[(4 * i) + 2] = (sbmr_read_buffer[current_index] >> 16) & 0xFF;
		buffer[(4 * i) + 3] = (sbmr_read_buffer[current_index] >> 24) & 0xFF;

		if (current_index == 0) {
			current_index = SBMR_POSTCODE_LEN - 1;
		} else {
			current_index--;
		}
	}
	return 4 * i;
}

static void process_postcode(void *arvg0, void *arvg1, void *arvg2)
{
	uint16_t send_index = 0;
	while (1) {
		k_sem_take(&sbmr_get_postcode_sem, K_FOREVER);
		ipmi_msg *msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
		if (msg == NULL) {
			LOG_ERR("Memory allocation failed.");
			continue;
		}

		uint16_t current_read_index = sbmr_read_index;
		for (; send_index != current_read_index; send_index++) {
			if (send_index == SBMR_POSTCODE_LEN - 1) {
				send_index = 0;
			}

			memset(msg, 0, sizeof(ipmi_msg));
			msg->InF_source = SELF;
			msg->InF_target = BMC_IPMB;
			msg->netfn = NETFN_OEM_1S_REQ;
			msg->cmd = CMD_OEM_1S_SEND_4BYTE_POST_CODE_TO_BMC;
			msg->data_len = 8;
			msg->data[0] = IANA_ID & 0xFF;
			msg->data[1] = (IANA_ID >> 8) & 0xFF;
			msg->data[2] = (IANA_ID >> 16) & 0xFF;
			msg->data[3] = 4;
			msg->data[4] = sbmr_read_buffer[send_index] & 0xFF;
			msg->data[5] = (sbmr_read_buffer[send_index] >> 8) & 0xFF;
			msg->data[6] = (sbmr_read_buffer[send_index] >> 16) & 0xFF;
			msg->data[7] = (sbmr_read_buffer[send_index] >> 24) & 0xFF;
			ipmb_error status = ipmb_read(msg, IPMB_inf_index_map[msg->InF_target]);
			if (status != IPMB_ERROR_SUCCESS) {
				LOG_ERR("Failed to send 4-byte post code to BMC, status %d.",
					status);
			}
			k_yield();
		}
		SAFE_FREE(msg)
	}
}

void sbmr_postcode_insert(struct sbmr_boot_progress_code boot_progress_code)
{
	sbmr_proc_4byte_postcode_ok = true;

	LOG_INF("* POSTCODE: 0x%x", boot_progress_code.efi_status_code);

	sbmr_read_buffer[sbmr_read_index] = boot_progress_code.efi_status_code;
	sbmr_cmplete_read_buffer[sbmr_read_index] = boot_progress_code;

	if (sbmr_read_len < SBMR_POSTCODE_LEN)
		sbmr_read_len++;

	sbmr_read_index++;
	if (sbmr_read_index == SBMR_POSTCODE_LEN)
		sbmr_read_index = 0;

	//k_sem_give(&sbmr_get_postcode_sem);
}

void sbmr_postcode_read_init()
{
	k_sem_init(&sbmr_get_postcode_sem, 0, 1);

	k_thread_create(&sbmr_process_postcode_thread_handler, sbmr_process_postcode_thread,
			K_THREAD_STACK_SIZEOF(sbmr_process_postcode_thread), process_postcode, NULL,
			NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sbmr_process_postcode_thread_handler, "sbmr_process_postcode_thread");
}

void reset_sbmr_postcode_buffer()
{
	sbmr_read_len = 0;
	return;
}

bool sbmr_get_4byte_postcode_ok()
{
	return sbmr_proc_4byte_postcode_ok;
}

void sbmr_reset_4byte_postcode_ok()
{
	sbmr_proc_4byte_postcode_ok = false;
}

__weak bool SBMR_SEND_BOOT_PROGRESS_CODE(ipmi_msg *msg)
{
	CHECK_NULL_ARG_WITH_RETURN(msg, false);

	msg->completion_code = SMBR_CC_ERR;

	if (msg->data_len != sizeof(struct sbmr_cmd_send_boot_progress_code_req)) {
		LOG_ERR("Get invalid data length 0x%x -- 0x%x", msg->data_len, sizeof(struct sbmr_cmd_send_boot_progress_code_req));
		goto exit;
	}

	struct sbmr_cmd_send_boot_progress_code_req *req = (struct sbmr_cmd_send_boot_progress_code_req *) msg->data;

	if (req->group_ext_def_body != SMBR_GROUP_DEF_BODE_CODE) {
		LOG_ERR("Get invalid group_ext_def_body 0x%x", req->group_ext_def_body);
		goto exit;
	}

	sbmr_postcode_insert(req->code);

	msg->data[0] = SMBR_GROUP_DEF_BODE_CODE;
	msg->data_len = 1;

	msg->completion_code = CC_SUCCESS;

exit:
	return true;
}

__weak bool SBMR_GET_BOOT_PROGRESS_CODE(ipmi_msg *msg)
{
	CHECK_NULL_ARG_WITH_RETURN(msg, false);

	msg->completion_code = SMBR_CC_ERR;

	if (msg->data_len != sizeof(struct sbmr_cmd_get_boot_progress_code_req)) {
		msg->data_len = 0;
		LOG_ERR("Get invalid data length");
		goto exit;
	}

	struct sbmr_cmd_get_boot_progress_code_req *req = (struct sbmr_cmd_get_boot_progress_code_req *) msg->data;

	if (req->group_ext_def_body != SMBR_GROUP_DEF_BODE_CODE) {
		LOG_ERR("Get invalid group_ext_def_body 0x%x", req->group_ext_def_body);
		msg->data_len = 0;
		goto exit;
	}

	msg->data[0] = SMBR_GROUP_DEF_BODE_CODE;
	msg->data_len = 1;

	if (!sbmr_read_len)
		memset(&msg->data[1], 0, sizeof(struct sbmr_boot_progress_code));
	else
		memcpy(&msg->data[1], (uint8_t *)&sbmr_cmplete_read_buffer[sbmr_read_len], sizeof(struct sbmr_boot_progress_code));

	msg->data_len += sizeof(struct sbmr_boot_progress_code);
	msg->completion_code = CC_SUCCESS;

exit:
	return true;
}

bool smbr_cmd_handler(ipmi_msg *msg)
{
	CHECK_NULL_ARG_WITH_RETURN(msg, false);

	switch (msg->cmd)
	{
	case CMD_DCMI_SEND_BOOT_PROGRESS_CODE:
		if (SBMR_SEND_BOOT_PROGRESS_CODE(msg) == false)
			return false;
		break;

	case CMD_DCMI_GET_BOOT_PROGRESS_CODE:
		if (SBMR_GET_BOOT_PROGRESS_CODE(msg) == false)
			return false;
		break;

	default:
		return false;
	}

	msg->netfn = (msg->netfn + 1) << 2;

	return true;
}

void print_out()
{
	for (int i=0; i<sbmr_read_len; i++) {
		LOG_INF("* postcode[%02d]: inst %d, type 0x%08x, code 0x%08x ", i,
		sbmr_cmplete_read_buffer[i].inst,
		sbmr_cmplete_read_buffer[i].status_code,
		sbmr_cmplete_read_buffer[i].efi_status_code);
	}
}

#endif

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

#include <zephyr.h>
#include <stdlib.h>
#include "libutil.h"
#include <logging/log.h>
#include "ipmi.h"
#include "ipmb.h"
#include "sensor.h"
#include "mctp.h"
#include "pldm.h"

LOG_MODULE_REGISTER(mpro);

#define MPRO_POSTCODE_LEN 1024
#define PROCESS_POSTCODE_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(process_postcode_thread, PROCESS_POSTCODE_STACK_SIZE);
static struct k_thread process_postcode_thread_handler;

static uint32_t mpro_read_buffer[MPRO_POSTCODE_LEN];
static uint16_t mpro_read_len = 0, mpro_read_index = 0;
static bool proc_4byte_postcode_ok = false;
static struct k_sem get_postcode_sem;

uint16_t copy_mpro_read_buffer(uint16_t start, uint16_t length, uint8_t *buffer,
			       uint16_t buffer_len)
{
	if ((buffer == NULL) || (buffer_len < (length * 4))) {
		return 0;
	}

	uint16_t current_index, i = 0;
	uint16_t current_read_len = mpro_read_len;
	uint16_t current_read_index = mpro_read_index;
	if (start < current_read_index) {
		current_index = current_read_index - start - 1;
	} else {
		current_index = current_read_index + MPRO_POSTCODE_LEN - start - 1;
	}

	for (; (i < length) && ((i + start) < current_read_len); i++) {
		buffer[4 * i] = mpro_read_buffer[current_index] & 0xFF;
		buffer[(4 * i) + 1] = (mpro_read_buffer[current_index] >> 8) & 0xFF;
		buffer[(4 * i) + 2] = (mpro_read_buffer[current_index] >> 16) & 0xFF;
		buffer[(4 * i) + 3] = (mpro_read_buffer[current_index] >> 24) & 0xFF;

		if (current_index == 0) {
			current_index = MPRO_POSTCODE_LEN - 1;
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
		k_sem_take(&get_postcode_sem, K_FOREVER);
		ipmi_msg *msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
		if (msg == NULL) {
			LOG_ERR("Memory allocation failed.");
			continue;
		}

		uint16_t current_read_index = mpro_read_index;
		for (; send_index != current_read_index; send_index++) {
			if (send_index == MPRO_POSTCODE_LEN - 1) {
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
			msg->data[4] = mpro_read_buffer[send_index] & 0xFF;
			msg->data[5] = (mpro_read_buffer[send_index] >> 8) & 0xFF;
			msg->data[6] = (mpro_read_buffer[send_index] >> 16) & 0xFF;
			msg->data[7] = (mpro_read_buffer[send_index] >> 24) & 0xFF;
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

void mpro_postcode_insert(uint32_t postcode)
{
	proc_4byte_postcode_ok = true;

	mpro_read_buffer[mpro_read_index] = postcode;

	if (mpro_read_len < MPRO_POSTCODE_LEN)
		mpro_read_len++;

	mpro_read_index++;
	if (mpro_read_index == MPRO_POSTCODE_LEN)
		mpro_read_index = 0;

	k_sem_give(&get_postcode_sem);
}

void mpro_postcode_read_init()
{
	k_sem_init(&get_postcode_sem, 0, 1);

	k_thread_create(&process_postcode_thread_handler, process_postcode_thread,
			K_THREAD_STACK_SIZEOF(process_postcode_thread), process_postcode, NULL,
			NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&process_postcode_thread_handler, "process_postcode_thread");
}

void reset_mpro_postcode_buffer()
{
	mpro_read_len = 0;
	return;
}

bool get_4byte_postcode_ok()
{
	return proc_4byte_postcode_ok;
}

void reset_4byte_postcode_ok()
{
	proc_4byte_postcode_ok = false;
}

uint8_t mpro_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	/* TODO: Should mapping sensor number to mpro sensor number */

	uint8_t mpro_eid = sensor_config[sensor_config_index_map[sensor_num]].port;
	mctp *mctp_inst = NULL;
	mctp_ext_params ext_params = { 0 };
	if (get_mctp_info_by_eid(mpro_eid, &mctp_inst, &ext_params) == false) {
		LOG_ERR("Failed to get mctp info by Mpro eid 0x%x", mpro_eid);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	pldm_msg pmsg = { 0 };
	pmsg.hdr.msg_type = MCTP_MSG_TYPE_PLDM;
	pmsg.hdr.pldm_type = PLDM_TYPE_PLAT_MON_CTRL;
	pmsg.hdr.cmd = PLDM_MONITOR_CMD_CODE_GET_SENSOR_READING;
	pmsg.hdr.rq = PLDM_REQUEST;

	struct pldm_get_sensor_reading_req req = {0};
	struct pldm_get_sensor_reading_resp res = {0};
	req.sensor_id = sensor_num;
	req.rearm_event_state = 0;
	pmsg.len = sizeof(req);
	memcpy(pmsg.buf, (uint8_t*)&req, pmsg.len);

	uint16_t resp_len = mctp_pldm_read(mctp_inst, &pmsg, (uint8_t *)&res, sizeof(res));
	if (resp_len == 0) {
		LOG_ERR("Failed to get mpro sensor #%d reading", sensor_num);
		return SENSOR_FAIL_TO_ACCESS;
	}

	if (res.completion_code != PLDM_SUCCESS) {
		LOG_ERR("Get Mpro sensor #0x%x with bad cc 0x%x", sensor_num, res.completion_code);
		return SENSOR_FAIL_TO_ACCESS;
	}

	if (res.sensor_operational_state != PLDM_SENSOR_ENABLED) {
		LOG_ERR("Mpro sensor #%d in abnormal op state 0x%x", sensor_num, res.sensor_operational_state);
		return SENSOR_NOT_ACCESSIBLE;
	}

	LOG_INF("mpro sensor#0x%x", sensor_num);
	LOG_HEXDUMP_INF(res.present_reading, resp_len - 7, "");

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = 0;
	sval->fraction = 0;
	return SENSOR_READ_SUCCESS;
}

uint8_t mpro_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = mpro_read;
	return SENSOR_INIT_SUCCESS;
}

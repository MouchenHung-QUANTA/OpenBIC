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

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <stdlib.h>
#include "mctp.h"
#include "mctp_ctrl.h"
#include "pldm.h"
#include "ipmi.h"
#include "sensor.h"
#include "mpro.h"
#include "plat_hook.h"
#include "plat_mctp.h"
#include "plat_gpio.h"
#include "plat_pldm.h"

LOG_MODULE_REGISTER(plat_pldm);

struct _pldm_cmd_sup_lst {
	uint8_t pldm_type;
	uint8_t cmd;
};

#define PLDM_EVENT_NUM_POSTCODE 0xAF

struct _pldm_sensor_event_sup_lst {
	uint16_t sensor_id;
	uint8_t sensor_event_class;
	bool (*event_handler_func)(uint8_t *, uint16_t);
};

static bool mpro_postcode_collect(uint8_t *buf, uint16_t len);

static struct _pldm_sensor_event_sup_lst event_sensor_sup_lst[] = {
	{ PLDM_EVENT_NUM_POSTCODE, PLDM_NUMERIC_SENSOR_STATE, mpro_postcode_collect },
};

struct _pldm_cmd_sup_lst pldm_cmd_sup_tbl[] = {
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_GETTID },
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_GET_PLDM_TYPE },
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_GET_PLDM_CMDS },

	{ PLDM_TYPE_PLAT_MON_CTRL, PLDM_MONITOR_CMD_CODE_PLATFORM_EVENT_MESSAGE },
	{ PLDM_TYPE_PLAT_MON_CTRL, PLDM_MONITOR_CMD_CODE_GET_STATE_EFFECTER_STATES },
};

bool pldm_request_msg_need_bypass(uint8_t *buf, uint32_t len)
{
	CHECK_NULL_ARG_WITH_RETURN(buf, false);
	pldm_hdr *hdr = (pldm_hdr *)buf;

	/* Do not filter response message */
	if (!hdr->rq)
		return false;

	for (int i = 0; i < ARRAY_SIZE(pldm_cmd_sup_tbl); i++) {
		if ((hdr->pldm_type == pldm_cmd_sup_tbl[i].pldm_type) &&
		    (hdr->cmd == pldm_cmd_sup_tbl[i].cmd))
			return false;
	}

	/* Filter some commands with certain data */
	if ((hdr->pldm_type == PLDM_TYPE_PLAT_MON_CTRL) &&
	    (hdr->cmd == PLDM_MONITOR_CMD_CODE_PLATFORM_EVENT_MESSAGE)) {
		struct pldm_platform_event_message_req *req_p =
			(struct pldm_platform_event_message_req *)(buf + sizeof(*hdr));
		if (req_p->event_class == PLDM_SENSOR_EVENT) {
			uint16_t sensor_id = req_p->event_data[0] | (req_p->event_data[1] << 8);
			for (int i = 0; i < ARRAY_SIZE(event_sensor_sup_lst); i++) {
				if (sensor_id == event_sensor_sup_lst[i].sensor_id)
					return false;
			}
		}
		return true;
	}

	return true;
}

uint8_t pldm_platform_event_message(void *mctp_inst, uint8_t *buf, uint16_t len,
				    uint8_t instance_id, uint8_t *resp, uint16_t *resp_len,
				    void *ext_params)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_inst, PLDM_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(buf, PLDM_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(resp, PLDM_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(resp_len, PLDM_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(ext_params, PLDM_ERROR);

	struct pldm_platform_event_message_req *req_p =
		(struct pldm_platform_event_message_req *)buf;
	struct pldm_platform_event_message_resp *res_p =
		(struct pldm_platform_event_message_resp *)resp;

	LOG_INF("Recieved event class 0x%x", req_p->event_class);
	LOG_HEXDUMP_WRN(req_p->event_data, len - 3, "event data:");

	uint8_t ret_cc = PLDM_SUCCESS;

	uint8_t ret = pldm_event_len_check(&req_p->event_class, len - 2);
	if (ret == PLDM_ERROR_INVALID_LENGTH) {
		ret_cc = ret;
		goto exit;
	}

	switch (req_p->event_class) {
	case PLDM_SENSOR_EVENT: {
		uint16_t sensor_id = req_p->event_data[0] | (req_p->event_data[1] << 8);
		uint8_t sensor_event_class = req_p->event_data[2];

		int i = 0;
		for (i = 0; i < ARRAY_SIZE(event_sensor_sup_lst); i++) {
			if ((sensor_id == event_sensor_sup_lst[i].sensor_id) &&
			    (sensor_event_class == event_sensor_sup_lst[i].sensor_event_class)) {
				break;
			}
		}

		if (i == ARRAY_SIZE(event_sensor_sup_lst))
			goto exit;

		if (!event_sensor_sup_lst[i].event_handler_func) {
			LOG_ERR("Event class 0x%x sensor id 0x%x lost handler", req_p->event_class,
				sensor_id);
			goto exit;
		}

		if (event_sensor_sup_lst[i].event_handler_func(&req_p->event_data[3], len - 5) ==
		    false) {
			LOG_ERR("Event class 0x%x sensor id 0x%x lost handler got error",
				req_p->event_class, sensor_id);
			goto exit;
		}
		break;
	}

	default:
		LOG_WRN("Event class 0x%x not supported yet!", req_p->event_class);
		goto exit;
	}

exit:
	res_p->completion_code = ret_cc;
	res_p->platform_event_status = 0x00; // BIC always not log record

	*resp_len = 2;

	return PLDM_SUCCESS;
}

static bool mpro_postcode_collect(uint8_t *buf, uint16_t len)
{
	CHECK_ARG_WITH_RETURN(buf, false);

	LOG_HEXDUMP_INF(buf, len, "* postcode: ");

	if (len < 4) {
		LOG_ERR("Received invalid length postcode data");
		return false;
	}

	uint32_t postcode = 0;
	for (int i = 0; i < len; i++)
		postcode |= (buf[i] << (i * 8));

	mpro_postcode_insert(postcode);

	return true;
}

#define PLDM_MAX_INSTID_COUNT 32
static uint32_t inst_table_for_ipmb;
static bridge_store store_table[PLDM_MAX_INSTID_COUNT];

bool pldm_save_mctp_inst_from_ipmb_req(void *mctp_inst, uint8_t inst_num,
				       mctp_ext_params ext_params)
{
	CHECK_ARG_WITH_RETURN(mctp_inst, false);

	if (inst_num >= PLDM_MAX_INSTID_COUNT) {
		LOG_ERR("Invalid instance number %d", inst_num);
		return false;
	}

	if (!(inst_table_for_ipmb & BIT(inst_num))) {
		LOG_ERR("Instant id %d has been used!", inst_num);
		return false;
	}

	WRITE_BIT(inst_table_for_ipmb, inst_num, 1);
	store_table[inst_num].mctp_inst = (mctp *)mctp_inst;
	store_table[inst_num].ext_params = ext_params;

	return true;
}

bridge_store *pldm_find_mctp_inst_by_inst_id(uint8_t inst_num)
{
	if (!(inst_table_for_ipmb & BIT(inst_num))) {
		LOG_ERR("Received unexpected inatant id %d not register yet!", inst_num);
		return NULL;
	}

	WRITE_BIT(inst_table_for_ipmb, inst_num, 0);

	return &store_table[inst_num];
}

bool pldm_send_ipmb_rsp(ipmi_msg *msg)
{
	CHECK_NULL_ARG_WITH_RETURN(msg, false);

	if ((msg->netfn != (NETFN_OEM_1S_REQ + 1)) || (msg->cmd != CMD_OEM_1S_SEND_PLDM_TO_IPMB)) {
		LOG_ERR("Non-support NetFn 0x%x and Cmd 0x%x", msg->netfn, msg->cmd);
		return false;
	}

	if (msg->data_len < sizeof(pldm_hdr)) {
		LOG_ERR("Received invalid data length");
		return false;
	}

	pldm_hdr *hdr = (pldm_hdr *)msg->data;
	bridge_store *hdr_info = pldm_find_mctp_inst_by_inst_id(hdr->inst_id);
	if (!hdr_info) {
		LOG_ERR("Given instant num %d can't get mctp header while ipmb response",
			hdr->inst_id);
		return false;
	}

	pldm_msg pmsg = { 0 };
	pmsg.ext_params = hdr_info->ext_params;
	memcpy(&pmsg.hdr, &msg->data[0], sizeof(pmsg.hdr));
	pmsg.hdr.rq = PLDM_RESPONSE;

	if (msg->completion_code == CC_CAN_NOT_RESPOND) {
		pmsg.buf[0] = PLDM_ERROR;
		pmsg.len = 1;
	} else {
		pmsg.buf = msg->data + sizeof(pmsg.hdr);
		pmsg.len = msg->data_len - sizeof(pmsg.hdr);
	}

	LOG_HEXDUMP_INF(pmsg.buf, pmsg.len, "receive pldm rsp from ipmb:");

	// Send response to PLDM/MCTP thread
	mctp_pldm_send_msg(hdr_info->mctp_inst, &pmsg);

	return true;
}
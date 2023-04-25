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

uint8_t pldm_platform_event_message(const void *mctp_inst, const uint8_t *buf, uint16_t len,
				    uint8_t *resp, uint16_t *resp_len, const void *ext_params)
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

	/* TODO: Collect data to  */

	return true;
}

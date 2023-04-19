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

LOG_MODULE_REGISTER(plat_pldm);

#define NETFN_OEM_PLDM_TO_IPMB 0xAA
#define CMD_OEM_PLDM_TO_IPMB 0xAA

struct _pldm_cmd_sup_lst {
	uint8_t pldm_type;
	uint8_t cmd;
};

struct _pldm_cmd_sup_lst pldm_cmd_sup_tbl[] = {
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_SETTID },
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_GETTID },
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_GET_PLDM_TYPE },
	{ PLDM_TYPE_BASE, PLDM_BASE_CMD_CODE_GET_PLDM_CMDS },

	{ PLDM_TYPE_PLAT_MON_CTRL, PLDM_MONITOR_CMD_CODE_GET_SENSOR_READING },
	{ PLDM_TYPE_PLAT_MON_CTRL, PLDM_MONITOR_CMD_CODE_SET_EVENT_RECEIVER },
	{ PLDM_TYPE_PLAT_MON_CTRL, PLDM_MONITOR_CMD_CODE_SET_STATE_EFFECTER_STATES },
	{ PLDM_TYPE_PLAT_MON_CTRL, PLDM_MONITOR_CMD_CODE_GET_STATE_EFFECTER_STATES },

	{ PLDM_TYPE_OEM, PLDM_OEM_CMD_ECHO },
	{ PLDM_TYPE_OEM, PLDM_OEM_IPMI_BRIDGE }
};
/*
bool pal_pldm_request_msg_filter(uint8_t *buff, uint32_t len, uint8_t *rsp_buff, uint32_t *rsp_len)
{
	CHECK_NULL_ARG_WITH_RETURN(buff, false);

	pldm_hdr *hdr = (pldm_hdr *)buff;

	for (int i=0; i<ARRAY_SIZE(pldm_cmd_sup_tbl); i++) {
		if ( (hdr->pldm_type == pldm_cmd_sup_tbl[i].pldm_type) && (hdr->cmd == pldm_cmd_sup_tbl[i].cmd) )
			return true;
	}

	uint8_t seq_source = 0xFF;

	ipmi_msg msg;
	memset(&msg, 0, sizeof(ipmi_msg));
	msg = construct_ipmi_message(seq_source, NETFN_OEM_PLDM_TO_IPMB, CMD_OEM_PLDM_TO_IPMB, SELF,
					   BMC_IPMB, len, buff);
	ipmb_error ipmb_ret = ipmb_read(&msg, IPMB_inf_index_map[msg.InF_target]);
	if ((ipmb_ret != IPMB_ERROR_SUCCESS) || (msg.completion_code != CC_SUCCESS)) {
		LOG_ERR("[%s] fail to send get dimm temperature command ret: 0x%x CC: 0x%x\n",
		       __func__, ipmb_ret, msg.completion_code);
		return false;
	}

	return false;
}
*/
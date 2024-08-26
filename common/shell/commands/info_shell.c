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

#include "info_shell.h"
#include "plat_version.h"
#include "util_sys.h"
#include "plat_mctp.h"
#include "mctp_ctrl.h"
#include "stdlib.h"

#ifndef CONFIG_BOARD
#define CONFIG_BOARD "unknown"
#endif

#define RTOS_TYPE "Zephyr"

int cmd_info_print(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
	shell_print(shell, "* NAME:          Platform command");
	shell_print(shell, "* DESCRIPTION:   Commands that could be used to debug or validate.");
	shell_print(shell, "* DATE/VERSION:  none");
	shell_print(shell, "* CHIP/OS:       %s - %s", CONFIG_BOARD, RTOS_TYPE);
	shell_print(shell, "* Note:          none");
	shell_print(shell, "------------------------------------------------------------------");
	shell_print(shell, "* PLATFORM:      %s-%s", PLATFORM_NAME, PROJECT_NAME);
	shell_print(shell, "* BOARD ID:      %d", BOARD_ID);
	shell_print(shell, "* STAGE:         %d", PROJECT_STAGE);
	shell_print(shell, "* SYSTEM:        %d", get_system_class());
	shell_print(shell, "* FW VERSION:    %d.%d", FIRMWARE_REVISION_1, FIRMWARE_REVISION_2);
	shell_print(shell, "* FW DATE:       %x%x.%x.%x", BIC_FW_YEAR_MSB, BIC_FW_YEAR_LSB,
		    BIC_FW_WEEK, BIC_FW_VER);
	shell_print(shell, "* FW IMAGE:      %s.bin", CONFIG_KERNEL_BIN_NAME);
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
#if 0
	mctp *mctp_inst = NULL;
	mctp_ctrl_msg msg = { 0 };

	if (get_mctp_info_by_eid(0xf0, &mctp_inst, &msg.ext_params) == false) {
		shell_error(shell, "Failed to get mctp info by eid 0x%x", 0xf0);
		return 0;
	}

	struct _set_eid_req set_eid_req = { 0 };
	struct _set_eid_resp set_eid_resp = { 0 };
	set_eid_req.op = SET_EID_REQ_OP_SET_EID;
	set_eid_req.eid = 0xf0;

	msg.hdr.cmd = MCTP_CTRL_CMD_SET_ENDPOINT_ID;
	msg.hdr.rq = MCTP_REQUEST;
	msg.cmd_data = (uint8_t *)&set_eid_req;
	msg.cmd_data_len = sizeof(set_eid_req);

	uint8_t ret = mctp_ctrl_read(mctp_inst, &msg, (uint8_t *)&set_eid_resp,
					sizeof(set_eid_resp));
	if (ret != MCTP_SUCCESS) {
		shell_error(shell, "Fail to set eid to SatMC");
		return 0;
	}

	if (set_eid_resp.completion_code != MCTP_SUCCESS) {
		shell_error(shell, "Fail to set eid to SatMC, completion code: 0x%x",
			set_eid_resp.completion_code);
		return 0;
	}

	shell_print(shell, "Set EID to SatMC successfully");
#endif

#if 0
	static uint16_t ii = 1000;
	uint8_t mctp_dest_eid = 0x08;
	uint8_t pldm_type = 0x3f;
	uint8_t pldm_cmd = 0x01;

	uint8_t *req_p = (uint8_t *)malloc(ii * sizeof(uint8_t));
	if (req_p == NULL) {
		shell_error(shell, "Failed to allocate memory");
		return 0;
	}

	memset(req_p, 0, ii);

	req_p[0] = 0x15;
	req_p[1] = 0xa0;
	req_p[2] = 0x00;
	req_p[3] = 0x18;
	req_p[4] = 0x01;

	shell_print(shell, "~~ data len %d", ii);

	uint8_t resp_buf[125] = { 0 };
	pldm_msg pmsg = { 0 };
	pmsg.hdr.msg_type = MCTP_MSG_TYPE_PLDM;
	pmsg.hdr.pldm_type = pldm_type;
	pmsg.hdr.cmd = pldm_cmd;
	pmsg.hdr.rq = PLDM_REQUEST;
	pmsg.len = ii;
	pmsg.buf = req_p;

	mctp *mctp_inst = NULL;
	if (get_mctp_info_by_eid(mctp_dest_eid, &mctp_inst, &pmsg.ext_params) == false) {
		shell_error(shell, "Failed to get mctp info by eid 0x%x", mctp_dest_eid);
		goto exit;
	}

	printf("mctp_inst: %p\n", mctp_inst);
	uint16_t resp_len = mctp_pldm_read(mctp_inst, &pmsg, resp_buf, sizeof(resp_buf));
	if (resp_len == 0) {
		shell_error(shell, "Failed to get mctp response");
		goto exit;
	}

	shell_print(shell, "* mctp: 0x%x addr: 0x%x eid: 0x%x msg_type: 0x%x", mctp_inst,
		    pmsg.ext_params.smbus_ext_params.addr, mctp_dest_eid, MCTP_MSG_TYPE_PLDM);
	shell_print(shell, "  pldm_type: 0x%x pldm cmd: 0x%x", pldm_type, pldm_cmd);
	shell_hexdump(shell, pmsg.buf, pmsg.len);

	if (resp_buf[0] != PLDM_SUCCESS)
		shell_error(shell, "Response with bad cc 0x%x", resp_buf[0]);
	else {
		shell_hexdump(shell, resp_buf, resp_len);
		shell_print(shell, "");
	}
	ii+=1;
exit:
	free(req_p);
#endif
	return 0;
}

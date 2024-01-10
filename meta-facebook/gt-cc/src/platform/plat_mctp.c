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

#include "pldm_monitor.h"
#include "pdr.h"
#include "nvidia.h"
#include "plat_def.h"

LOG_MODULE_REGISTER(plat_mctp);

/* i2c 8 bit address */
#define I2C_ADDR_BIC 0x40
#define I2C_ADDR_BMC 0x20
#define I2C_ADDR_NIC 0x64

/* i2c dev bus */
#define I2C_BUS_BMC 0x06
#define I2C_BUS_NIC_0 0x00
#define I2C_BUS_NIC_1 0x01
#define I2C_BUS_NIC_2 0x02
#define I2C_BUS_NIC_3 0x03
#define I2C_BUS_NIC_4 0x0A
#define I2C_BUS_NIC_5 0x0B
#define I2C_BUS_NIC_6 0x0C
#define I2C_BUS_NIC_7 0x0D
/* mctp endpoint */
#define MCTP_EID_BMC 0x08
#define MCTP_EID_NIC_0 0x10
#define MCTP_EID_NIC_1 0x10
#define MCTP_EID_NIC_2 0x11
#define MCTP_EID_NIC_3 0x12
#define MCTP_EID_NIC_4 0x0b
#define MCTP_EID_NIC_5 0x0c
#define MCTP_EID_NIC_6 0x0d
#define MCTP_EID_NIC_7 0x0e

K_TIMER_DEFINE(send_cmd_timer, send_cmd_to_dev, NULL);
K_WORK_DEFINE(send_cmd_work, send_cmd_to_dev_handler);

static mctp_port smbus_port[] = {
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_BMC },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_0 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_1 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_2 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_3 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_4 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_5 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_6 },
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_NIC_7 },
};
/*
mctp_route_entry mctp_route_tbl[] = {
	{ MCTP_EID_BMC, I2C_BUS_BMC, I2C_ADDR_BMC },
	{ MCTP_EID_NIC_0, I2C_BUS_NIC_7, 0x70, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_1, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_2, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_3, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_4, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_5, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_6, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
	{ MCTP_EID_NIC_7, I2C_BUS_NIC_7, 0x60, PRSNT_NIC7_R_N },
};
*/
mctp_route_entry mctp_route_tbl[] = {
	{ MCTP_EID_BMC, I2C_BUS_BMC, I2C_ADDR_BMC },
	{ MCTP_EID_NIC_0, I2C_BUS_NIC_0, I2C_ADDR_NIC, PRSNT_NIC0_R_N },
	{ 0x00, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x08, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x09, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x0a, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x0b, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x0c, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x0d, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x0e, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x0f, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x10, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x11, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x12, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x13, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x14, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x15, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x16, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x17, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x18, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x19, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x1a, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x1b, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
	{ 0x1c, I2C_BUS_NIC_7, 0x28, PRSNT_NIC7_R_N },
};

static mctp *find_mctp_by_smbus(uint8_t bus)
{
	uint8_t i;
	for (i = 0; i < ARRAY_SIZE(smbus_port); i++) {
		mctp_port *p = smbus_port + i;

		if (bus == p->conf.smbus_conf.bus)
			return p->mctp_inst;
	}

	return NULL;
}

uint8_t get_mctp_info(uint8_t dest_endpoint, mctp **mctp_inst, mctp_ext_params *ext_params)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_inst, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(ext_params, MCTP_ERROR);

	uint8_t rc = MCTP_ERROR;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;
		if (!p) {
			return MCTP_ERROR;
		}
		if (p->endpoint == dest_endpoint) {
			*mctp_inst = find_mctp_by_smbus(p->bus);
			ext_params->type = MCTP_MEDIUM_TYPE_SMBUS;
			ext_params->smbus_ext_params.addr = p->addr;
			rc = MCTP_SUCCESS;
			break;
		}
	}
	return rc;
}

static void set_endpoint_resp_handler(void *args, uint8_t *buf, uint16_t len)
{
	if (!buf || !len)
		return;
	LOG_HEXDUMP_WRN(buf, len, __func__);
}

static void set_endpoint_resp_timeout(void *args)
{
	mctp_route_entry *p = (mctp_route_entry *)args;
	printk("[%s] Endpoint 0x%x set endpoint failed on bus %d \n", __func__, p->endpoint,
	       p->bus);
}

static void set_dev_endpoint(void)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;

		/* skip BMC */
		if (p->bus == I2C_BUS_BMC && p->addr == I2C_ADDR_BMC)
			continue;

		if (gpio_get(p->dev_present_pin))
			continue;

		for (uint8_t j = 0; j < ARRAY_SIZE(smbus_port); j++) {
			if (p->bus != smbus_port[j].conf.smbus_conf.bus)
				continue;
			
			LOG_INF("SET DEVICE EID 0x%x...", p->endpoint);

			struct _set_eid_req req = { 0 };
			req.op = SET_EID_REQ_OP_SET_EID;
			req.eid = p->endpoint;

			mctp_ctrl_msg msg;
			memset(&msg, 0, sizeof(msg));
			msg.ext_params.type = MCTP_MEDIUM_TYPE_SMBUS;
			msg.ext_params.smbus_ext_params.addr = p->addr;

			msg.hdr.cmd = MCTP_CTRL_CMD_SET_ENDPOINT_ID;
			msg.hdr.rq = 1;

			msg.cmd_data = (uint8_t *)&req;
			msg.cmd_data_len = sizeof(req);

			msg.recv_resp_cb_fn = set_endpoint_resp_handler;
			msg.timeout_cb_fn = set_endpoint_resp_timeout;
			msg.timeout_cb_fn_args = p;

			LOG_INF("EID: 0x%x bus: %d addr:0x%x", p->endpoint, p->bus, p->addr);

			mctp_ctrl_send_msg(find_mctp_by_smbus(p->bus), &msg);
		}
	}
}

static void get_dev_firmware_resp_timeout(void *args)
{
	mctp_route_entry *p = (mctp_route_entry *)args;
	printk("[%s] Endpoint 0x%x get parameter failed on bus %d \n", __func__, p->endpoint,
	       p->bus);
}

struct pldm_variable_field nic_vesion[8];

static void get_dev_firmware_resp_handler(void *args, uint8_t *buf, uint16_t len)
{
	CHECK_NULL_ARG(args);
	CHECK_NULL_ARG(buf);

	if (!len)
		return;

	mctp_route_entry *p = (mctp_route_entry *)args;
	struct pldm_get_firmware_parameters_resp *response =
		(struct pldm_get_firmware_parameters_resp *)buf;

	uint8_t nic_index = p->endpoint - MCTP_EID_NIC_0;

	SAFE_FREE(nic_vesion[nic_index].ptr);
	nic_vesion[nic_index].ptr =
		(uint8_t *)malloc(sizeof(uint8_t) * response->active_comp_image_set_ver_str_len);

	if (!nic_vesion[nic_index].ptr) {
		LOG_ERR("The buffer of NIC%d version memory allocate failed", nic_index);
		return;
	}

	nic_vesion[nic_index].length = response->active_comp_image_set_ver_str_len;
	memcpy(nic_vesion[nic_index].ptr, buf + sizeof(struct pldm_get_firmware_parameters_resp),
	       nic_vesion[nic_index].length);
}

static void get_dev_firmware_parameters(void)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;

		if (p->addr != I2C_ADDR_NIC)
			continue;

		if (gpio_get(p->dev_present_pin))
			continue;

		for (uint8_t j = 0; j < ARRAY_SIZE(smbus_port); j++) {
			if (p->bus != smbus_port[j].conf.smbus_conf.bus)
				continue;
		}
		pldm_msg msg = { 0 };

		msg.ext_params.type = MCTP_MEDIUM_TYPE_SMBUS;
		msg.ext_params.smbus_ext_params.addr = p->addr;

		msg.hdr.pldm_type = PLDM_TYPE_FW_UPDATE;
		msg.hdr.cmd = 0x02;
		msg.hdr.rq = 1;
		msg.len = 0;

		msg.recv_resp_cb_fn = get_dev_firmware_resp_handler;
		msg.recv_resp_cb_args = p;
		msg.timeout_cb_fn = get_dev_firmware_resp_timeout;
		msg.timeout_cb_fn_args = p;

		mctp_pldm_send_msg(find_mctp_by_smbus(p->bus), &msg);
	}
}

static bool get_pdr_send_req(mctp *mctp_inst, struct pldm_get_pdr_req *req, struct pldm_get_pdr_resp *rsp, mctp_ext_params ext_params)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_inst, false);
	CHECK_NULL_ARG_WITH_RETURN(req, false);
	CHECK_NULL_ARG_WITH_RETURN(rsp, false);

	if (req->request_count > NUMERIC_PDR_SIZE) {
		LOG_WRN("Request pdr data response size over limit 0x%x", NUMERIC_PDR_SIZE);
		return false;
	}

	uint8_t resp_buf[PLDM_MAX_DATA_SIZE] = { 0 };
	pldm_msg pmsg = { 0 };
	pmsg.hdr.msg_type = MCTP_MSG_TYPE_PLDM;
	pmsg.hdr.pldm_type = 0x02;
	pmsg.hdr.cmd = PLDM_MONITOR_CMD_CODE_GET_PDR;
	pmsg.hdr.rq = PLDM_REQUEST;
	pmsg.len = sizeof(struct pldm_get_pdr_req);
	pmsg.buf = (uint8_t *)req;
	pmsg.ext_params = ext_params;

	uint16_t resp_len = mctp_pldm_read(mctp_inst, &pmsg, resp_buf, sizeof(resp_buf));
	if (resp_len == 0) {
		LOG_ERR("Failed to get mctp response...");
		return false;
	}

	if (resp_buf[0] != PLDM_SUCCESS) {
		LOG_ERR("GetPDR: Get bad cc 0x%x", resp_buf[0]);
		return false;
	}

	LOG_HEXDUMP_DBG(resp_buf, resp_len, "resp_buf:");
	memcpy(rsp, (struct pldm_get_pdr_resp *)resp_buf, resp_len);

	return true;
}

bool mctp_add_sel_to_ipmi(common_addsel_msg_t *sel_msg)
{
	CHECK_NULL_ARG_WITH_RETURN(sel_msg, false);

	uint8_t system_event_record = 0x02;
	uint8_t evt_msg_version = 0x04;

	pldm_msg msg = { 0 };
	struct mctp_to_ipmi_sel_req req = { 0 };

	msg.ext_params.type = MCTP_MEDIUM_TYPE_SMBUS;
	msg.ext_params.smbus_ext_params.addr = I2C_ADDR_BMC;

	msg.hdr.pldm_type = PLDM_TYPE_OEM;
	msg.hdr.cmd = PLDM_OEM_IPMI_BRIDGE;
	msg.hdr.rq = 1;

	msg.buf = (uint8_t *)&req;
	msg.len = sizeof(struct mctp_to_ipmi_sel_req);

	if (set_iana(req.header.iana, sizeof(req.header.iana))) {
		LOG_ERR("Set IANA fail");
		return false;
	}

	req.header.netfn_lun = NETFN_STORAGE_REQ;
	req.header.ipmi_cmd = CMD_STORAGE_ADD_SEL;
	req.req_data.event.record_type = system_event_record;
	req.req_data.event.gen_id[0] = (I2C_ADDR_BIC << 1);
	req.req_data.event.evm_rev = evt_msg_version;

	memcpy(&req.req_data.event.sensor_type, &sel_msg->sensor_type,
	       sizeof(common_addsel_msg_t) - sizeof(uint8_t));

	uint8_t resp_len = sizeof(struct mctp_to_ipmi_sel_resp);
	uint8_t rbuf[resp_len];

	if (!mctp_pldm_read(find_mctp_by_smbus(I2C_BUS_BMC), &msg, rbuf, resp_len)) {
		LOG_ERR("mctp_pldm_read fail");
		return false;
	}

	struct mctp_to_ipmi_sel_resp *resp = (struct mctp_to_ipmi_sel_resp *)rbuf;

	if ((resp->header.completion_code != MCTP_SUCCESS) ||
	    (resp->header.ipmi_comp_code != CC_SUCCESS)) {
		LOG_ERR("Check reponse completion code fail");
		return false;
	}

	return true;
}

static uint8_t mctp_msg_recv(void *mctp_p, uint8_t *buf, uint32_t len, mctp_ext_params ext_params)
{
	if (!mctp_p || !buf || !len)
		return MCTP_ERROR;

	/* first byte is message type and ic */
	uint8_t msg_type = (buf[0] & MCTP_MSG_TYPE_MASK) >> MCTP_MSG_TYPE_SHIFT;
	uint8_t ic = (buf[0] & MCTP_IC_MASK) >> MCTP_IC_SHIFT;
	(void)ic;

	switch (msg_type) {
	case MCTP_MSG_TYPE_CTRL:
		mctp_ctrl_cmd_handler(mctp_p, buf, len, ext_params);
		break;

	case MCTP_MSG_TYPE_PLDM:
		mctp_pldm_cmd_handler(mctp_p, buf, len, ext_params);
		break;

	default:
		LOG_WRN("Cannot find message receive function!!");
		return MCTP_ERROR;
	}

	return MCTP_SUCCESS;
}

static uint8_t get_mctp_route_info(uint8_t dest_endpoint, void **mctp_inst,
				   mctp_ext_params *ext_params)
{
	if (!mctp_inst || !ext_params)
		return MCTP_ERROR;

	uint8_t rc = MCTP_ERROR;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;
		if (p->endpoint == dest_endpoint) {
			if ((p->addr == I2C_ADDR_NIC) && gpio_get(p->dev_present_pin))
				return MCTP_ERROR;
			*mctp_inst = find_mctp_by_smbus(p->bus);
			ext_params->type = MCTP_MEDIUM_TYPE_SMBUS;
			ext_params->smbus_ext_params.addr = p->addr;
			rc = MCTP_SUCCESS;
			break;
		}
	}

	return rc;
}

void send_cmd_to_dev_handler(struct k_work *work)
{
	/* init the device endpoint */
	set_dev_endpoint();
	/* get device parameters */
	get_dev_firmware_parameters();
}
void send_cmd_to_dev(struct k_timer *timer)
{
	k_work_submit(&send_cmd_work);
}

#define MAX_PDR_RSP 0x25
void pdr_handler(void *arug0, void *arug1, void *arug2)
{
	k_msleep(3000);

	mctp *mctp_inst = NULL;
	mctp_ext_params ext_params = {0};
	if (get_mctp_info_by_eid(MCTP_EID_NIC_0, &mctp_inst, &ext_params) == false) {
		LOG_ERR("Failed to get mctp info by eid 0x%x", MCTP_EID_NIC_0);
		return;
	}

	LOG_INF("[STEP1] Set Event Receiver");
	uint8_t resp_buf[128] = { 0 };
	pldm_msg pmsg = { 0 };
	pmsg.hdr.msg_type = MCTP_MSG_TYPE_PLDM;
	pmsg.hdr.pldm_type = 0x02;
	pmsg.hdr.cmd = 0x05;
	pmsg.hdr.rq = PLDM_REQUEST;
	pmsg.len = 0;
	pmsg.ext_params = ext_params;

	uint16_t resp_len = mctp_pldm_read(mctp_inst, &pmsg, resp_buf, sizeof(resp_buf));
	if (resp_len == 0) {
		LOG_ERR("Failed to get mctp response...");
		goto step2;
	}

	if (resp_buf[0] != PLDM_SUCCESS) {
		LOG_ERR("GetEventReceiver: Get bad cc 0x%x", resp_buf[0]);
		goto step2;
	}

	LOG_HEXDUMP_INF(resp_buf, resp_len, "resp_buf:");

step2:
	LOG_INF("[STEP2] Get PDR");

	struct pldm_get_pdr_req req_data = {0};
	struct pldm_get_pdr_resp rsp_data = {0};

	PDR_common_header common_hdr = {0}; 

	uint16_t pdr_idx = 0;
	bool first_pdr = true;

	req_data.record_handle = 0x00000000;
	req_data.data_transfer_handle = 0x00000000;
	req_data.record_change_number = 0;
	req_data.request_count = MAX_PDR_RSP;
	req_data.transfer_operation_flag = 0x01;

	LOG_WRN("PDR requirement task start!");
	while (1)
	{
		if (first_pdr == false) {

		}

		LOG_INF("=============== Get PDR No.%d ===============", pdr_idx);
		LOG_INF("[req]");
		LOG_INF("* record_handle:             0x%x", req_data.record_handle);
		LOG_INF("* data_transfer_handle:      0x%x", req_data.data_transfer_handle);
		LOG_INF("* transfer_operation_flag:   0x%x", req_data.transfer_operation_flag);
		LOG_INF("* request_count:             0x%x", req_data.request_count);
		LOG_INF("* record_change_number:      0x%x", req_data.record_change_number);

		/* get pdr from NIC_0 */
		if (get_pdr_send_req(mctp_inst, &req_data, &rsp_data, ext_params) == false) {
			break;;
		}

		LOG_INF("[rsp]");
		LOG_INF("* next_record_handle:        0x%x", rsp_data.next_record_handle);
		LOG_INF("* next_data_transfer_handle: 0x%x", rsp_data.next_data_transfer_handle);

		char *trans_flag_name;
		if (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START)
			trans_flag_name = "Start";
		else if (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_MIDDLE)
			trans_flag_name = "Middle";
		else if (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_END)
			trans_flag_name = "Eed";
		else if (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START_AND_END)
			trans_flag_name = "StartAndEnd";
		else
			trans_flag_name = "---";

		LOG_INF("* transfer_flag:             0x%x (%s)", rsp_data.transfer_flag, trans_flag_name);
		LOG_INF("* response_count:            0x%x", rsp_data.response_count);
		LOG_HEXDUMP_INF(rsp_data.record_data, rsp_data.response_count, "* record_data:");

		if ( (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START) || (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START_AND_END) ) {
			memcpy(&common_hdr, rsp_data.record_data, sizeof(PDR_common_header));

			LOG_INF("[common pdr header]");
			LOG_INF("   + record_handle:        0x%x", common_hdr.record_handle);
			LOG_INF("   + pdr_header_version:   0x%x", common_hdr.PDR_header_version);

			char *pdr_type_name;
			switch (common_hdr.PDR_type)
			{
			case PLDM_TERMINUS_LOCATOR_PDR:
				pdr_type_name = "TERMINUS LOCATOR PDR";
				break;
			case PLDM_NUMERIC_SENSOR_PDR:
				pdr_type_name = "NUMERIC SENSOR PDR";
				break;
			case PLDM_NUMERIC_SENSOR_INITIALIZATION_PDR:
				pdr_type_name = "NUMERIC SENSOR INITIALIZATION PDR";
				break;
			case PLDM_STATE_SENSOR_PDR:
				pdr_type_name = "STATE SENSOR PDR";
				break;
			case PLDM_STATE_SENSOR_INITIALIZATION_PDR:
				pdr_type_name = "STATE SENSOR INITIALIZATION PDR";
				break;
			case PLDM_SENSOR_AUXILIARY_NAMES_PDR:
				pdr_type_name = "SENSOR AUXILIARY NAMES PDR";
				break;
			case PLDM_OEM_UNIT_PDR:
				pdr_type_name = "OEM UNIT PDR";
				break;
			case PLDM_OEM_STATE_SET_PDR:
				pdr_type_name = "OEM STATE SET PDR";
				break;
			case PLDM_NUMERIC_EFFECTER_PDR:
				pdr_type_name = "NUMERIC EFFECTER PDR";
				break;
			case PLDM_NUMERIC_EFFECTER_INITIALIZATION_PDR:
				pdr_type_name = "NUMERIC EFFECTER INITIALIZATION PDR";
				break;
			case PLDM_STATE_EFFECTER_PDR:
				pdr_type_name = "STATE EFFECTER PDR";
				break;
			case PLDM_STATE_EFFECTER_INITIALIZATION_PDR:
				pdr_type_name = "STATE EFFECTER INITIALIZATION PDR";
				break;
			case PLDM_EFFECTER_AUXILIARY_NAMES_PDR:
				pdr_type_name = "EFFECTER AUXILIARY NAMES PDR";
				break;
			case PLDM_EFFECTER_OEM_SEMANTIC_PDR:
				pdr_type_name = "EFFECTER OEM SEMANTIC PDR";
				break;
			case PLDM_PDR_ENTITY_ASSOCIATION:
				pdr_type_name = "ENTITY ASSOCIATION PDR";
				break;
			case PLDM_ENTITY_AUXILIARY_NAMES_PDR:
				pdr_type_name = "ENTITY AUXILIARY NAMES PDR";
				break;
			case PLDM_OEM_ENTITY_ID_PDR:
				pdr_type_name = "OEM ENTITY ID PDR";
				break;
			case PLDM_INTERRUPT_ASSOCIATION_PDR:
				pdr_type_name = "INTERRUPT ASSOCIATION PDR";
				break;
			case PLDM_EVENT_LOG_PDR:
				pdr_type_name = "EVENT LOG PDR";
				break;
			case PLDM_PDR_FRU_RECORD_SET:
				pdr_type_name = "FRU RECORD SET PDR";
				break;
			case PLDM_COMPACT_NUMERIC_SENSOR_PDR:
				pdr_type_name = "COMPACT NUMERIC SENSOR PDR";
				break;
			case PLDM_OEM_DEVICE_PDR:
				pdr_type_name = "OEM DEVICE PDR";
				break;
			case PLDM_OEM_PDR:
				pdr_type_name = "OEM PDR";
				break;

			default:
				pdr_type_name = "---";
				break;
			}

			LOG_INF("   + pdr_type:             0x%x (%s)", common_hdr.PDR_type, pdr_type_name);
			LOG_INF("   + record_change_number: 0x%x", common_hdr.record_change_number);
			LOG_INF("   + data_length:          0x%x", common_hdr.data_length);
		}

#ifdef ENABLE_NVIDIA
		static uint8_t cur_numeric_sensor_buff[sizeof(PDR_numeric_sensor)] = {0};
		static int cur_numeric_sensor_idx = 0;

		if (common_hdr.PDR_type == PLDM_NUMERIC_SENSOR_PDR) {
			memcpy(&cur_numeric_sensor_buff[cur_numeric_sensor_idx], rsp_data.record_data, rsp_data.response_count);
			cur_numeric_sensor_idx += rsp_data.response_count;

			if (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_END || rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START_AND_END) {
				nv_satmc_pdr_collect(PLDM_NUMERIC_SENSOR_PDR, cur_numeric_sensor_buff, cur_numeric_sensor_idx + 1);
				memset(cur_numeric_sensor_buff, 0, sizeof(PDR_numeric_sensor));
				cur_numeric_sensor_idx = 0;
			}
		}
#endif

		if (rsp_data.next_record_handle == 0) {
			LOG_WRN("Last PDR has been received(total: %d)", pdr_idx+1);
			break;
		}

		req_data.record_handle = rsp_data.next_record_handle;
		req_data.data_transfer_handle = rsp_data.next_data_transfer_handle;

		if ( (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_END) || (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START_AND_END) ) {
			LOG_INF("");
			req_data.transfer_operation_flag = 0x01;
			pdr_idx++;
		} else if ( (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_START) || (rsp_data.transfer_flag == PLDM_TRANSFER_FLAG_MIDDLE) ) {
			req_data.transfer_operation_flag = 0x00;
		} else {
			LOG_ERR("Receivce invalid transfer flag 0x%x", rsp_data.transfer_flag);
			break;
		}

		k_msleep(500);
	}
}

struct k_thread pdr_thread;
K_KERNEL_STACK_MEMBER(pdr_thread_stack, 4096);
void plat_mctp_init(void)
{
	LOG_INF("plat_mctp_init");

	/* init the mctp/pldm instance */
	for (uint8_t i = 0; i < ARRAY_SIZE(smbus_port); i++) {
		mctp_port *p = smbus_port + i;
		LOG_INF("smbus port %d", i);
		LOG_INF("bus = %x, addr = %x", p->conf.smbus_conf.bus, p->conf.smbus_conf.addr);

		p->mctp_inst = mctp_init();
		if (!p->mctp_inst) {
			LOG_ERR("mctp_init failed!!");
			continue;
		}

		LOG_INF("mctp_inst = %p", p->mctp_inst);
		uint8_t rc =
			mctp_set_medium_configure(p->mctp_inst, MCTP_MEDIUM_TYPE_SMBUS, p->conf);
		LOG_INF("mctp_set_medium_configure %s",
			(rc == MCTP_SUCCESS) ? "success" : "failed");

		mctp_reg_endpoint_resolve_func(p->mctp_inst, get_mctp_route_info);
		mctp_reg_msg_rx_func(p->mctp_inst, mctp_msg_recv);

		mctp_start(p->mctp_inst);
	}

	/* Only send command to device when DC on */
	if (is_mb_dc_on())
		k_timer_start(&send_cmd_timer, K_MSEC(3000), K_NO_WAIT);

	k_thread_create(&pdr_thread, pdr_thread_stack, K_THREAD_STACK_SIZEOF(pdr_thread_stack),
			pdr_handler, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&pdr_thread, "PDR_thread");

}

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "plat_mctp.h"

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include "mctp.h"
#include "mctp_ctrl.h"
#include "pldm.h"
#include "ipmi.h"
#include "sensor.h"
#include "plat_hook.h"
#include "plat_ipmb.h"

#include "hal_i3c.h"

LOG_MODULE_REGISTER(plat_mctp);

#define MCTP_MSG_TYPE_SHIFT 0
#define MCTP_MSG_TYPE_MASK 0x7F
#define MCTP_IC_SHIFT 7
#define MCTP_IC_MASK 0x80

/* i3c 8-bit addr */
#define I3C_STATIC_ADDR_BIC 0x40
#define I3C_STATIC_ADDR_BMC 0x20

/* i3c dev bus */
#define I3C_BUS_BMC 0

/* mctp endpoint */
#define MCTP_EID_BMC 0x01
#define MCTP_EID_SELF 0x02

uint8_t stop_flag = t1_en | t2_en | t3_en;

K_THREAD_STACK_DEFINE(dev_id_thread, 1024);
struct k_thread dev_id_thread_handler;
k_tid_t dev_id_tid;

K_THREAD_STACK_DEFINE(self_test_thread, 1024);
struct k_thread self_test_thread_handler;
k_tid_t self_test_tid;

K_THREAD_STACK_DEFINE(dev_guid_thread, 1024);
struct k_thread dev_guid_thread_handler;
k_tid_t dev_guid_tid;

K_TIMER_DEFINE(send_cmd_timer, send_cmd_to_dev, NULL);
K_WORK_DEFINE(send_cmd_work, send_cmd_to_dev_handler);

bool first_time_fail = true;
uint16_t stress_limit = 30;

typedef struct _mctp_smbus_port {
	mctp *mctp_inst;
	mctp_medium_conf conf;
	uint8_t user_idx;
} mctp_smbus_port;

typedef struct _mctp_i3c_port {
	mctp *mctp_inst;
	mctp_medium_conf conf;
	uint8_t user_idx;
} mctp_i3c_port;

/* mctp route entry struct */
typedef struct _mctp_route_entry {
	uint8_t endpoint;
	uint8_t bus; /* TODO: only consider smbus/i3c */
	uint8_t addr; /* TODO: only consider smbus/i3c */
} mctp_route_entry;

typedef struct _mctp_msg_handler {
	MCTP_MSG_TYPE type;
	mctp_fn_cb msg_handler_cb;
} mctp_msg_handler;

static mctp_i3c_port i3c_port[] = {
	{ .conf.i3c_conf.bus = I3C_BUS_BMC, .conf.i3c_conf.addr = I3C_STATIC_ADDR_BMC },
};

mctp_route_entry mctp_route_tbl[] = {
	{ MCTP_EID_BMC, I3C_BUS_BMC, I3C_STATIC_ADDR_BMC },
};

static mctp *find_mctp_by_i3c(uint8_t bus)
{
	uint8_t i;
	for (i = 0; i < ARRAY_SIZE(i3c_port); i++) {
		mctp_i3c_port *p = i3c_port + i;

		if (bus == p->conf.i3c_conf.bus) {
			return p->mctp_inst;
		}
	}

	return NULL;
}

static void set_endpoint_resp_handler(void *args, uint8_t *buf, uint16_t len)
{
	ARG_UNUSED(args);
	CHECK_NULL_ARG(buf);

	LOG_HEXDUMP_DBG(buf, len, __func__);
}

static void set_endpoint_resp_timeout(void *args)
{
	CHECK_NULL_ARG(args);

	mctp_route_entry *p = (mctp_route_entry *)args;
	LOG_DBG("Endpoint 0x%x set endpoint failed on bus %d", p->endpoint, p->bus);
}

static void set_dev_endpoint(void)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;

		/* skip BMC */
		if (p->bus == I3C_BUS_BMC && p->addr == I3C_STATIC_ADDR_BMC)
			continue;

		for (uint8_t j = 0; j < ARRAY_SIZE(i3c_port); j++) {
			if (p->bus != i3c_port[j].conf.i3c_conf.bus)
				continue;

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

			mctp_ctrl_send_msg(find_mctp_by_i3c(p->bus), &msg);
		}
	}
}

static uint8_t mctp_msg_recv(void *mctp_p, uint8_t *buf, uint32_t len, mctp_ext_params ext_params)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_p, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(buf, MCTP_ERROR);

	/* first byte is message type */
	uint8_t msg_type = (buf[0] & MCTP_MSG_TYPE_MASK) >> MCTP_MSG_TYPE_SHIFT;

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
	CHECK_NULL_ARG_WITH_RETURN(mctp_inst, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(ext_params, MCTP_ERROR);

	uint8_t rc = MCTP_ERROR;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;
		if (p->endpoint == dest_endpoint) {
			*mctp_inst = find_mctp_by_i3c(p->bus);
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
}

void send_cmd_to_dev(struct k_timer *timer)
{
	k_work_submit(&send_cmd_work);
}

bool mctp_add_sel_to_ipmi(common_addsel_msg_t *sel_msg)
{
	CHECK_NULL_ARG_WITH_RETURN(sel_msg, false);

	uint8_t system_event_record = 0x02;
	uint8_t evt_msg_version = 0x04;

	pldm_msg msg = { 0 };
	struct mctp_to_ipmi_sel_req req = { 0 };

	msg.ext_params.type = MCTP_MEDIUM_TYPE_I3C;
	msg.ext_params.i3c_ext_params.addr = I3C_BUS_BMC;

	msg.hdr.pldm_type = PLDM_TYPE_OEM;
	msg.hdr.cmd = PLDM_OEM_IPMI_BRIDGE;
	msg.hdr.rq = 1;

	msg.buf = (uint8_t *)&req;
	msg.len = sizeof(struct mctp_to_ipmi_sel_req);

	if (set_iana(req.header.iana, sizeof(req.header.iana))) {
		LOG_ERR("Set IANA fail");
		return false;
	}

	req.header.netfn_lun = (NETFN_STORAGE_REQ << 2);
	req.header.ipmi_cmd = CMD_STORAGE_ADD_SEL;
	req.req_data.event.record_type = system_event_record;
	req.req_data.event.gen_id[0] = (I3C_STATIC_ADDR_BIC << 1);
	req.req_data.event.evm_rev = evt_msg_version;

	memcpy(&req.req_data.event.sensor_type, &sel_msg->sensor_type,
	       sizeof(common_addsel_msg_t) - sizeof(uint8_t));

	uint8_t resp_len = sizeof(struct mctp_to_ipmi_sel_resp);
	uint8_t rbuf[resp_len];

	uint8_t instido;
	uint8_t msgtag;
	if (!mctp_pldm_read(find_mctp_by_i3c(I3C_BUS_BMC), &msg, rbuf, resp_len, 0, &instido, &msgtag)) {
		LOG_ERR("mctp_pldm_read fail");
		return false;
	}

	struct mctp_to_ipmi_sel_resp *resp = (struct mctp_to_ipmi_sel_resp *)rbuf;

	if ((resp->header.completion_code != MCTP_SUCCESS) ||
	    (resp->header.ipmi_comp_code != CC_SUCCESS)) {
		LOG_ERR("Check reponse completion code fail %x %x", resp->header.completion_code,
			resp->header.ipmi_comp_code);
		return false;
	}

	return true;
}

void send_unknow_request(uint8_t net_fn, uint8_t cmd)
{
	pldm_msg msg = { 0 };
	struct mctp_to_ipmi_header_req req = { 0 };
	memset(&req, 0, sizeof(struct mctp_to_ipmi_header_req));
	msg.ext_params.type = MCTP_MEDIUM_TYPE_I3C;
	msg.ext_params.i3c_ext_params.addr = I3C_BUS_BMC;

	msg.hdr.pldm_type = PLDM_TYPE_OEM;
	msg.hdr.cmd = PLDM_OEM_IPMI_BRIDGE;
	msg.hdr.rq = 1;

	msg.buf = (uint8_t *)&req;
	msg.len = sizeof(struct mctp_to_ipmi_header_req);

	if (set_iana(req.iana, sizeof(req.iana))) {
		LOG_ERR("Set IANA fail");
		return;
	}

	req.netfn_lun = (NETFN_OEM_1S_REQ << 2);
	req.ipmi_cmd = 0xFF;

	uint8_t rbuf[20];
	memset(rbuf, 0, 20);

	uint8_t instido;
	uint8_t msgtag;
	mctp_pldm_read(find_mctp_by_i3c(I3C_BUS_BMC), &msg, rbuf, 20, 0, &instido, &msgtag);
}

void mctp_get_BMC_dev_id()
{
	pldm_msg msg = { 0 };
	struct mctp_to_ipmi_header_req req = { 0 };
	uint32_t count = 0;
	uint16_t err_cnt = 0;
	while (1) {
		k_msleep(1);
		if (stop_flag & t1_en) {
			continue;
		}
		count++;
		memset(&req, 0, sizeof(struct mctp_to_ipmi_header_req));
		msg.ext_params.type = MCTP_MEDIUM_TYPE_I3C;
		msg.ext_params.i3c_ext_params.addr = I3C_BUS_BMC;

		msg.hdr.pldm_type = PLDM_TYPE_OEM;
		msg.hdr.cmd = PLDM_OEM_IPMI_BRIDGE;
		msg.hdr.rq = 1;

		msg.buf = (uint8_t *)&req;
		msg.len = sizeof(struct mctp_to_ipmi_header_req);

		if (set_iana(req.iana, sizeof(req.iana))) {
			LOG_ERR("Set IANA fail");
			continue;
		}

		req.netfn_lun = (NETFN_APP_REQ << 2);
		req.ipmi_cmd = CMD_APP_GET_DEVICE_ID;

		uint8_t rbuf[30];
		memset(rbuf, 0, sizeof(rbuf));

		uint8_t instido;
		uint8_t msgtag;
		uint16_t ret_len = mctp_pldm_read(find_mctp_by_i3c(I3C_BUS_BMC), &msg, rbuf, sizeof(rbuf), 1, &instido, &msgtag);
		if (!ret_len) {
			LOG_ERR("mctp_pldm_read fail");
			continue;
		}

		pldm_hdr tmp = {0};
		memcpy(&tmp, rbuf, sizeof(pldm_hdr));

		memmove(rbuf, rbuf + sizeof(pldm_hdr), ret_len - sizeof(pldm_hdr));
		ret_len -= sizeof(pldm_hdr);

		if (!ret_len) {
			LOG_ERR("mctp_pldm_read empty");
			continue;
		}

		if ((rbuf[0] != MCTP_SUCCESS) || (rbuf[3] != CC_SUCCESS) ||
		    (rbuf[1] != (NETFN_APP_RES << 2)) || (rbuf[2] != CMD_APP_GET_DEVICE_ID)) {
			if (first_time_fail) {
				LOG_ERR("!!!!!!!!!!!!!!!! first time error, send Netfn 0x38 Cmd 0xFF !!!!!!!!!!!!!!!!");
				send_unknow_request((NETFN_APP_REQ << 2), CMD_APP_GET_DEVICE_ID);
				first_time_fail = false;
			}
			LOG_ERR("{%d --> %d --> %d} GET_DEV_ID unexpect return value, count %d", msgtag, instido, tmp.inst_id, count);
			LOG_HEXDUMP_ERR(rbuf, ret_len, "BMC GET_DEV_ID");
			err_cnt++;
			if (err_cnt == stress_limit) {
				stop_flag |= t1_en;
				err_cnt = 0;
			}
			continue;
		}
		if ((count % 100) == 0) {
			LOG_INF("GET_DEV_ID, %d passed", count);
		}
	}
}

void mctp_get_BMC_self_test()
{
	pldm_msg msg = { 0 };
	struct mctp_to_ipmi_header_req req = { 0 };
	uint32_t count = 0;
	uint16_t err_cnt = 0;
	while (1) {
		k_msleep(1);
		if (stop_flag & t2_en) {
			continue;
		}
		count++;
		memset(&req, 0, sizeof(struct mctp_to_ipmi_header_req));
		msg.ext_params.type = MCTP_MEDIUM_TYPE_I3C;
		msg.ext_params.i3c_ext_params.addr = I3C_BUS_BMC;

		msg.hdr.pldm_type = PLDM_TYPE_OEM;
		msg.hdr.cmd = PLDM_OEM_IPMI_BRIDGE;
		msg.hdr.rq = 1;

		msg.buf = (uint8_t *)&req;
		msg.len = sizeof(struct mctp_to_ipmi_header_req);

		if (set_iana(req.iana, sizeof(req.iana))) {
			LOG_ERR("Set IANA fail");
			continue;
		}

		req.netfn_lun = (NETFN_APP_REQ << 2);
		req.ipmi_cmd = CMD_APP_GET_SELFTEST_RESULTS;

		uint8_t rbuf[30];
		memset(rbuf, 0, sizeof(rbuf));
uint8_t instido;
uint8_t msgtag;
		uint16_t ret_len = mctp_pldm_read(find_mctp_by_i3c(I3C_BUS_BMC), &msg, rbuf, sizeof(rbuf), 1, &instido, &msgtag);
		if (!ret_len) {
			LOG_ERR("mctp_pldm_read fail");
			continue;
		}

		pldm_hdr tmp = {0};
		memcpy(&tmp, rbuf, sizeof(pldm_hdr));

		memmove(rbuf, rbuf + sizeof(pldm_hdr), ret_len - sizeof(pldm_hdr));
		ret_len -= sizeof(pldm_hdr);

		if (!ret_len) {
			LOG_ERR("mctp_pldm_read empty");
			continue;
		}

		if ((rbuf[0] != MCTP_SUCCESS) || (rbuf[3] != CC_SUCCESS) ||
		    (rbuf[1] != (NETFN_APP_RES << 2)) ||
		    (rbuf[2] != CMD_APP_GET_SELFTEST_RESULTS)) {
			if (first_time_fail) {
				LOG_ERR("!!!!!!!!!!!!!!!! first time error, send Netfn 0x38 Cmd 0xFF !!!!!!!!!!!!!!!!");
				send_unknow_request((NETFN_APP_REQ << 2),
						    CMD_APP_GET_SELFTEST_RESULTS);
				first_time_fail = false;
			}
			LOG_ERR("{%d --> %d --> %d} GET_SELF_TEST unexpect return value, count %d", msgtag, instido, tmp.inst_id, count);
			LOG_HEXDUMP_ERR(rbuf, ret_len, "BMC GET_SELF_TEST");
			err_cnt++;
			if (err_cnt == stress_limit) {
				stop_flag |= t2_en;
				err_cnt = 0;
			}
			continue;
		}
		if ((count % 100) == 0) {
			LOG_INF("GET_SELF_TEST, %d passed", count);
		}
	}
}

void mctp_get_BMC_dev_guid()
{
	pldm_msg msg = { 0 };
	struct mctp_to_ipmi_header_req req = { 0 };
	uint32_t count = 0;
	uint16_t err_cnt = 0;
	while (1) {
		k_msleep(1);
		if (stop_flag & t3_en) {
			continue;
		}
		count++;
		memset(&req, 0, sizeof(struct mctp_to_ipmi_header_req));
		msg.ext_params.type = MCTP_MEDIUM_TYPE_I3C;
		msg.ext_params.i3c_ext_params.addr = I3C_BUS_BMC;

		msg.hdr.pldm_type = PLDM_TYPE_OEM;
		msg.hdr.cmd = PLDM_OEM_IPMI_BRIDGE;
		msg.hdr.rq = 1;

		msg.buf = (uint8_t *)&req;
		msg.len = sizeof(struct mctp_to_ipmi_header_req);

		if (set_iana(req.iana, sizeof(req.iana))) {
			LOG_ERR("Set IANA fail");
			continue;
		}

		req.netfn_lun = (NETFN_APP_REQ << 2);
		req.ipmi_cmd = CMD_APP_GET_SYSTEM_GUID;

		uint8_t rbuf[30];
		memset(rbuf, 0, sizeof(rbuf));
uint8_t instido;
uint8_t msgtag;
		uint16_t ret_len = mctp_pldm_read(find_mctp_by_i3c(I3C_BUS_BMC), &msg, rbuf, sizeof(rbuf), 1, &instido, &msgtag);
		if (!ret_len) {
			LOG_ERR("mctp_pldm_read fail");
			continue;
		}

		pldm_hdr tmp = {0};
		memcpy(&tmp, rbuf, sizeof(pldm_hdr));

		memmove(rbuf, rbuf + sizeof(pldm_hdr), ret_len - sizeof(pldm_hdr));
		ret_len -= sizeof(pldm_hdr);

		if (!ret_len) {
			LOG_ERR("mctp_pldm_read empty");
			continue;
		}

		if ((rbuf[0] != MCTP_SUCCESS) || (rbuf[3] != CC_SUCCESS) ||
		    (rbuf[1] != (NETFN_APP_RES << 2)) || (rbuf[2] != CMD_APP_GET_SYSTEM_GUID)) {
			if (first_time_fail) {
				LOG_ERR("!!!!!!!!!!!!!!!! first time error, send Netfn 0x38 Cmd 0xFF !!!!!!!!!!!!!!!!");
				send_unknow_request((NETFN_APP_RES << 2), CMD_APP_GET_SYSTEM_GUID);
				first_time_fail = false;
			}
			LOG_ERR("{%d --> %d --> %d} GET_DEV_GUID unexpect return value, count %d", msgtag, instido, tmp.inst_id, count);
			LOG_HEXDUMP_ERR(rbuf, ret_len, "BMC GET_DEV_GUID");
			err_cnt++;
			if (err_cnt == stress_limit) {
				stop_flag |= t3_en;
				err_cnt = 0;
			}
			continue;
		}
		if ((count % 100) == 0) {
			LOG_INF("GET_DEV_GUID, %d passed", count);
		}
	}
}

void start_i3c_stress()
{
	dev_id_tid = k_thread_create(&dev_id_thread_handler, dev_id_thread,
				     K_THREAD_STACK_SIZEOF(dev_id_thread), mctp_get_BMC_dev_id,
				     NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&dev_id_thread_handler, "dev_id_thread");

	self_test_tid =
		k_thread_create(&self_test_thread_handler, self_test_thread,
				K_THREAD_STACK_SIZEOF(self_test_thread), mctp_get_BMC_self_test,
				NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&self_test_thread_handler, "self_test_thread");

	dev_guid_tid =
		k_thread_create(&dev_guid_thread_handler, dev_guid_thread,
				K_THREAD_STACK_SIZEOF(dev_guid_thread), mctp_get_BMC_dev_guid, NULL,
				NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&dev_guid_thread_handler, "dev_guid_thread");
}

int pal_get_medium_type(uint8_t interface)
{
	int medium_type = -1;

	switch (interface) {
	case BMC_IPMB:
	case MCTP:
	case PLDM:
		medium_type = MCTP_MEDIUM_TYPE_I3C;
		break;
	default:
		medium_type = -1;
		break;
	}

	return medium_type;
}

int pal_get_target(uint8_t interface)
{
	int target = -1;

	switch (interface) {
	case BMC_IPMB:
	case MCTP:
	case PLDM:
		target = I3C_BUS_BMC;
		break;
	default:
		target = -1;
		break;
	}

	return target;
}

mctp *pal_get_mctp(uint8_t mctp_medium_type, uint8_t bus)
{
	switch (mctp_medium_type) {
	case MCTP_MEDIUM_TYPE_I3C:
		return find_mctp_by_i3c(bus);
	default:
		return NULL;
	}
}

void plat_mctp_init(void)
{
	int ret = 0;

	/* init the mctp/pldm instance */
	for (uint8_t i = 0; i < ARRAY_SIZE(i3c_port); i++) {
		mctp_i3c_port *p = i3c_port + i;

		p->mctp_inst = mctp_init();
		if (!p->mctp_inst) {
			LOG_ERR("mctp_init failed!!");
			continue;
		}

		uint8_t rc = mctp_set_medium_configure(p->mctp_inst, MCTP_MEDIUM_TYPE_I3C, p->conf);
		if (rc != MCTP_SUCCESS) {
			LOG_INF("mctp set medium configure failed");
		}

		mctp_reg_endpoint_resolve_func(p->mctp_inst, get_mctp_route_info);

		mctp_reg_msg_rx_func(p->mctp_inst, mctp_msg_recv);

		ret = mctp_start(p->mctp_inst);
	}
}

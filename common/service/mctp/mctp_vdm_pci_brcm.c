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

#include "mctp.h"
#include "mctp_vdm_pci_brcm.h"
#include "libutil.h"
#include <logging/log.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/printk.h>
#include <zephyr.h>

LOG_MODULE_DECLARE(mctp);

#define DEFAULT_WAIT_TO_MS 3000
#define RESP_MSG_PROC_MUTEX_WAIT_TO_MS 1000
#define TO_CHK_INTERVAL_MS 1000

#define VDM_PCI_READ_EVENT_SUCCESS BIT(0) // Get success cc from pex
#define VDM_PCI_READ_EVENT_FAILED BIT(1) // Get bad cc from pex
#define VDM_PCI_READ_EVENT_TIMEOUT BIT(2) // Can't get response in time

static K_MUTEX_DEFINE(wait_recv_resp_mutex);
static sys_slist_t wait_recv_resp_list = SYS_SLIST_STATIC_INIT(&wait_recv_resp_list);

typedef struct _wait_msg {
	sys_snode_t node;
	mctp *mctp_inst;
	int64_t exp_to_ms;
	mctp_vdm_pci_brcm_msg msg;
} wait_msg;

typedef struct _recv_resp_arg {
	struct k_msgq *msgq;
	uint8_t *rbuf;
	uint16_t rbuf_len;
	uint16_t return_len;
} recv_resp_arg;

static void mctp_vend_pci_resp_handler(void *args, uint8_t *rbuf, uint16_t rlen)
{
	CHECK_NULL_ARG(args);
	CHECK_NULL_ARG(rbuf);

	if (!rlen)
		return;

	recv_resp_arg *recv_arg = (recv_resp_arg *)args;
	if (rlen > recv_arg->rbuf_len) {
		LOG_WRN("[%s] response length(%d) is greater than buffer length(%d)!", __func__,
			rlen, recv_arg->rbuf_len);
		recv_arg->return_len = recv_arg->rbuf_len;
	} else {
		recv_arg->return_len = rlen;
	}
	memcpy(recv_arg->rbuf, rbuf, recv_arg->return_len);
	uint8_t status = VDM_PCI_READ_EVENT_SUCCESS;
	k_msgq_put(recv_arg->msgq, &status, K_NO_WAIT);
}

static void mctp_vend_pci_resp_timeout(void *args)
{
	CHECK_NULL_ARG(args);

	struct k_msgq *msgq = (struct k_msgq *)args;
	uint8_t status = VDM_PCI_READ_EVENT_TIMEOUT;
	k_msgq_put(msgq, &status, K_NO_WAIT);
}

static uint8_t mctp_vend_pci_msg_timeout_check(sys_slist_t *list, struct k_mutex *mutex)
{
	CHECK_NULL_ARG_WITH_RETURN(list, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(mutex, MCTP_ERROR);

	if (k_mutex_lock(mutex, K_MSEC(RESP_MSG_PROC_MUTEX_WAIT_TO_MS))) {
		LOG_WRN("pldm mutex is locked over %d ms!!", RESP_MSG_PROC_MUTEX_WAIT_TO_MS);
		return MCTP_ERROR;
	}

	sys_snode_t *node;
	sys_snode_t *s_node;
	sys_snode_t *pre_node = NULL;
	int64_t cur_uptime = k_uptime_get();

	SYS_SLIST_FOR_EACH_NODE_SAFE (list, node, s_node) {
		wait_msg *p = (wait_msg *)node;

		if ((p->exp_to_ms <= cur_uptime)) {
			LOG_ERR("mctp vendor pci msg timeout, remove cmd %x tag %x",
				p->msg.req_hdr.cmd, p->msg.req_hdr.app_msg_tag);
			sys_slist_remove(list, pre_node, node);

			if (p->msg.timeout_cb_fn)
				p->msg.timeout_cb_fn(p->msg.timeout_cb_fn_args);

			free(p);
		} else {
			pre_node = node;
		}
	}

	k_mutex_unlock(mutex);
	return MCTP_SUCCESS;
}

static void mctp_vend_pci_msg_timeout_monitor(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	while (1) {
		k_msleep(TO_CHK_INTERVAL_MS);

		mctp_vend_pci_msg_timeout_check(&wait_recv_resp_list, &wait_recv_resp_mutex);
	}
}

static uint8_t mctp_vdm_pci_cmd_resp_process(mctp *mctp_inst, uint8_t *buf, uint32_t len,
					     mctp_ext_params ext_params)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_inst, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(buf, MCTP_ERROR);

	if (!len)
		return MCTP_ERROR;

	if (k_mutex_lock(&wait_recv_resp_mutex, K_MSEC(RESP_MSG_PROC_MUTEX_WAIT_TO_MS))) {
		LOG_WRN("mutex is locked over %d ms!", RESP_MSG_PROC_MUTEX_WAIT_TO_MS);
		return MCTP_ERROR;
	}

	mctp_vend_pci_rsp_first_hdr *hdr = (mctp_vend_pci_rsp_first_hdr *)buf;
	sys_snode_t *node;
	sys_snode_t *s_node;
	sys_snode_t *pre_node = NULL;
	sys_snode_t *found_node = NULL;

	SYS_SLIST_FOR_EACH_NODE_SAFE (&wait_recv_resp_list, node, s_node) {
		wait_msg *p = (wait_msg *)node;
		/* found the proper handler */
		if ((p->msg.req_hdr.app_msg_tag == hdr->app_msg_tag) &&
		    (p->mctp_inst == mctp_inst)) {
			found_node = node;
			sys_slist_remove(&wait_recv_resp_list, pre_node, node);
			break;
		} else {
			pre_node = node;
		}
	}
	k_mutex_unlock(&wait_recv_resp_mutex);

	if (found_node) {
		/* invoke resp handler */
		wait_msg *p = (wait_msg *)found_node;
		if (p->msg.recv_resp_cb_fn) {
			if (p->msg.rsp_hdr.cc != MCTP_7E_CC_SUCCESS) {
				p->msg.recv_resp_cb_fn(
					p->msg.recv_resp_cb_args, buf + sizeof(p->msg.rsp_hdr),
					len - sizeof(p->msg.rsp_hdr)); /* remove mctp ctrl header for handler */
			} else {
				LOG_WRN("Get non-success complition code %d from 7E command %xh",
					p->msg.rsp_hdr.cc, p->msg.req_hdr.cmd);
				recv_resp_arg *recv_arg = (recv_resp_arg *)p->msg.recv_resp_cb_args;
				uint8_t status = VDM_PCI_READ_EVENT_FAILED;
				k_msgq_put(recv_arg->msgq, &status, K_NO_WAIT);
			}
		}

		free(p);
	}

	return MCTP_SUCCESS;
}

uint8_t mctp_vdm_pci_brcm_cmd_handler(void *mctp_p, uint8_t *buf, uint32_t len,
				      mctp_ext_params ext_params)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_p, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(buf, MCTP_ERROR);

	if (!len)
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;

	/* Assume that only catch response-messages from pex */
	if (1)
		return mctp_vdm_pci_cmd_resp_process(mctp_inst, buf, len, ext_params);

	return MCTP_SUCCESS;
}

static uint8_t find_rsp_len_by_cmd(SM_API_COMMANDS cmd, uint16_t req_len, uint16_t *rsp_len)
{
	CHECK_NULL_ARG_WITH_RETURN(rsp_len, MCTP_ERROR);

	switch (cmd) {
	case SM_API_CMD_FW_REV:
		if (req_len != sizeof(struct _get_fw_rev_req))
			goto error;
		*rsp_len = sizeof(struct _get_fw_rev_resp);
		break;

	case SM_API_CMD_GET_SW_ATTR:
		if (req_len != sizeof(struct _get_sw_attr_req))
			goto error;
		*rsp_len = sizeof(struct _get_sw_attr_resp);
		break;

	case SM_API_CMD_GET_SW_MFG_INFO:
		if (req_len != sizeof(struct _sm_sw_mfg_info_req))
			goto error;
		*rsp_len = sizeof(struct _sm_sw_mfg_info_resp);
		break;

	case SM_API_CMD_GET_SW_TEMP:
		if (req_len != sizeof(struct _get_sw_temp_req))
			goto error;
		*rsp_len = sizeof(struct _get_sw_temp_resp);
		break;

	default:
		LOG_ERR("Given unsupported request command");
		return MCTP_ERROR;
	}

	return MCTP_SUCCESS;

error:
	LOG_ERR("Given with invalid request length");
	return MCTP_ERROR;
}

static uint8_t mctp_vdm_pci_send_msg(void *mctp_p, mctp_vdm_pci_brcm_msg *msg, uint8_t *buff,
				     uint16_t buff_len)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_p, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(msg, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(buff, MCTP_ERROR);

	if (!buff_len)
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;

	uint8_t rc = mctp_send_msg(mctp_inst, buff, buff_len, msg->ext_params);
	if (rc == MCTP_ERROR) {
		LOG_WRN("mctp_send_msg error!!");
		return MCTP_ERROR;
	}

	wait_msg *p = (wait_msg *)malloc(sizeof(*p));
	if (!p) {
		LOG_WRN("wait_msg alloc failed!");
		return MCTP_ERROR;
	}

	memset(p, 0, sizeof(*p));
	p->mctp_inst = mctp_inst;
	p->msg = *msg;
	p->exp_to_ms = k_uptime_get() + (msg->timeout_ms ? msg->timeout_ms : DEFAULT_WAIT_TO_MS);

	k_mutex_lock(&wait_recv_resp_mutex, K_FOREVER);
	sys_slist_append(&wait_recv_resp_list, &p->node);
	k_mutex_unlock(&wait_recv_resp_mutex);

	return MCTP_SUCCESS;
}

uint16_t mctp_vdm_pci_brcm_read(void *mctp_p, mctp_vdm_pci_brcm_msg *msg, uint8_t *rbuf,
				uint16_t rbuf_len)
{
	CHECK_NULL_ARG_WITH_RETURN(mctp_p, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(msg, MCTP_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(msg->cmd_data, MCTP_ERROR);

	if (msg->req_hdr.cmd >= SM_API_CMD_MAX) {
		LOG_ERR("Unsupported Broadcom MCTP SM command");
		return 0;
	}

	if (find_rsp_len_by_cmd(msg->req_hdr.cmd, msg->cmd_data_len, &msg->req_hdr.resp_len))
		return 0;

	memset(&msg->req_sup_hdr, 0, sizeof(msg->req_sup_hdr));
	memset(&msg->rsp_hdr, 0, sizeof(msg->req_sup_hdr));
	memset(&msg->rsp_sup_hdr, 0, sizeof(msg->req_sup_hdr));

	msg->req_hdr.msg_type = MCTP_MSG_TYPE_VEN_DEF_PCI & 0x7F;
	msg->req_hdr.ic = 0;
	msg->req_hdr.vendor_id_hi = (BRDCM_ID >> 8) & 0xFF;
	msg->req_hdr.vendor_id_low = BRDCM_ID & 0xFF;
	msg->req_hdr.payload_id = MCTP_PAYLOADID_SL_COMMAND;
	msg->ext_params.tag_owner = 1;

	recv_resp_arg rcv_p;
	uint8_t event_msgq_buffer[1];
	struct k_msgq event_msgq;

	k_msgq_init(&event_msgq, event_msgq_buffer, sizeof(uint8_t), 1);

	static uint8_t inst_id;
	uint16_t len = 0;
	uint16_t payload_msg_len = 0;
	uint16_t remain_payload_msg_len = msg->cmd_data_len;
	uint8_t *payload_data_p = msg->cmd_data;
	uint8_t buf[MAX_MCTP_VEND_PCI_PAYLOAD_LEN];

	uint16_t total_pkt = 1;
	total_pkt += (msg->cmd_data_len / MAX_MCTP_VEND_PCI_PAYLOAD_LEN);
	if (total_pkt != 1 && msg->cmd_data_len % MAX_MCTP_VEND_PCI_PAYLOAD_LEN)
		total_pkt++;

	for (int cur_idx = 0; cur_idx < total_pkt; cur_idx++) {
		payload_msg_len = remain_payload_msg_len;
		if (remain_payload_msg_len > MAX_MCTP_VEND_PCI_PAYLOAD_LEN)
			payload_msg_len = MAX_MCTP_VEND_PCI_PAYLOAD_LEN;

		if (cur_idx == 0) {
			/* First packet */
			msg->req_hdr.payload_len = sizeof(pmg_mcpu_msg_hdr) + payload_msg_len;
			msg->req_hdr.pkt_seq_cnt = cur_idx;
			msg->req_hdr.app_msg_tag = (++inst_id) & MCTP_VEND_PCI_INST_ID_MASK;

			pmg_mcpu_msg_hdr payload_hdr;
			payload_hdr.msg_len =
				MCTP_BYTES_TO_DWORDS(msg->cmd_data_len + sizeof(pmg_mcpu_msg_hdr));
			payload_hdr.hdr_ver = PMG_MCPU_MSG_HEADER_VERSION;
			payload_hdr.src_domain = 0;
			payload_hdr.src_id = 0; //Thist must be MCTP eid
			payload_hdr.src_addr_type = PMG_MCPU_MSG_ADDRESS_TYPE_MCTP;
			payload_hdr.src_type_specific = 0;

			payload_hdr.msg_type = PMG_MCPU_MSG_TYPE_FABRIC_EVENT;
			payload_hdr.msg_flags = 0;
			payload_hdr.msg_type_specific = msg->req_hdr.cmd;
			payload_hdr.pkt_num = msg->req_hdr.pkt_seq_cnt;
			payload_hdr.req_tag = msg->req_hdr.app_msg_tag;
			payload_hdr.dest_domain = 0;
			payload_hdr.dest_id = 0; //Thist must be MCTP eid
			payload_hdr.dest_addr_type = PMG_MCPU_MSG_ADDRESS_TYPE_PCIE;
			payload_hdr.dest_type_specific = 0;

			memcpy(buf, &msg->req_hdr, sizeof(mctp_vend_pci_req_first_hdr));
			memcpy(buf + sizeof(mctp_vend_pci_req_first_hdr), &payload_hdr,
			       sizeof(pmg_mcpu_msg_hdr));
			memcpy(buf + sizeof(mctp_vend_pci_req_first_hdr) + sizeof(pmg_mcpu_msg_hdr),
			       payload_data_p, payload_msg_len);
			len = sizeof(mctp_vend_pci_req_first_hdr) + sizeof(pmg_mcpu_msg_hdr) +
			      payload_msg_len;
		} else {
			/* Suppliment packet */
			mctp_vend_pci_req_supp_hdr supp_hdr;
			supp_hdr.vendor_id_hi = msg->req_hdr.vendor_id_hi;
			supp_hdr.vendor_id_low = msg->req_hdr.vendor_id_low;
			supp_hdr.payload_id = msg->req_hdr.payload_id;
			supp_hdr.rsv = 0x00;
			supp_hdr.pkt_seq_cnt = cur_idx;
			supp_hdr.app_msg_tag = (++inst_id) & MCTP_VEND_PCI_INST_ID_MASK;
			supp_hdr.cmd = msg->req_hdr.cmd;

			memcpy(buf, &supp_hdr, sizeof(mctp_vend_pci_req_supp_hdr));
			memcpy(buf + sizeof(mctp_vend_pci_req_supp_hdr), payload_data_p,
			       payload_msg_len);
			len = sizeof(mctp_vend_pci_req_supp_hdr) + payload_msg_len;
		}

		LOG_DBG("REQ tag[%d] pkt[%d/%d]", inst_id & MCTP_VEND_PCI_INST_ID_MASK, cur_idx + 1,
			total_pkt);
		LOG_HEXDUMP_DBG(buf, len, "");
#if 1
		rcv_p.msgq = &event_msgq;
		rcv_p.rbuf_len = rbuf_len;
		rcv_p.rbuf = rbuf;
		rcv_p.return_len = 0;
		msg->recv_resp_cb_fn = mctp_vend_pci_resp_handler;
		msg->recv_resp_cb_args = &rcv_p;
		msg->timeout_cb_fn = mctp_vend_pci_resp_timeout;
		msg->timeout_ms = MCTP_VEND_PCI_MSG_TIMEOUT_MS;

		int retry = 0;
		for (retry = 0; retry < MCTP_VEND_PCI_MSG_RETRY; retry++) {
			/* Send out message */
			if (mctp_vdm_pci_send_msg(mctp_p, msg, buf, len) != MCTP_SUCCESS) {
				LOG_WRN("[%s] send msg failed!", __func__);
				continue;
			}

			/* Wait for response */
			uint8_t event;
			if (k_msgq_get(&event_msgq, &event,
				       K_MSEC(MCTP_VEND_PCI_MSG_TIMEOUT_MS + 1000))) {
				LOG_WRN("[%s] Failed to get status from msgq!", __func__);
				continue;
			}
			if (event == VDM_PCI_READ_EVENT_SUCCESS)
				break;
			else if (event == VDM_PCI_READ_EVENT_FAILED) {
				LOG_WRN("[%s] Failed to get success CC from pex!", __func__);
				return 0;
			}
		}

		if (retry == MCTP_VEND_PCI_MSG_RETRY) {
			LOG_ERR("Message send&receive retry over limit at packet #%d/%d\n",
				cur_idx + 1, total_pkt);
			return 0;
		}

		LOG_DBG("RSP seq[%d] pkt[%d/%d]:", inst_id & MCTP_VEND_PCI_INST_ID_MASK,
			cur_idx + 1, total_pkt);
		LOG_HEXDUMP_DBG(rbuf, rcv_p.return_len, "");

#endif

		payload_data_p += payload_msg_len;
		remain_payload_msg_len -= payload_msg_len;
		if (cur_idx != (total_pkt - 1)) {
			payload_data_p += payload_msg_len;
		}
	}

	return rcv_p.return_len;
}

K_THREAD_DEFINE(vend_pci_tid, 1024, mctp_vend_pci_msg_timeout_monitor, NULL, NULL, NULL, 7, 0, 0);

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
#include "mctp_vend_pci.h"
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

static K_MUTEX_DEFINE(wait_recv_resp_mutex);
static sys_slist_t wait_recv_resp_list = SYS_SLIST_STATIC_INIT(&wait_recv_resp_list);

typedef struct _wait_msg {
	sys_snode_t node;
	mctp *mctp_inst;
	int64_t exp_to_ms;
	mctp_vend_pci_msg msg;
} wait_msg;

static uint8_t mctp_vend_pci_msg_timeout_check(sys_slist_t *list, struct k_mutex *mutex)
{
	if (!list || !mutex)
		return MCTP_ERROR;

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
			printk("mctp vendor pci msg timeout!!\n");
			printk("cmd %x, app msg tag %x\n", p->msg.hdr.cmd, p->msg.hdr.app_msg_tag);
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

uint8_t mctp_vend_pci_send_msg(void *mctp_p, mctp_vend_pci_msg *msg)
{
	if (!mctp_p || !msg || !msg->cmd_data)
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;

    static uint8_t inst_id;
    msg->hdr.app_msg_tag = (inst_id++) & MCTP_VEND_PCI_INST_ID_MASK;
    msg->hdr.msg_type = MCTP_MSG_TYPE_VEN_DEF_PCI;
    msg->ext_params.tag_owner = 1;

	uint16_t len = sizeof(msg->hdr) + msg->cmd_data_len;
	uint8_t buf[len];

	memcpy(buf, &msg->hdr, sizeof(msg->hdr));
	memcpy(buf + sizeof(msg->hdr), msg->cmd_data, msg->cmd_data_len);

	LOG_HEXDUMP_DBG(buf, len, __func__);

	uint8_t rc = mctp_send_msg(mctp_inst, buf, len, msg->ext_params);
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
    p->exp_to_ms =
        k_uptime_get() + (msg->timeout_ms ? msg->timeout_ms : DEFAULT_WAIT_TO_MS);

    k_mutex_lock(&wait_recv_resp_mutex, K_FOREVER);
    sys_slist_append(&wait_recv_resp_list, &p->node);
    k_mutex_unlock(&wait_recv_resp_mutex);

	return MCTP_SUCCESS;
}

K_THREAD_DEFINE(vend_pci_tid, 1024, mctp_vend_pci_msg_timeout_monitor, NULL, NULL, NULL, 7, 0, 0);

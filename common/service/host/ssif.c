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
//#ifdef ENABLE_SSIF

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <device.h>
#include <stdlib.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include <sys/crc.h>
#include "ipmi.h"
#include "ssif.h"
#include "pldm.h"
#include "libutil.h"
#include "plat_i2c.h"

LOG_MODULE_REGISTER(ssif);

ssif_dev *ssif;
static uint8_t ssif_channel_cnt = 0;

ssif_status_t pre_status = SSIF_STATUS_IDLE;
ssif_status_t cur_status = SSIF_STATUS_IDLE;
ssif_err_status_t cur_err_status = SSIF_STATUS_NO_ERR;
ssif_action_t cur_action = SSIF_DO_NOTHING;

static uint16_t cur_rd_blck = 0; // for multi-read middle/end

static uint8_t is_pec_exist = 1; // should only assign 0 or 1

ipmi_msg_cfg current_ipmi_msg; // for ipmi request data

bool ssif_set_data(uint8_t channel, ipmi_msg_cfg *msg_cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(msg_cfg, false);

	if (channel >= ssif_channel_cnt) {
		LOG_WRN("Invalid SSIF channel %d", channel);
		return false;
	}

	CHECK_MUTEX_INIT_WITH_RETURN(&ssif[channel].rd_buff_mutex, false);

	if (k_mutex_lock(&ssif[channel].rd_buff_mutex, K_MSEC(5000))) {
		LOG_ERR("ssif %d mutex lock failed", channel);
		return false;
	}

	uint8_t ssif_buff[SSIF_BUFF_SIZE];
	ssif_buff[0] = (msg_cfg->buffer.netfn + 1) << 2; // netfn + lun(0)
	ssif_buff[1] = msg_cfg->buffer.cmd;
	ssif_buff[2] = msg_cfg->buffer.completion_code;
	if (msg_cfg->buffer.data_len) {
		if (msg_cfg->buffer.data_len <= (SSIF_BUFF_SIZE - 3))
			memcpy(&ssif_buff[3], msg_cfg->buffer.data,
					msg_cfg->buffer.data_len);
		else
			memcpy(&ssif_buff[3], msg_cfg->buffer.data,
					(SSIF_BUFF_SIZE - 3));
	}

	ssif[channel].rd_len = msg_cfg->buffer.data_len + 3;
	memcpy(&ssif[channel].rd_buff, ssif_buff, msg_cfg->buffer.data_len + 3);

	LOG_DBG("ssif from ipmi netfn %x, cmd %x, data length %d, cc %x", ssif[channel].rd_buff[0],
		ssif[channel].rd_buff[1], msg_cfg->buffer.data_len, ssif[channel].rd_buff[2]);

	k_sem_give(&ssif[channel].rd_buff_sem);

	if (k_mutex_unlock(&ssif[channel].rd_buff_mutex))
		LOG_ERR("ssif %d mutex unlock failed", channel);

	return true;
}

static ssif_dev *ssif_inst_get_by_bus(uint8_t bus)
{
	for (int i=0; i<ssif_channel_cnt; i++) {
		if (ssif[i].i2c_bus == bus) {
			return &ssif[i];
		}
	}

	return NULL;
}

static void ssif_status_change(ssif_status_t status)
{
	pre_status = cur_status;
	cur_status = status;
}

static void ssif_reset(ssif_dev *ssif_inst)
{
	CHECK_NULL_ARG(ssif_inst);

	ssif_status_change(SSIF_STATUS_IDLE);
	cur_action = SSIF_DO_NOTHING;
	memset(&current_ipmi_msg, 0, sizeof(current_ipmi_msg));
}

/**
 * @brief SSIF pec check function
 *
 * Check CRC 8 from received message with pec.
 *
 * @param addr Target address(BIC itself)
 * @param data Data to be calculted, should include pec at last byte
 *             and exclude target address
 * @param len Length of the data in bytes
 *
 * @return true, if match 
 */
static bool ssif_pec_check(uint8_t addr, uint8_t *data, uint16_t len)
{
	CHECK_NULL_ARG_WITH_RETURN(data, false);

	if (len < 2) {
		LOG_ERR("SSIF request data should at last contain smb_cmd and length");
		return false;
	}

	/* skip if pec not exist */
	if ( (len - data[1] - 2) == 0 ) {
		is_pec_exist = 0;
		return true;
	} else {
		is_pec_exist = 1;
	}

	uint8_t pec_buf[len];
	pec_buf[0] = (addr << 1); // wr:0
	memcpy(pec_buf + 1, data, len - 1);

	/** pec byte use 7-degree polynomial with 0 init value and false reverse **/
	uint8_t cal_pec = crc8(pec_buf, sizeof(pec_buf), 0x07, 0x00, false);

	if (data[len-1] != cal_pec)
		return false;

	return true;
}

/**
 * @brief SSIF pec get function
 *
 * Compute CRC 8 of send out message by passing address, smbus command, data.
 *
 * @param addr Target address(BIC itself)
 * @param smb_cmd SMBus command previously received
 * @param data Data to be calculted, should exclude target address
 * @param len Length of the data in bytes
 *
 * @return pec(crc8)
 */
static uint8_t ssif_pec_get(uint8_t addr, uint8_t smb_cmd, uint8_t *data, uint16_t len)
{
	CHECK_NULL_ARG_WITH_RETURN(data, 0);

	uint8_t pec_buf[len + 3]; // addr + smb_cmd + addr + data
	pec_buf[0] = (addr << 1); // wr:0
	pec_buf[1] = smb_cmd;
	pec_buf[2] = (addr << 1) + 1; // rd:1
	memcpy(pec_buf + 3, data, len);

	/** pec byte use 7-degree polynomial with 0 init value and false reverse **/
	return crc8(pec_buf, sizeof(pec_buf), 0x07, 0x00, false);
}

static bool ssif_status_check(uint8_t smb_cmd)
{
	bool ret = false;

	switch (smb_cmd) {
	case SSIF_WR_SINGLE:
	case SSIF_WR_MULTI_START:
	case SSIF_RD_SINGLE:
		if (cur_status != SSIF_STATUS_IDLE)
			goto exit;
		if (smb_cmd == SSIF_WR_SINGLE)
			ssif_status_change(SSIF_STATUS_WR_SINGLE);
		else if (smb_cmd == SSIF_WR_MULTI_START)
			ssif_status_change(SSIF_STATUS_WR_MULTI_START);
		else
			ssif_status_change(SSIF_STATUS_RD_SINGLE);
		break;

	case SSIF_WR_MULTI_MIDDLE:
	case SSIF_WR_MULTI_END:
		if (cur_status != SSIF_STATUS_WR_MULTI_START && cur_status != SSIF_STATUS_WR_MULTI_MIDDLE)
			goto exit;
		if (smb_cmd == SSIF_WR_MULTI_MIDDLE)
			ssif_status_change(SSIF_STATUS_WR_MULTI_MIDDLE);
		else
			ssif_status_change(SSIF_STATUS_WR_MULTI_END);
		break;

	case SSIF_RD_MULTI_MIDDLE:
		if (cur_status != SSIF_STATUS_RD_MULTI_START && cur_status != SSIF_STATUS_RD_MULTI_MIDDLE) {
			goto exit;
		}
		ssif_status_change(SSIF_STATUS_RD_MULTI_MIDDLE);
		break;

	case SSIF_RD_MULTI_RETRY:
		if (cur_status != SSIF_STATUS_RD_MULTI_MIDDLE) {
			goto exit;
		}
		ssif_status_change(SSIF_STATUS_RD_MULTI_MIDDLE);
		break;
	
	default:
		LOG_ERR("Invalid SMB command %d received in first package", smb_cmd);
		cur_err_status = SSIF_STATUS_INVALID_CMD;
		return false;
	}

	ret = true;
exit:
	if (ret == false)
		cur_err_status = SSIF_STATUS_INVALID_CMD_IN_CUR_STATUS;

	return ret;
}

static bool ssif_do_action(ssif_action_t action, uint8_t smb_cmd, ssif_dev *ssif_inst)
{
	CHECK_NULL_ARG_WITH_RETURN(ssif_inst, false);

	static uint16_t remain_data_len = 0;

	switch (action) {
	case SSIF_DO_NOTHING:
		break;

	case SSIF_SEND_IPMI:
		if (pal_request_msg_to_BIC_from_KCS(current_ipmi_msg.buffer.netfn, current_ipmi_msg.buffer.cmd)) {
			while (k_msgq_put(&ipmi_msgq, &current_ipmi_msg, K_NO_WAIT) != 0) {
				k_msgq_purge(&ipmi_msgq);
				LOG_WRN("SSIF retrying put ipmi msgq");
				return false;
			}
		} else {
			if (pal_immediate_respond_from_KCS(current_ipmi_msg.buffer.netfn, current_ipmi_msg.buffer.cmd)) {
				current_ipmi_msg.buffer.data_len = 0;
				current_ipmi_msg.buffer.completion_code = CC_SUCCESS;

				if (((current_ipmi_msg.buffer.netfn == NETFN_STORAGE_REQ) &&
						(current_ipmi_msg.buffer.cmd == CMD_STORAGE_ADD_SEL))) {
					current_ipmi_msg.buffer.data_len += 2;
					current_ipmi_msg.buffer.data[0] = 0x00;
					current_ipmi_msg.buffer.data[1] = 0x00;
				}

				if (ssif_set_data(current_ipmi_msg.buffer.InF_source - HOST_SSIF_1, &current_ipmi_msg) == false) {
					LOG_ERR("Failed to write ssif response data");
				}
			}
/*
			if ((current_ipmi_msg.buffer.netfn == NETFN_APP_REQ) &&
			    (current_ipmi_msg.buffer.cmd == CMD_APP_SET_SYS_INFO_PARAMS) &&
			    (current_ipmi_msg.buffer.data[0] == CMD_SYS_INFO_FW_VERSION)) {
				int ret = pal_record_bios_fw_version(ibuf, rc);
				if (ret == -1) {
					LOG_ERR("Record bios fw version fail");
				}
			}
*/
			/* TODO: Send command to BMC */
			LOG_WRN("Commands to BMC not ready yet, skip it");
		}

		int retry = 0;
		while (retry < 3) {
			if (k_sem_take(&ssif_inst->rd_buff_sem, K_MSEC(500))) {
				retry++;
				continue;
			}
			break;
		}

		if (retry == 3) {
			LOG_ERR("Get ipmi message retry over limit");
			return false;
		}

		LOG_DBG("ipmi rsp netfn 0x%x, cmd 0x%x, cc 0x%x, data length %d:", ssif_inst->rd_buff[0], ssif_inst->rd_buff[1], ssif_inst->rd_buff[2], ssif_inst->rd_len-3);
		LOG_HEXDUMP_DBG(ssif_inst->rd_buff + 3, ssif_inst->rd_len - 3, "");

		/* TODO: Should let HOST to know data ready */
		break;

	case SSIF_COLLECT_DATA: {
		uint16_t rd_buff_len = 0;
		uint8_t rd_buff[SSIF_BUFF_SIZE];
		memset(rd_buff, 0, ARRAY_SIZE(rd_buff));

		if (ssif_inst->rd_len) {
			if (cur_status == SSIF_STATUS_RD_SINGLE || cur_status == SSIF_STATUS_RD_MULTI_START) {
				remain_data_len = ssif_inst->rd_len;
				cur_rd_blck = 0;
				if (remain_data_len > SSIF_MAX_IPMI_DATA_SIZE) {
					rd_buff[1] = (SSIF_MULTI_RD_KEY >> 8) & 0xFF;
					rd_buff[2] = SSIF_MULTI_RD_KEY & 0xFF;
					memcpy(rd_buff + 3, ssif_inst->rd_buff, SSIF_MAX_IPMI_DATA_SIZE - 2);
					rd_buff_len = SSIF_MAX_IPMI_DATA_SIZE;
					ssif_status_change(SSIF_STATUS_RD_MULTI_MIDDLE);
					remain_data_len -= (SSIF_MAX_IPMI_DATA_SIZE - 2);
				} else {
					memcpy(rd_buff + 1, ssif_inst->rd_buff, ssif_inst->rd_len);
					rd_buff_len = ssif_inst->rd_len;
					ssif_status_change(SSIF_STATUS_RD_SINGLE);
					remain_data_len = 0;
				}
			} else if (cur_status == SSIF_STATUS_RD_MULTI_MIDDLE || cur_status == SSIF_STATUS_RD_MULTI_END) {
				if (remain_data_len > (SSIF_MAX_IPMI_DATA_SIZE - 1)) {
					rd_buff[1] = cur_rd_blck;
					rd_buff_len = SSIF_MAX_IPMI_DATA_SIZE;
					ssif_status_change(SSIF_STATUS_RD_MULTI_MIDDLE);
				} else {
					rd_buff[1] = 0xFF;
					rd_buff_len = remain_data_len + 1;
					ssif_status_change(SSIF_STATUS_RD_MULTI_END);
				}
				memcpy(rd_buff + 2, ssif_inst->rd_buff + (ssif_inst->rd_len - remain_data_len), rd_buff_len - 1);
				remain_data_len -= (rd_buff_len - 1);
				cur_rd_blck++;
			} else {
				LOG_WRN("Current status not supposed to do this action");
				return false;
			}

			rd_buff[0] = rd_buff_len;
			rd_buff[rd_buff_len + 1] = ssif_pec_get(ssif_inst->addr, smb_cmd, rd_buff, rd_buff_len + 1);

			LOG_HEXDUMP_INF(rd_buff, rd_buff_len + 2, "host SSIF write RESP data:");

			uint8_t rc = i2c_target_write(ssif_inst->i2c_bus, rd_buff, rd_buff_len + 2);
			if (rc) {
				LOG_ERR("i2c_target_write fail, ret %d\n", rc);
				return false;
			}

		} else {
			LOG_WRN("Data not ready");
			return false;
		}
		break;
	}

	default:
		LOG_WRN("Invalid action %d", action);
		return false;
	}

	return true;
}

ssif_err_status_t ssif_get_error_status()
{
	return cur_err_status;
}

void ssif_collect_data(uint8_t smb_cmd, uint8_t bus)
{
	if (smb_cmd != SSIF_RD_SINGLE && smb_cmd != SSIF_RD_MULTI_MIDDLE && smb_cmd != SSIF_RD_MULTI_RETRY) {
		return;
	}

	if (ssif_status_check(smb_cmd) == false) {
		return;
	}

	switch (cur_status) {
	case SSIF_STATUS_RD_SINGLE:
	case SSIF_STATUS_RD_MULTI_START:
	case SSIF_STATUS_RD_MULTI_MIDDLE: {
		ssif_dev *ssif_inst = ssif_inst_get_by_bus(bus);
		if (!ssif_inst) {
			LOG_WRN("Failed to get ssif inst by i2c bus %d", bus);
			return;
		}

		if (ssif_do_action(SSIF_COLLECT_DATA, smb_cmd, ssif_inst) == false) {
			cur_err_status = SSIF_STATUS_UNKNOWN_ERR;
			return;
		}
		break;
	}

	default:
		break;
	}

	return;
}

static void ssif_read_task(void *arvg0, void *arvg1, void *arvg2)
{
	int rc = 0;
	uint8_t cur_smb_cmd = 0;

	ARG_UNUSED(arvg1);
	ARG_UNUSED(arvg2);
	ssif_dev *ssif_inst = (ssif_dev *)arvg0;
	if (!ssif_inst) {
		LOG_ERR("Failed to get the ssif instance");
		return;
	}

	memset(&current_ipmi_msg, 0, sizeof(current_ipmi_msg));

	while (1) {
		uint8_t rdata[SSIF_BUFF_SIZE] = { 0 };
		uint16_t rlen = 0;
		rc = i2c_target_read(ssif_inst->i2c_bus, rdata, ARRAY_SIZE(rdata), &rlen);
		if (rc) {
			LOG_ERR("i2c_target_read fail, ret %d\n", rc);
			cur_err_status = SSIF_STATUS_UNKNOWN_ERR;
			goto reset;
		}

		if (rlen == 0) {
			LOG_ERR("Invalid length of SSIF message received");
			cur_err_status = SSIF_STATUS_INVALID_LEN;
			goto reset;
		}

		/* Should ignore READ command, cause already been handle in lower level */
		if (cur_status & 0xF0) {
			if (cur_status == SSIF_STATUS_RD_MULTI_END || cur_status == SSIF_STATUS_RD_SINGLE) {
				cur_err_status = SSIF_STATUS_NO_ERR;
				goto reset;
			} else {
				continue;
			}
		}

		LOG_HEXDUMP_INF(rdata, rlen, "host SSIF read REQ data:");

		cur_smb_cmd = rdata[0];

		if (ssif_status_check(cur_smb_cmd) == false) {
			LOG_ERR("ssif status check failed");
			goto reset;
		}

		if (ssif_pec_check(ssif_inst->addr, rdata, rlen) == false) {
			LOG_ERR("ssif pec check failed");
			cur_err_status = SSIF_STATUS_INVALID_PEC;
			goto reset;
		}

		/* This part should only handle WRITE command */
		switch (cur_status) {
		case SSIF_STATUS_WR_SINGLE:
		case SSIF_STATUS_WR_MULTI_START: {
			struct ssif_wr_start wr_start_msg;
			memset(&wr_start_msg, 0, sizeof(wr_start_msg));
			if (rlen - 1 - is_pec_exist > sizeof(wr_start_msg)) { // exclude smb_cmd, pec
				LOG_WRN("Invalid message length for smb command %d", cur_smb_cmd);
				cur_err_status = SSIF_STATUS_INVALID_LEN;
				goto reset;
			}
			memcpy(&wr_start_msg, rdata + 1, rlen - 1 - is_pec_exist); // exclude smb_cmd, pec

			if (wr_start_msg.len != (rlen - 2 - is_pec_exist)) { // exclude smb_cmd, len, pec
				LOG_WRN("Invalid length byte for smb command %d", cur_smb_cmd);
				cur_err_status = SSIF_STATUS_INVALID_LEN;
				goto reset;
			}

			current_ipmi_msg.buffer.InF_source = HOST_SSIF_1 + ssif_inst->index;
			current_ipmi_msg.buffer.netfn = wr_start_msg.netfn >> 2;
			current_ipmi_msg.buffer.cmd = wr_start_msg.cmd;
			current_ipmi_msg.buffer.data_len = wr_start_msg.len - 2; // exclude netfn, cmd
			if (current_ipmi_msg.buffer.data_len != 0) {
				memcpy(current_ipmi_msg.buffer.data, wr_start_msg.data,
				       current_ipmi_msg.buffer.data_len);
			}

			if (cur_status == SSIF_STATUS_WR_MULTI_START)
				continue;
			else {
				cur_action = SSIF_SEND_IPMI;
				break;
			}
		}

		case SSIF_STATUS_WR_MULTI_MIDDLE:
		case SSIF_STATUS_WR_MULTI_END: {
			struct ssif_wr_middle wr_middle_msg;
			memset(&wr_middle_msg, 0, sizeof(wr_middle_msg));

			if (rlen - 1 - is_pec_exist > sizeof(wr_middle_msg)) { // exclude smb_cmd, pec
				LOG_WRN("Invalid message length for smb command %d", cur_smb_cmd);
				cur_err_status = SSIF_STATUS_INVALID_LEN;
				goto reset;
			}
			memcpy(&wr_middle_msg, rdata + 1, rlen - 1 - is_pec_exist); // exclude smb_cmd, pec

			if (wr_middle_msg.len != (rlen - 2 - is_pec_exist)) { // exclude smb_cmd, len, pec
				LOG_WRN("Invalid length byte for smb command %d", cur_smb_cmd);
				cur_err_status = SSIF_STATUS_INVALID_LEN;
				goto reset;
			}

			if (cur_status == SSIF_STATUS_WR_MULTI_MIDDLE && wr_middle_msg.len != SSIF_MAX_IPMI_DATA_SIZE) {
				LOG_WRN("Invalid length for multi middle read");
				cur_err_status = SSIF_STATUS_INVALID_LEN;
				goto reset;
			}

			if (current_ipmi_msg.buffer.data_len == 0) {
				LOG_WRN("Lost first multi read message");
				cur_err_status = SSIF_STATUS_INVALID_CMD_IN_CUR_STATUS;
				goto reset;
			}

			memcpy(current_ipmi_msg.buffer.data + current_ipmi_msg.buffer.data_len, wr_middle_msg.data, wr_middle_msg.len);
			current_ipmi_msg.buffer.data_len += wr_middle_msg.len;

			if (cur_status == SSIF_STATUS_WR_MULTI_MIDDLE)
				continue;
			else {
				cur_action = SSIF_SEND_IPMI;
				break;
			}
		}

		default:
			LOG_ERR("Invalid status %d detect", cur_status);
			goto reset;
		}

		LOG_DBG("ipmi req netfn 0x%x, cmd 0x%x, data length %d:",
				current_ipmi_msg.buffer.netfn, current_ipmi_msg.buffer.cmd,
				current_ipmi_msg.buffer.data_len);
		LOG_HEXDUMP_DBG(current_ipmi_msg.buffer.data, current_ipmi_msg.buffer.data_len, "");

		if (ssif_do_action(cur_action, cur_smb_cmd, ssif_inst) == false) {
			cur_err_status = SSIF_STATUS_UNKNOWN_ERR;
			goto reset;
		}

		cur_err_status = SSIF_STATUS_NO_ERR;
reset:
		ssif_reset(ssif_inst);
	}
}

void ssif_device_init(uint8_t *config, uint8_t size)
{
	SAFE_FREE(ssif);

	ssif = (ssif_dev *)malloc(size * sizeof(*ssif));
	if (!ssif)
		return;
	memset(ssif, 0, size * sizeof(*ssif));
	
	ssif_channel_cnt = size;

	for (int i=0; i<size; i++) {
		if (config[i] >= I2C_BUS_MAX_NUM) {
			LOG_ERR("Given i2c bus index %d over limit", config[i]);
			continue;
		}

		if (k_mutex_init(&ssif->rd_buff_mutex)) {
			LOG_ERR("ssif %d rd mutex initial failed", i);
			continue;
		}

		if (k_sem_init(&ssif->rd_buff_sem, 0, 1)) {
			LOG_ERR("ssif %d rd semaphore initial failed", i);
			continue;
		}

		struct _i2c_target_config cfg;
		uint8_t ret = i2c_target_cfg_get(config[i], &cfg);
		if (ret) {
			LOG_ERR("Failed to get i2c %d target config cause of %d", config[i], ret);
			continue;
		}

		ssif[i].i2c_bus = config[i];
		ssif[i].addr = cfg.address;

		snprintf(ssif[i].task_name, sizeof(ssif[i].task_name), "ssif%d_polling", config[i]);

		ssif[i].ssif_task_tid = k_thread_create(&ssif[i].task_thread, ssif[i].ssif_task_stack,
							K_THREAD_STACK_SIZEOF(ssif[i].ssif_task_stack),
							ssif_read_task, (void *)&ssif[i], NULL, NULL,
							CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
		k_thread_name_set(ssif[i].ssif_task_tid, ssif[i].task_name);

		LOG_INF("SSIF %d created", config[i]);
	}

	return;
}

//#endif /* ENABLE_SSIF */

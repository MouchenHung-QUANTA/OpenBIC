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
#ifdef ENABLE_NVIDIA

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <logging/log.h>
#include "libutil.h"
#include "sensor.h"
#include "hal_i2c.h"
#include "nvidia.h"

LOG_MODULE_REGISTER(nv_smbpbi);

#define NV_SMBPBI_STATUS_CHECK_RETRY 5

static sys_slist_t smbpbi_p_list;

typedef struct {
	struct k_mutex mutex;
	uint8_t bus;
	uint8_t addr;
	sys_snode_t node; // linked list node
} smbpbi_unit;

smbpbi_unit *find_smbpbi_port(uint8_t bus, uint8_t addr)
{
	sys_snode_t *node = NULL;
	SYS_SLIST_FOR_EACH_NODE (&smbpbi_p_list, node) {
		smbpbi_unit *p;
		p = CONTAINER_OF(node, smbpbi_unit, node);
		if ( (p->bus == bus) && (p->addr == addr) ) {
			return p;
		}
	}

	return NULL;
}

uint8_t find_addr_by_id(uint8_t dev_id)
{
	switch (dev_id) {
		case NV_FPGA:
			return NV_FPGA_ADDR;
		case NV_HMC:
			return NV_HMC_ADDR;
		case NV_GPU1:
			return NV_GPU1_ADDR;
		case NV_GPU2:
			return NV_GPU2_ADDR;
		case NV_GPU3:
			return NV_GPU3_ADDR;
		case NV_GPU4:
			return NV_GPU4_ADDR;
		case NV_GPU5:
			return NV_GPU5_ADDR;
		case NV_GPU6:
			return NV_GPU6_ADDR;
		case NV_GPU7:
			return NV_GPU7_ADDR;
		case NV_GPU8:
			return NV_GPU8_ADDR;
		
		default:
			LOG_WRN("Invalid device id 0x%x", dev_id);
			break;
	}
	return 0;
}

bool nv_smbpbi_access(uint8_t bus, uint8_t dev_id, uint8_t opcode, uint8_t arg1, uint8_t arg2, uint32_t *val)
{
	CHECK_NULL_ARG_WITH_RETURN(val, false);

	bool ret = false;

	uint8_t addr = find_addr_by_id(dev_id);

	smbpbi_unit *p = find_smbpbi_port(bus, addr);
	if (!p) {
		p = (smbpbi_unit *)malloc(sizeof(smbpbi_unit));
		if (!p) {
			LOG_ERR("The smbpbi_unit malloc failed at port(bus:0x%x addr:0x%x)", bus, addr);
			return false;
		}

		p->bus = bus;
		p->addr = addr;

		if (k_mutex_init(&p->mutex)) {
			LOG_ERR("smbpbi port(bus:0x%x addr:0x%x) mutex initialize failed", p->bus, p->addr);
			SAFE_FREE(p);
			return false;
		}

		LOG_INF("smbpbi port(bus:0x%x addr:0x%x) mutex initialize success!", p->bus, p->addr);
		sys_slist_append(&smbpbi_p_list, &p->node);
	}

	if (k_mutex_lock(&p->mutex, K_MSEC(5000))) {
		LOG_WRN("smbpbi port(bus:0x%x addr:0x%x) mutex lock failed", p->bus, p->addr);
		return false;
	}

	struct nv_smbpbi_cmd_stat_reg cmd_status_reg = {0};

	I2C_MSG msg = { 0 };
	memset(&msg, 0, sizeof(msg));
	msg.bus = bus;
	msg.target_addr = addr;

	int rc = 0;
	int i2c_retry_time = 3;
	int retry = 0;

	/* write data (optional) */
	if (*val) {
		msg.tx_len = 6;
		msg.data[0] = NV_SMBPBI_DATA_REG;
		msg.data[1] = 4;
		memcpy(&msg.data[2], (uint8_t *)val, msg.data[1]);
		rc = i2c_master_write(&msg, i2c_retry_time);
		if (rc) {
			LOG_ERR("Failed to write data, rc: %d", rc);
			goto unlock;
		}
	}

	/* write command */
	msg.tx_len = 6;
	msg.data[0] = NV_SMBPBI_CMD_STAT_REG;
	msg.data[1] = 4;

	memset(&cmd_status_reg, 0, sizeof(cmd_status_reg));
	cmd_status_reg.opcode = opcode;
	cmd_status_reg.arg1 = arg1;
	cmd_status_reg.arg2 = arg2;
	cmd_status_reg.cmd_exe = 1;
	memcpy(&msg.data[2], (uint8_t *)&cmd_status_reg, msg.data[1]);

	rc = i2c_master_write(&msg, i2c_retry_time);
	if (rc) {
		LOG_ERR("Failed to write command, rc: %d", rc);
		goto unlock;
	}

	/* read status */
	for (retry = 0; retry < NV_SMBPBI_STATUS_CHECK_RETRY; retry++) {
		msg.tx_len = 1;
		msg.rx_len = 5;
		msg.data[0] = NV_SMBPBI_CMD_STAT_REG;

		rc = i2c_master_read(&msg, i2c_retry_time);
		if (rc) {
			LOG_ERR("Failed to read status, rc: %d", rc);
			goto unlock;
		}

		LOG_HEXDUMP_DBG(msg.data, 5, "read status:");

		if (msg.data[0] != 4) {
			LOG_WRN("Invalid read byte 0x%x while read status", msg.data[0]);
			continue;
		}
		memset(&cmd_status_reg, 0, sizeof(cmd_status_reg));
		memcpy(&cmd_status_reg, (uint8_t *)&msg.data[1], sizeof(cmd_status_reg));

		if (cmd_status_reg.cmd_exe != 0 ) {
			LOG_WRN("Command not accepted by target device");
			continue;
		}

		if (cmd_status_reg.status != NV_SMBPBI_STATUS_SUCCESS ) {
			LOG_WRN("Invalid smbpbi status 0x%x", cmd_status_reg.status);
			continue;
		}

		break;
	}

	if (retry == NV_SMBPBI_STATUS_CHECK_RETRY) {
		LOG_ERR("Read status retry failed");
		goto unlock;
	}

	/* read data (optional) */
	for (retry = 0; retry < NV_SMBPBI_STATUS_CHECK_RETRY; retry++) {
		msg.tx_len = 1;
		msg.rx_len = 5;
		msg.data[0] = NV_SMBPBI_DATA_REG;

		rc = i2c_master_read(&msg, i2c_retry_time);
		if (rc) {
			LOG_ERR("Failed to read data, rc: %d", rc);
			goto unlock;
		}

		LOG_HEXDUMP_DBG(msg.data, 5, "read data:");

		if (msg.data[0] != 4) {
			LOG_WRN("Invalid read byte 0x%x while read data", msg.data[0]);
			continue;
		}

		break;
	}

	if (retry == NV_SMBPBI_STATUS_CHECK_RETRY) {
		LOG_ERR("Read data retry failed");
		goto unlock;
	}

	memcpy(val, &msg.data[1], sizeof(uint32_t));

	ret = true;
unlock:
	if (k_mutex_unlock(&p->mutex))
		LOG_WRN("smbpbi port(bus:0x%x addr:0x%x) mutex unlock failed", p->bus, p->addr);

	return ret;
}

uint8_t nv_smbpbi_read(sensor_cfg *cfg, int *reading)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(reading, SENSOR_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(cfg->init_args, SENSOR_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		LOG_ERR("sensor num: 0x%x is invalid", cfg->num);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	nv_smbpbi_init_arg *init_arg = (nv_smbpbi_init_arg *)cfg->init_args;

	uint32_t ret_val = 0;
	if (nv_smbpbi_access(cfg->port, init_arg->dev_id, init_arg->opcode, init_arg->arg1, init_arg->arg2, &ret_val) == false) {
		LOG_ERR("Failed to access device %d with smbpbi op:0x%x arg1:0x%x arg2:0x%x", init_arg->dev_id, init_arg->opcode, init_arg->arg1, init_arg->arg2);
		return SENSOR_FAIL_TO_ACCESS;
	}

	LOG_DBG("opcode: 0x%x arg1: 0x%x arg2 0x%x reading: 0x%04x", init_arg->opcode, init_arg->arg1, init_arg->arg2, ret_val);

	sensor_val *sval = (sensor_val *)reading;

	/* TODO: need to be fixed based on sensor */
	switch (init_arg->opcode)
	{
	case SMBPBI_OPCODE_GET_TEMP_SINGLE:
		sval->integer = (int16_t)(ret_val >> 8);
		sval->fraction = 0;
		break;

	case SMBPBI_OPCODE_GET_TEMP_EXT:
		sval->integer = (int16_t)(ret_val >> 8);
		sval->fraction = (int16_t)(ret_val & 0xFF);
		break;

	default:
		LOG_ERR("Unsupported opcode");
		return SENSOR_UNSPECIFIED_ERROR;
	}

	LOG_INF("SMBPBI sensor #0x%x reading: 0x%04x (%d.%d)", cfg->num, ret_val, sval->integer, sval->fraction);

	return SENSOR_READ_SUCCESS;
}

uint8_t nv_smbpbi_init(sensor_cfg *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_INIT_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	cfg->read = nv_smbpbi_read;
	return SENSOR_INIT_SUCCESS;
}

#endif

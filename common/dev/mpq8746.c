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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <logging/log.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "pmbus.h"

LOG_MODULE_REGISTER(mpq8746);

#define VR_REG_VOUT_MODE 0x20

enum {
	VOUT_MODE_DIRECT,
	VOUT_MODE_VID,
	VOUT_MODE_LINEAR,
	VOUT_MODE_UNKNOWN = 0xFF,
};

uint8_t vr_vout_mode = VOUT_MODE_UNKNOWN;

uint8_t mpq8746_read(sensor_cfg *cfg, int *reading)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(reading, SENSOR_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		LOG_ERR("sensor num: 0x%x is invalid", cfg->num);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t i2c_max_retry = 5;
	float val = 0;

	I2C_MSG msg = {0};

	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = cfg->offset;

	if (i2c_master_read(&msg, i2c_max_retry)) {
		LOG_ERR("Failed to get sensor 0x%x value", cfg->num);
		return SENSOR_FAIL_TO_ACCESS;
	}

	switch (cfg->offset) {
	case PMBUS_READ_VOUT:
		val = (msg.data[1] << 8) | msg.data[0];
		if (vr_vout_mode == VOUT_MODE_LINEAR) {
			val *= 0.001953125;
		} else {
			val *= 0.0015625;
		}
		break;
	case PMBUS_READ_IOUT:
		val = (msg.data[1] << 8) | msg.data[0];
		val *= 0.0625;
		break;
	case PMBUS_READ_TEMPERATURE_1:
		val = msg.data[0];
		break;

	default:
		LOG_WRN("offset not supported: 0x%x", cfg->offset);
		return SENSOR_FAIL_TO_ACCESS;
		break;
	}

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;

	return SENSOR_READ_SUCCESS;
}

uint8_t mpq8746_init(sensor_cfg *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_INIT_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	uint8_t i2c_max_retry = 5;

	I2C_MSG msg = {0};
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = VR_REG_VOUT_MODE;

	if (i2c_master_read(&msg, i2c_max_retry)) {
		LOG_ERR("Failed to get vout mode");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	vr_vout_mode = (msg.data[0] & (BIT(5) | BIT(6))) >> 5;

	if (vr_vout_mode > VOUT_MODE_LINEAR)
		vr_vout_mode = VOUT_MODE_LINEAR;

	cfg->read = mpq8746_read;
	return SENSOR_INIT_SUCCESS;
}

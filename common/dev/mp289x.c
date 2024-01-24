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

LOG_MODULE_REGISTER(mp289x);

#define VR_REG_MFR_IMON_DIGI_GAIN 0xD0

uint8_t total_current_set = 0xFF;

static bool mp289x_set_page(uint8_t bus, uint8_t addr, uint8_t page)
{
	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;

	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 2;
	i2c_msg.data[0] = PMBUS_PAGE;
	i2c_msg.data[1] = page;

	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to set page to 0x%02X", page);
		return false;
	}

	return true;
}

static bool mp289x_pre_read(sensor_cfg *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, false);

	switch (cfg->offset)
	{
	case PMBUS_READ_VOUT:
	case PMBUS_READ_IOUT:
	case PMBUS_READ_TEMPERATURE_1:
	case PMBUS_READ_POUT:
		if (mp289x_set_page(cfg->port, cfg->target_addr, 0) == false)
			return false;
		break;
	
	default:
		LOG_WRN("Non-support ofset 0x%x", cfg->offset);
		break;
	}

	return true;
}

uint8_t mp289x_read(sensor_cfg *cfg, int *reading)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(reading, SENSOR_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		LOG_ERR("sensor num: 0x%x is invalid", cfg->num);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	if (mp289x_pre_read(cfg) == false) {
		LOG_ERR("sensor num: 0x%x pre-read failed", cfg->num);
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
		val = ((msg.data[1] << 8) | msg.data[0]) & BIT_MASK(12);
		val *= 0.001;
		break;
	case PMBUS_READ_IOUT:
		val = ((msg.data[1] << 8) | msg.data[0]) & BIT_MASK(12);
		val = ((total_current_set == 0) ? (0.25 * val) : (0.5 * val));
		break;
	case PMBUS_READ_TEMPERATURE_1:
		val = ((msg.data[1] << 8) | msg.data[0]) & BIT_MASK(12);
		val *= 0.1;
		break;
	case PMBUS_READ_POUT:
		val = ((msg.data[1] << 8) | msg.data[0]) & BIT_MASK(11);
		val = ((total_current_set == 0) ? (0.5 * val) : (1 * val));
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

uint8_t mp289x_init(sensor_cfg *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_INIT_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	if (mp289x_set_page(cfg->port, cfg->target_addr, 4) == false)
		return SENSOR_INIT_UNSPECIFIED_ERROR;

	uint8_t i2c_max_retry = 5;

	I2C_MSG msg = {0};
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = VR_REG_MFR_IMON_DIGI_GAIN;

	if (i2c_master_read(&msg, i2c_max_retry)) {
		LOG_ERR("Failed to get MFR_IMON_DIGI_GAIN");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	/* Get bit11 IMON_RESO */
	total_current_set = (msg.data[1] & BIT(3)) >> 3;

	cfg->read = mp289x_read;
	return SENSOR_INIT_SUCCESS;
}

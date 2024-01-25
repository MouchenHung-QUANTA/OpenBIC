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
#include <string.h>

#include "ina3221.h"
#include "sensor.h"
#include "hal_i2c.h"
#include <logging/log.h>

#define INA3221_BUS_VOLTAGE_LSB 0.008 // 8 mV.
#define INA3221_VSH_VOLTAGE_LSB 0.000040 // 40 uV.

LOG_MODULE_REGISTER(dev_ina3221);

uint8_t ina3221_read(sensor_cfg *cfg, int *reading)
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
	case INA3221_CH1_VSH_VOL_OFFSET:
	case INA3221_CH2_VSH_VOL_OFFSET:
	case INA3221_CH3_VSH_VOL_OFFSET:
		val = (((msg.data[1] << 8) | msg.data[0]) >> 3) & BIT_MASK(13);
		val *= INA3221_VSH_VOLTAGE_LSB;
		break;
	case INA3221_CH1_BUS_VOL_OFFSET:
	case INA3221_CH2_BUS_VOL_OFFSET:
	case INA3221_CH3_BUS_VOL_OFFSET:
		val = (((msg.data[1] << 8) | msg.data[0]) >> 3) & BIT_MASK(13);
		val *= INA3221_BUS_VOLTAGE_LSB;
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

uint8_t ina3221_init(sensor_cfg *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_INIT_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(cfg->init_args, SENSOR_INIT_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	cfg->read = ina3221_read;
	return SENSOR_INIT_SUCCESS;
}

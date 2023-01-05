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
#include <stdlib.h>
#include <logging/log.h>
#include "hal_i2c.h"
#include "pldm_firmware_update.h"
#include "libutil.h"
#include "lattice.h"

LOG_MODULE_REGISTER(lattice);

#define CHECK_STATUS_RETRY 100
#define IDCODE_PUB 0xE0
#define LSC_INIT_ADDRESS 0x46
#define LSC_INIT_ADDR_UFM 0x47
#define LSC_CHECK_BUSY 0xF0
#define USERCODE 0xC0
#define ISC_PROGRAM_USERCODE 0xC2
#define LSC_PROG_INCR_NV 0x70

static bool x02x03_i2c_update(lattice_update_config_t *config);
static bool x02x03_jtag_update(lattice_update_config_t *config);

struct lattice_dev_config LATTICE_CFG_TABLE[] = {
	/* Family LCMX02 */
	[LATTICE_LCMX02_2000HC] =
		{
			.name = "LCMXO2-2000HC",
			.id = 0x012BB043,
			.cpld_i2C_update = x02x03_i2c_update,
			.cpld_jtag_update = x02x03_jtag_update,
		},
	[LATTICE_LCMX02_4000HC] =
		{
			.name = "LCMXO2-4000HC",
			.id = 0x012BC043,
			.cpld_i2C_update = x02x03_i2c_update,
			.cpld_jtag_update = x02x03_jtag_update,
		},
	[LATTICE_LCMX02_7000HC] =
		{
			.name = "LCMXO2-7000HC",
			.id = 0x012BD043,
			.cpld_i2C_update = x02x03_i2c_update,
			.cpld_jtag_update = x02x03_jtag_update,
		},
	/* Family LCMX03 */
	[LATTICE_LCMX03_2100C] =
		{
			.name = "LCMXO3-2100C",
			.id = 0x612BB043,
			.cpld_i2C_update = x02x03_i2c_update,
			.cpld_jtag_update = NULL,
		},
	[LATTICE_LCMX03_4300C] =
		{
			.name = "LCMXO3-4300C",
			.id = 0x612BC043,
			.cpld_i2C_update = x02x03_i2c_update,
			.cpld_jtag_update = NULL,
		},
	[LATTICE_LCMX03_9400C] =
		{
			.name = "LCMXO3-9400C",
			.id = 0x612BE043,
			.cpld_i2C_update = x02x03_i2c_update,
			.cpld_jtag_update = NULL,
		},
	/* Family LFMNX */
	[LATTICE_LFMNX_50] =
		{
			.name = "LFMNX-50",
			.id = 0x412E3043,
			.cpld_i2C_update = NULL,
			.cpld_jtag_update = NULL,
		},
};

static uint8_t bit_swap(uint8_t input)
{
	uint8_t output = 0;

	for (int bit = 0; bit < 8; bit++) {
		output |= (((input & (1 << bit)) >> bit) << (7 - bit));
	}

	return output;
}

static bool read_cpld_busy_flag(uint8_t bus, uint8_t addr, uint16_t sleep_ms)
{
	//support XO2, XO3, NX
	for (int retry = 0; retry < CHECK_STATUS_RETRY; retry++) {
		I2C_MSG i2c_msg = { 0 };
		uint8_t retry = 3;
		i2c_msg.bus = bus;
		i2c_msg.target_addr = addr;

		i2c_msg.tx_len = 4;
		i2c_msg.rx_len = 1;
		memset(i2c_msg.data, 0, i2c_msg.tx_len);
		i2c_msg.data[0] = LSC_CHECK_BUSY;
		if (i2c_master_read(&i2c_msg, retry)) {
			LOG_ERR("Failed to send read_cpld_busy_flag cmd");
			return false;
		}

		if (((i2c_msg.data[0] & 0x80) >> 7) == 0x0) {
			return true;
		}
		k_msleep(sleep_ms);
	}

	LOG_ERR("CPLD is still busy after retry %d times", CHECK_STATUS_RETRY);
	return false;
}

static bool reset_addr(uint8_t bus, uint8_t addr, lattice_dev_type_t type, uint8_t sector)
{
	if (type >= LATTICE_UNKNOWN) {
		LOG_ERR("Invalid lattice device type detect");
		return false;
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 4;
	memset(i2c_msg.data, 0, i2c_msg.tx_len);

	if (type != LATTICE_LFMNX_50) {
		switch (sector) {
		case CFG0:
			i2c_msg.data[0] = LSC_INIT_ADDRESS;
			break;
		case UFM0:
			i2c_msg.data[0] = LSC_INIT_ADDR_UFM;
			break;
		}
	} else {
		switch (sector) {
		case CFG0:
			i2c_msg.data[0] = LSC_INIT_ADDRESS;
			i2c_msg.data[2] |= 1 << 0; //bit8
			break;
		case UFM0:
			i2c_msg.data[0] = LSC_INIT_ADDRESS;
			i2c_msg.data[2] |= 1 << 6; //bit14
			break;
		}
	}

	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to reset the flash pointer");
		return false;
	}

	return true;
}

bool cpld_i2c_get_id(uint8_t bus, uint8_t addr, uint32_t *dev_id)
{
	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 4;
	i2c_msg.rx_len = 4;
	memset(i2c_msg.data, 0, i2c_msg.tx_len);
	i2c_msg.data[0] = IDCODE_PUB;

	if (i2c_master_read(&i2c_msg, retry)) {
		LOG_ERR("Failed to read id register");
		return false;
	}

	memcpy(dev_id, i2c_msg.data, i2c_msg.rx_len);

	return true;
}

bool program_user_code(uint8_t bus, uint8_t addr, uint32_t usrcode, lattice_dev_type_t type)
{
	if (reset_addr(bus, addr, type, CFG0) == false) {
		return false;
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 8;
	memset(i2c_msg.data, 0, i2c_msg.tx_len);
	i2c_msg.data[0] = ISC_PROGRAM_USERCODE;
	for (int i = 0; i < 4; i++) {
		i2c_msg.data[4 + i] = (usrcode >> 8 * (3 - i)) & 0xFF;
	}
	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to read id register");
		return false;
	}

	if (read_cpld_busy_flag(bus, addr, 10) == false) {
		return false;
	}

	if (reset_addr(bus, addr, type, CFG0) == false) {
		return false;
	}

	i2c_msg.tx_len = 4;
	i2c_msg.rx_len = 4;
	memset(i2c_msg.data, 0, i2c_msg.tx_len);
	i2c_msg.data[0] = USERCODE;
	if (i2c_master_read(&i2c_msg, retry)) {
		LOG_ERR("Failed to read user code");
		return false;
	}

	uint32_t read_usrcode = 0;
	for (int i = 0; i < 4; i++)
		read_usrcode |= (i2c_msg.data[i] << 8 * (3 - i));

	return (memcmp(&read_usrcode, &usrcode, i2c_msg.rx_len) == 0) ? true : false;
}

bool cpld_program_i2c(uint8_t bus, uint8_t addr, uint8_t *buff, lattice_dev_type_t type,
		      uint8_t sector, bool first_flag)
{
	CHECK_NULL_ARG_WITH_RETURN(buff, false);

	if (first_flag == true) {
		if (reset_addr(bus, addr, type, sector) == false) {
			return false;
		}
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 4 + 16;
	memset(i2c_msg.data, 0, i2c_msg.tx_len);
	i2c_msg.data[0] = LSC_PROG_INCR_NV;
	i2c_msg.data[3] = 0x01;

	for (int index = 0; index < 16; index++) {
		if (!buff[index]) {
			LOG_ERR("Get invalid data buffer");
			return false;
		}
		i2c_msg.data[index + 4] = bit_swap(buff[index]);
	}

	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to send program page command");
		return false;
	}

	if (read_cpld_busy_flag(bus, addr, 10)) {
		return false;
	}

	return true;
}

static bool x02x03_i2c_update(lattice_update_config_t *config)
{
	CHECK_NULL_ARG_WITH_RETURN(config, false);

	bool ret = false;

	if (config->type >= ARRAY_SIZE(LATTICE_CFG_TABLE)) {
		LOG_ERR("Non-support type %d of lattice device detect", config->type);
		return false;
	}

	/* check local device id mach with given cpld type */
	uint32_t dev_id;
	if (cpld_i2c_get_id(config->bus, config->addr, &dev_id)) {
		LOG_ERR("Can't get cpld device id");
		return false;
	}
	if (dev_id != LATTICE_CFG_TABLE[config->type].id) {
		LOG_ERR("Given cpld type not match with local cpld device's type");
		return false;
	}

	/* TODO - Add parsing and return next offset and length */
	goto exit;

	ret = true;
exit:
	return ret;
}

static bool x02x03_jtag_update(lattice_update_config_t *config)
{
	bool ret = false;

	LOG_WRN("Currently not support update cpld via jtag");

	return ret;
}

bool lattice_fwupdate(lattice_update_config_t *config)
{
	CHECK_NULL_ARG_WITH_RETURN(config, false);

	if (config->type >= LATTICE_UNKNOWN) {
		LOG_ERR("Invalid type %d of LATTICE cpld chip detect", config->type);
		return false;
	}

	if (config->interface == CPLD_TAR_I2C) {
		if (LATTICE_CFG_TABLE[config->type].cpld_i2C_update) {
			if (LATTICE_CFG_TABLE[config->type].cpld_i2C_update(config) == false) {
				return false;
			}
		}
	} else if (config->interface == CPLD_TAR_JTAG) {
		if (LATTICE_CFG_TABLE[config->type].cpld_jtag_update) {
			if (LATTICE_CFG_TABLE[config->type].cpld_jtag_update(config) == false) {
				return false;
			}
		}
	} else {
		LOG_ERR("Given empty or invalid target interface");
		return false;
	}

	return true;
}

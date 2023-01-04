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

#define CMD_LINE_BYTE_CNT 16
#define MAX_CF_SIZE_PER_SECTION (CMD_LINE_BYTE_CNT * 64) //1k

#define KEYWORD_CF_START "L000"
#define KEYWORD_USR_CODE "NOTE User Electronic"

#define IDCODE_PUB 0xE0

static bool x02x03_i2c_update(void *fw_update_param);
static bool x02x03_jtag_update(void *fw_update_param);

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

static bool cpld_i2c_get_id(uint8_t bus, uint8_t addr, uint32_t *dev_id)
{
	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 4;
	i2c_msg.rx_len = 4;
	i2c_msg.data[0] = IDCODE_PUB;
	memset(&i2c_msg.data[1], 0, 3);
	if (i2c_master_read(&i2c_msg, retry)) {
		LOG_ERR("Failed to read id register");
		return false;
	}

	memcpy(dev_id, i2c_msg.data, i2c_msg.rx_len);

	return true;
}

static bool parsing_image(void *fw_update_param)
{
	bool ret = false;

	LOG_WRN("Currently not support fw update via ipmb portocol");
	ret = true;
	//exit:
	return ret;
}

static bool x02x03_i2c_update(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, false);

	pldm_fw_update_param_t *p = (pldm_fw_update_param_t *)fw_update_param;

	CHECK_NULL_ARG_WITH_RETURN(p->data, false);
	CHECK_NULL_ARG_WITH_RETURN(p->extra_parm, false);

	lattice_usr_config_t *ext = (lattice_usr_config_t *)p->extra_parm;

	bool ret = false;

	/* Step1. Before update */
	uint32_t dev_id;
	if (cpld_i2c_get_id(p->bus, p->addr, &dev_id)) {
		LOG_ERR("Can't get cpld device id");
		return false;
	}
	if (ext->dev_cfg->id != dev_id) {
		LOG_ERR("Given cpld type not match with local cpld device");
		return false;
	}

	/* Step2. Image collect */
	static uint8_t *hex_buff = NULL;
	static uint32_t hex_idx = 0;
	static bool cf_find = false;
	static uint8_t cf_keyword_box[4];
	static uint8_t usrcode_keyword_box[20];

	if (p->data_ofs == 0) {
		ext->img_cfg.CF_Line = 0;
		if (hex_buff) {
			LOG_ERR("previous hex_buff doesn't clean up!");
			return 1;
		}
		if (ext->img_cfg.CF) {
			LOG_ERR("previous CF doesn't clean up!");
			return 1;
		}
		hex_buff = malloc(MAX_CF_SIZE_PER_SECTION * 8);
		if (!hex_buff) {
			LOG_ERR("Failed to malloc hex_buff");
			return 1;
		}
		ext->img_cfg.CF = malloc(MAX_CF_SIZE_PER_SECTION);
		if (!ext->img_cfg.CF) {
			LOG_ERR("Failed to malloc CF");
			return 1;
		}
		memset(cf_keyword_box, 0, ARRAY_SIZE(cf_keyword_box));
		memset(usrcode_keyword_box, 0, ARRAY_SIZE(usrcode_keyword_box));
		cf_find = false;
	}

	memcpy(hex_buff + (int)p->data_ofs, p->data, p->data_len);

	p->next_ofs = p->data_ofs + p->data_len;
	p->next_len = fw_update_cfg.max_buff_size;

	if (cf_find == false) {
		if (p->next_ofs < fw_update_cfg.image_size) {
			if (p->next_ofs + p->next_len > fw_update_cfg.image_size)
				p->next_len = fw_update_cfg.image_size - p->next_ofs;
			return 0;
		} else {
			p->next_len = 0;
		}
	} else {

	}

	/* Step3. Image parsing */
	struct xdpe_config dev_cfg = { 0 };
	if (parsing_image(hex_buff, &dev_cfg) == false) {
		LOG_ERR("Failed to parsing image!");
		goto exit;
	}
	SAFE_FREE(hex_buff);

	/* Step4. FW update */

	/* Step5. FW verify */

	ret = true;
exit:
	return ret;
}

static bool x02x03_jtag_update(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, false);

	bool ret = false;

	LOG_WRN("Currently not support update cpld via jtag");

	ret = true;
	//exit:
	return ret;
}

uint8_t lattice_fwupdate(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, 1);

	pldm_fw_update_param_t *p = (pldm_fw_update_param_t *)fw_update_param;

	CHECK_NULL_ARG_WITH_RETURN(p->data, 1);
	CHECK_NULL_ARG_WITH_RETURN(p->extra_parm, 1);

	lattice_usr_config_t *ext = (lattice_usr_config_t *)p->extra_parm;

	if (ext->tar_cfg.type >= LATTICE_UNKNOWN) {
		LOG_ERR("Invalid type %d of LATTICE cpld chip detect", ext->tar_cfg.type);
		return 1;
	}

	/* paste info table to config */
	memcpy(ext->dev_cfg, &LATTICE_CFG_TABLE[ext->tar_cfg.type], sizeof(struct lattice_dev_config));

	if (ext->tar_cfg.select_tar_inf == CPLD_TAR_I2C) {
		if (LATTICE_CFG_TABLE[ext->tar_cfg.type].cpld_i2C_update) {
			if (LATTICE_CFG_TABLE[ext->tar_cfg.type].cpld_i2C_update(fw_update_param) == false) {
				return 1;
			}
		}
	} else if (ext->tar_cfg.select_tar_inf == CPLD_TAR_JTAG) {
		if (LATTICE_CFG_TABLE[ext->tar_cfg.type].cpld_jtag_update) {
			if (LATTICE_CFG_TABLE[ext->tar_cfg.type].cpld_jtag_update(fw_update_param) == false) {
				return 1;
			}
		}
	} else {
		LOG_ERR("Given empty or invalid target interface");
		return 1;
	}

	return 0;
}

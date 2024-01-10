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

#include "libutil.h"
#include "util_spi.h"
#include "hal_jtag.h"
#include "sensor.h"
#include "i2c-mux-tca9548.h"
#include "lattice.h"

#include "pldm_firmware_update.h"
#include "plat_pldm_fw_update.h"
#include "plat_gpio.h"
#include "plat_i2c.h"
#include "plat_sensor_table.h"
#include "plat_hook.h"
#include "plat_class.h"

LOG_MODULE_REGISTER(plat_fwupdate);

static uint8_t pldm_pre_cpld_update(void *fw_update_param);
static uint8_t pldm_pre_nv_fpga_update(void *fw_update_param);
static uint8_t pldm_nv_fpga_update(void *fw_update_param);
static uint8_t pldm_post_nv_fpga_update(void *fw_update_param);
static bool get_cpld_user_code(void *info_p, uint8_t *buf, uint8_t *len);
static bool get_nv_fpga_fw_version(void *info_p, uint8_t *buf, uint8_t *len);

/* PLDM FW update table */
pldm_fw_update_info_t PLDMUPDATE_FW_CONFIG_TABLE[] = {
	{
		.enable = true,
		.comp_classification = COMP_CLASS_TYPE_DOWNSTREAM,
		.comp_identifier = CL_COMPNT_BIC,
		.comp_classification_index = 0x00,
		.pre_update_func = NULL,
		.update_func = pldm_bic_update,
		.pos_update_func = NULL,
		.inf = COMP_UPDATE_VIA_SPI,
		.activate_method = COMP_ACT_SELF,
		.self_act_func = pldm_bic_activate,
		.get_fw_version_fn = NULL,
	},
	{
		.enable = true,
		.comp_classification = COMP_CLASS_TYPE_DOWNSTREAM,
		.comp_identifier = CL_COMPNT_NV_FPGA,
		.comp_classification_index = 0x00,
		.pre_update_func = pldm_pre_nv_fpga_update,
		.update_func = pldm_nv_fpga_update,
		.pos_update_func = pldm_post_nv_fpga_update,
		.inf = COMP_UPDATE_VIA_SPI,
		.activate_method = COMP_ACT_DC_PWR_CYCLE,
		.self_act_func = NULL,
		.get_fw_version_fn = get_nv_fpga_fw_version,
	},
	{
		.enable = true,
		.comp_classification = COMP_CLASS_TYPE_DOWNSTREAM,
		.comp_identifier = CL_COMPNT_CPLD,
		.comp_classification_index = 0x00,
		.pre_update_func = pldm_pre_cpld_update,
		.update_func = pldm_cpld_update,
		.pos_update_func = NULL,
		.inf = COMP_UPDATE_VIA_I2C,
		.activate_method = COMP_ACT_AC_PWR_CYCLE,
		.self_act_func = NULL,
		.get_fw_version_fn = get_cpld_user_code,
	},
};

void load_pldmupdate_comp_config(void)
{
	if (comp_config) {
		LOG_WRN("PLDM update component table has already been load");
		return;
	}

	comp_config_count = ARRAY_SIZE(PLDMUPDATE_FW_CONFIG_TABLE);
	comp_config = malloc(sizeof(pldm_fw_update_info_t) * comp_config_count);
	if (!comp_config) {
		LOG_ERR("comp_config malloc failed");
		return;
	}

	memcpy(comp_config, PLDMUPDATE_FW_CONFIG_TABLE, sizeof(PLDMUPDATE_FW_CONFIG_TABLE));
}

static uint8_t pldm_pre_cpld_update(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, 1);

	pldm_fw_update_param_t *p = (pldm_fw_update_param_t *)fw_update_param;

	if (p->inf == COMP_UPDATE_VIA_I2C) {
		p->bus = I2C_BUS1;
		p->addr = CPLD_ADDR;
	}

	return 0;
}

static uint8_t pldm_pre_nv_fpga_update(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, 1);

	bool ret = pal_switch_bios_spi_mux(GPIO_HIGH);
	if (!ret) {
		LOG_ERR("Failed to open spi mux");
		return 1;
	}

	return 0;
}

/* pldm fw-update func */
static uint8_t pldm_nv_fpga_update(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, 1);

	pldm_fw_update_param_t *p = (pldm_fw_update_param_t *)fw_update_param;

	CHECK_NULL_ARG_WITH_RETURN(p->data, 1);

	int pos = pal_get_bios_flash_position();
	if (pos == -1) {
		LOG_ERR("Failed to get bios flash position");
		return 1;
	}

	uint8_t update_flag = 0;

	/* prepare next data offset and length */
	p->next_ofs = p->data_ofs + p->data_len;
	p->next_len = fw_update_cfg.max_buff_size;

	if (p->next_ofs < fw_update_cfg.image_size) {
		if (p->next_ofs + p->next_len > fw_update_cfg.image_size)
			p->next_len = fw_update_cfg.image_size - p->next_ofs;

		if (((p->next_ofs % SECTOR_SZ_64K) + p->next_len) > SECTOR_SZ_64K)
			p->next_len = SECTOR_SZ_64K - (p->next_ofs % SECTOR_SZ_64K);
	} else {
		/* current data is the last packet
		 * set the next data length to 0 to inform the update completely
		 */
		p->next_len = 0;
		update_flag = (SECTOR_END_FLAG | NO_RESET_FLAG);
	}

	uint8_t ret = fw_update(p->data_ofs, p->data_len, p->data, update_flag, pos);

	if (ret) {
		LOG_ERR("Firmware update failed, offset(0x%x), length(0x%x), status(%d)",
			p->data_ofs, p->data_len, ret);
		return 1;
	}

	return 0;
}

static uint8_t pldm_post_nv_fpga_update(void *fw_update_param)
{
	ARG_UNUSED(fw_update_param);

	bool ret = pal_switch_bios_spi_mux(GPIO_LOW);
	if (!ret) {
		LOG_ERR("Failed to close spi mux");
		return 1;
	}

	return 0;
}

static bool get_cpld_user_code(void *info_p, uint8_t *buf, uint8_t *len)
{
	CHECK_NULL_ARG_WITH_RETURN(buf, false);
	CHECK_NULL_ARG_WITH_RETURN(len, false);
	ARG_UNUSED(info_p);

	uint8_t tmp_buf[4] = { 0 };
	uint32_t read_usrcode = 0;

	bool ret = cpld_i2c_get_usercode(I2C_BUS1, CPLD_ADDR, &read_usrcode);
	if (ret == false) {
		LOG_ERR("Fail to get CPLD usercode");
		return false;
	}

	memcpy(tmp_buf, &read_usrcode, sizeof(read_usrcode));
	*len = bin2hex(tmp_buf, 4, buf, 8);

	return true;
}

#define PLDM_PLAT_ERR_CODE_NO_POWER_ON 8
static bool get_nv_fpga_fw_version(void *info_p, uint8_t *buf, uint8_t *len)
{
	CHECK_NULL_ARG_WITH_RETURN(buf, false);
	CHECK_NULL_ARG_WITH_RETURN(len, false);
	CHECK_NULL_ARG_WITH_RETURN(info_p, false);

	LOG_WRN("Not support yet!");

	return false;
}

void clear_pending_version(uint8_t activate_method)
{
	if (!comp_config || !comp_config_count) {
		LOG_ERR("Component configuration is empty");
		return;
	}

	for (uint8_t i = 0; i < comp_config_count; i++) {
		if (comp_config[i].activate_method == activate_method)
			SAFE_FREE(comp_config[i].pending_version_p);
	}
}

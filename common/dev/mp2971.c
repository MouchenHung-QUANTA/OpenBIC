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
#include "pldm_firmware_update.h"
#include "mp2971.h"

LOG_MODULE_REGISTER(mp2971);

#define VR_MPS_PAGE_0 0x00
#define VR_MPS_PAGE_1 0x01
#define VR_MPS_PAGE_2 0x02
#define VR_MPS_PAGE_29 0x29
#define VR_MPS_PAGE_2A 0x2A

#define VR_MPS_CMD_STORE_NORMAL_CODE 0xF1

#define VR_MPS_REG_MFR_MTP_PMBUS_CTRL 0x4F

#define MFR_RESO_SET 0xC7

/*Page0 */
#define VR_MPS_REG_WRITE_PROTECT 0x10

/*Page1 */
#define VR_MPS_REG_MFR_VR_CONFIG2 0x35

/*Page2 */
#define VR_MPS_CMD_STORE_MULTI_CODE 0xF3

#define MP2856_DISABLE_WRITE_PROTECT 0x63

/* STATUS_CML bit[3] */
#define MASK_PWD_MATCH 0x08
/* MFR_VR_CONFIG2 bit[2] */
#define MASK_WRITE_PROTECT_MODE 0x04
/* MFR_MTP_PMBUS_CTRL bit[5] */
#define MASK_MTP_BYTE_RW_EN 0x20

#define MAX_CMD_LINE 720

enum { ATE_CONF_ID = 0,
       ATE_PAGE_NUM,
       ATE_REG_ADDR_HEX,
       ATE_REG_ADDR_DEC,
       ATE_REG_NAME,
       ATE_REG_DATA_HEX,
       ATE_REG_DATA_DEC,
       ATE_WRITE_TYPE,
       ATE_COL_MAX,
};

struct mp2856_data {
	uint16_t cfg_id;
	uint8_t page;
	uint8_t reg_addr;
	uint32_t reg_data;
	uint8_t reg_len;
};

struct mp2856_config {
	uint8_t mode;
	uint8_t addr;
	uint16_t cfg_id;
	uint16_t wr_cnt;
	uint16_t product_id_exp;
	struct mp2856_data *pdata;
};

static bool mp2856_set_page(uint8_t bus, uint8_t addr, uint8_t page)
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

static bool mp2856_write_data(uint8_t bus, uint8_t addr, struct mp2856_data *data)
{
	CHECK_NULL_ARG_WITH_RETURN(data, false);

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;

	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = data->reg_len + 1;
	i2c_msg.data[0] = data->reg_addr;
	memcpy(&i2c_msg.data[1], &data->reg_data, data->reg_len);

	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to write register 0x%02X", data->reg_addr);
		return false;
	}

	return true;
}

static bool mp2856_enable_mtp_page_rw(uint8_t bus, uint8_t addr)
{
	if (mp2856_set_page(bus, addr, VR_MPS_PAGE_1) == false) {
		return false;
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 2;
	i2c_msg.data[0] = VR_MPS_REG_MFR_MTP_PMBUS_CTRL;

	if (i2c_master_read(&i2c_msg, retry)) {
		LOG_ERR("Failed to read register 0x%02X", VR_MPS_REG_MFR_MTP_PMBUS_CTRL);
		return false;
	}

	uint8_t rsp[i2c_msg.rx_len];
	memcpy(rsp, i2c_msg.data, i2c_msg.rx_len);

	if ((i2c_msg.data[0] & MASK_MTP_BYTE_RW_EN) == 0) {
		i2c_msg.tx_len = 3;
		i2c_msg.data[0] = VR_MPS_REG_MFR_MTP_PMBUS_CTRL;
		i2c_msg.data[1] = rsp[0] | MASK_MTP_BYTE_RW_EN;
		i2c_msg.data[2] = rsp[1];

		if (i2c_master_write(&i2c_msg, retry)) {
			LOG_ERR("Failed to write register 0x%02X", VR_MPS_REG_MFR_MTP_PMBUS_CTRL);
			return false;
		}
	}

	return true;
}

static bool mp2856_is_pwd_unlock(uint8_t bus, uint8_t addr)
{
	if (mp2856_set_page(bus, addr, VR_MPS_PAGE_0) == false) {
		return false;
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 1;
	i2c_msg.data[0] = PMBUS_STATUS_CML;

	if (i2c_master_read(&i2c_msg, retry)) {
		LOG_ERR("Failed to read register 0x%02X", PMBUS_STATUS_CML);
		return false;
	}

	if ((i2c_msg.data[0] & MASK_PWD_MATCH) == 0x00) {
		LOG_ERR("PWD_MATCH not set!\n");
		return false;
	}

	return true;
}

static bool mp2856_unlock_write_protect_mode(uint8_t bus, uint8_t addr)
{
	if (mp2856_set_page(bus, addr, VR_MPS_PAGE_1) == false) {
		return false;
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;
	i2c_msg.bus = bus;
	i2c_msg.target_addr = addr;

	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 2;
	i2c_msg.data[0] = VR_MPS_REG_MFR_VR_CONFIG2;

	if (i2c_master_read(&i2c_msg, retry)) {
		LOG_ERR("Failed to read register 0x%02X", VR_MPS_REG_MFR_VR_CONFIG2);
		return false;
	}

	if ((i2c_msg.data[1] & MASK_WRITE_PROTECT_MODE) == 0) {
		//MTP protection mode
		//check write protect status
		if (mp2856_set_page(bus, addr, VR_MPS_PAGE_0) == false) {
			return false;
		}

		i2c_msg.tx_len = 1;
		i2c_msg.rx_len = 1;
		i2c_msg.data[0] = VR_MPS_REG_WRITE_PROTECT;

		if (i2c_master_read(&i2c_msg, retry)) {
			LOG_ERR("Failed to read register 0x%02X", VR_MPS_REG_WRITE_PROTECT);
			return false;
		}

		if (i2c_msg.data[0] == MP2856_DISABLE_WRITE_PROTECT) {
			return true;
		} else {
			//Unlock MTP Write protection
			i2c_msg.tx_len = 2;
			i2c_msg.data[0] = VR_MPS_REG_WRITE_PROTECT;
			i2c_msg.data[1] = MP2856_DISABLE_WRITE_PROTECT;

			if (i2c_master_write(&i2c_msg, retry)) {
				LOG_ERR("Failed to write register 0x%02X",
					VR_MPS_REG_WRITE_PROTECT);
				return false;
			}
		}
	}

	return true;
}

static bool parsing_image(uint8_t *hex_buff, struct mp2856_config *dev_cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(hex_buff, false);
	CHECK_NULL_ARG_WITH_RETURN(dev_cfg, false);

	bool ret = false;

	/* Parsing image */
	int max_line = MAX_CMD_LINE;
	dev_cfg->pdata = (struct mp2856_data *)malloc(sizeof(struct mp2856_data) * max_line);
	if (!dev_cfg->pdata) {
		LOG_ERR("pdata malloc failed!");
		goto exit;
	}

	struct mp2856_data *cur_line = &dev_cfg->pdata[0];
	uint8_t cur_ele_idx = 0;
	uint32_t data_store = 0;
	uint8_t data_idx = 0;
	dev_cfg->wr_cnt = 0;
	for (int i = 0; i < fw_update_cfg.image_size; i++) {
		/* check valid */
		if (!hex_buff[i]) {
			LOG_ERR("Get invalid buffer data at index %d", i);
			goto exit;
		}

		if (cur_ele_idx == ATE_CONF_ID && i + 2 < fw_update_cfg.image_size) {
			if (!strncmp(&hex_buff[i], "END", 3)) {
				break;
			}
		}
		if (hex_buff[i] != 0x09 && hex_buff[i] != 0x0d) {
			// pass non hex charactor
			int val = ascii_to_val(hex_buff[i]);
			if (val == -1)
				continue;
			data_store = (data_store << 4) | val;
			data_idx++;
			continue;
		}

		uint8_t byte_cnt = data_idx % 2 == 0 ? data_idx / 2 : (data_idx / 2 + 1);
		switch (cur_ele_idx) {
		case ATE_CONF_ID:
			cur_line->cfg_id = data_store & 0xffff;
			break;

		case ATE_PAGE_NUM:
			cur_line->page = data_store & 0xff;
			break;

		case ATE_REG_ADDR_HEX:
			cur_line->reg_addr = data_store & 0xff;
			break;

		case ATE_REG_ADDR_DEC:
			break;

		case ATE_REG_NAME:
			break;

		case ATE_REG_DATA_HEX:
			cur_line->reg_data = data_store;
			cur_line->reg_len = byte_cnt;
			break;

		case ATE_REG_DATA_DEC:
			break;

		case ATE_WRITE_TYPE:
			break;

		default:
			LOG_ERR("Got unknow element index %d", cur_ele_idx);
			goto exit;
		}

		data_idx = 0;
		data_store = 0;
		if (hex_buff[i] == 0x09) {
			cur_ele_idx++;
		} else if (hex_buff[i] == 0x0d) {
			LOG_DBG("vr[%d] page: %d addr:%x data:%x\n", dev_cfg->wr_cnt,
				cur_line->page, cur_line->reg_addr, cur_line->reg_data);
			cur_ele_idx = 0;
			dev_cfg->wr_cnt++;
			if (dev_cfg->wr_cnt > max_line) {
				LOG_ERR("Line record count is overlimit");
				goto exit;
			}
			cur_line++;
			i++; //skip 'a'
		}
	}

	ret = true;

exit:
	if (ret == false)
		SAFE_FREE(dev_cfg->pdata);

	return ret;
}

uint8_t mp2971_fwupdate(void *fw_update_param)
{
	CHECK_NULL_ARG_WITH_RETURN(fw_update_param, 1);

	pldm_fw_update_param_t *p = (pldm_fw_update_param_t *)fw_update_param;

	CHECK_NULL_ARG_WITH_RETURN(p->data, 1);

	uint8_t ret = 1;

	uint8_t dev_i2c_bus = p->bus;
	uint8_t dev_i2c_addr = p->addr;

	struct mp2856_config dev_cfg = { 0 };

	/* Step1. Before update */
	// none

	/* Step2. Image collect */
	static uint8_t *hex_buff = NULL;
	if (p->data_ofs == 0) {
		if (hex_buff) {
			LOG_ERR("previous hex_buff doesn't clean up!");
			return 1;
		}
		hex_buff = malloc(fw_update_cfg.image_size);
		if (!hex_buff) {
			LOG_ERR("Failed to malloc hex_buff");
			return 1;
		}
	}

	memcpy(hex_buff + (int)p->data_ofs, p->data, p->data_len);

	p->next_ofs = p->data_ofs + p->data_len;
	p->next_len = fw_update_cfg.max_buff_size;

	if (p->next_ofs < fw_update_cfg.image_size) {
		if (p->next_ofs + p->next_len > fw_update_cfg.image_size)
			p->next_len = fw_update_cfg.image_size - p->next_ofs;
		return 0;
	} else {
		p->next_len = 0;
	}

	/* Step3. Image parsing */
	if (parsing_image(hex_buff, &dev_cfg) == false) {
		LOG_ERR("Failed to parsing image!");
		goto exit;
	}
	SAFE_FREE(hex_buff);

	/* Step4. FW Update */
	if (mp2856_is_pwd_unlock(dev_i2c_bus, dev_i2c_addr) == false) {
		LOG_ERR("Failed to PWD UNLOCK");
		goto exit;
	}

	if (mp2856_unlock_write_protect_mode(dev_i2c_bus, dev_i2c_addr) == false) {
		LOG_ERR("Failed to unlock MTP Write protection");
		goto exit;
	}

	if (mp2856_set_page(dev_i2c_bus, dev_i2c_addr, VR_MPS_PAGE_0) == false) {
		goto exit;
	}

	uint8_t page = 0;
	int page2_start = 0;
	struct mp2856_data *cur_data;
	uint16_t line_idx = 0;

	//Program Page0 and Page1 registers
	for (line_idx = 0; line_idx < dev_cfg.wr_cnt; line_idx++) {
		cur_data = &dev_cfg.pdata[line_idx];
		if (cur_data->page == 2) {
			page2_start = line_idx;
			break;
		}
		if (page != cur_data->page) {
			if (mp2856_set_page(dev_i2c_bus, dev_i2c_addr, cur_data->page) == false) {
				goto exit;
			}
			page = cur_data->page;
		}
		mp2856_write_data(dev_i2c_bus, dev_i2c_addr, cur_data);

		uint8_t percent = ((line_idx + 1) * 100) / dev_cfg.wr_cnt;
		if (percent % 10 == 0)
			LOG_INF("updated: %d%% (line: %d/%d page: %d)", percent, line_idx + 1,
				dev_cfg.wr_cnt, cur_data->page);
	}

	//Store Page0/1 reggisters to MTP
	if (mp2856_set_page(dev_i2c_bus, dev_i2c_addr, VR_MPS_PAGE_0) == false) {
		goto exit;
	}

	I2C_MSG i2c_msg = { 0 };
	uint8_t retry = 3;

	i2c_msg.bus = dev_i2c_bus;
	i2c_msg.target_addr = dev_i2c_addr;

	i2c_msg.tx_len = 1;
	i2c_msg.data[0] = VR_MPS_CMD_STORE_NORMAL_CODE;

	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to write register 0x%02X", VR_MPS_CMD_STORE_NORMAL_CODE);
		goto exit;
	}
	k_msleep(500); //wait command finish

	if (mp2856_enable_mtp_page_rw(dev_i2c_bus, dev_i2c_addr) == false) {
		printf("ERROR: Enable MTP PAGE RW FAILED!\n");
		goto exit;
	}

	//Enable STORE_MULTI_CODE
	if (mp2856_set_page(dev_i2c_bus, dev_i2c_addr, VR_MPS_PAGE_2) == false) {
		goto exit;
	}

	i2c_msg.tx_len = 1;
	i2c_msg.data[0] = VR_MPS_CMD_STORE_MULTI_CODE;
	if (i2c_master_write(&i2c_msg, retry)) {
		LOG_ERR("Failed to write register 0x%02X", VR_MPS_CMD_STORE_MULTI_CODE);
		goto exit;
	}

	if (mp2856_set_page(dev_i2c_bus, dev_i2c_addr, VR_MPS_PAGE_2A) == false) {
		goto exit;
	}
	k_msleep(2); //wait command finish

	//Program Page2 registers
	for (line_idx = page2_start; line_idx < dev_cfg.wr_cnt; line_idx++) {
		cur_data = &dev_cfg.pdata[line_idx];
		if (cur_data->page != 2) {
			break;
		}
		mp2856_write_data(dev_i2c_bus, dev_i2c_addr, cur_data);
		k_msleep(2);

		uint8_t percent = ((line_idx + 1) * 100) / dev_cfg.wr_cnt;
		if (percent % 10 == 0)
			LOG_INF("updated: %d%% (line: %d/%d page: %d)", percent, line_idx + 1,
				dev_cfg.wr_cnt, cur_data->page);
	}

	if (mp2856_set_page(dev_i2c_bus, dev_i2c_addr, VR_MPS_PAGE_1) == false) {
		goto exit;
	}

	/* Step5. FW verify */
	// TODO

	ret = 0;
exit:
	SAFE_FREE(hex_buff);
	SAFE_FREE(dev_cfg.pdata);
	return ret;
}

float get_resolution(uint8_t sensor_num)
{
	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	uint8_t page = 0;
	uint16_t mfr_reso_set = 0;

	I2C_MSG msg;
	uint8_t i2c_max_retry = 5;

	//get page
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = PMBUS_PAGE;

	if (i2c_master_read(&msg, i2c_max_retry)) {
		LOG_WRN("i2c read failed.\n");
		return SENSOR_FAIL_TO_ACCESS;
	}

	page = msg.data[0];

	//get reso set
	msg.rx_len = 2;
	msg.data[0] = MFR_RESO_SET;

	if (i2c_master_read(&msg, i2c_max_retry)) {
		LOG_WRN("i2c read failed.\n");
		return SENSOR_FAIL_TO_ACCESS;
	}

	mfr_reso_set = (msg.data[1] << 8) | msg.data[0];

	uint8_t vout_reso_set;
	uint8_t iout_reso_set;
	uint8_t iin_reso_set;
	uint8_t pout_reso_set;

	float vout_reso = 0;
	float iout_reso = 0;
	float iin_reso = 0;
	float pout_reso = 0;
	float temp_reso = 1;

	//get reso from MFR_RESO_SET(C7h)
	if (page == 0) {
		vout_reso_set = (mfr_reso_set & GENMASK(7, 6)) >> 6;
		iout_reso_set = (mfr_reso_set & GENMASK(5, 4)) >> 4;
		iin_reso_set = (mfr_reso_set & GENMASK(3, 2)) >> 2;
		pout_reso_set = (mfr_reso_set & GENMASK(1, 0));

		if (vout_reso_set & BIT(1)) {
			vout_reso = 0.001;
		} else {
			LOG_WRN("vout_reso_set not supported: 0x%x\n", vout_reso_set);
		}

		if (iout_reso_set == 0) {
			iout_reso = 2;
		} else if (iout_reso_set == 1) {
			iout_reso = 1;
		} else if (iout_reso_set == 2) {
			iout_reso = 0.5;
		} else {
			LOG_WRN("iout_reso_set not supported: 0x%x\n", iout_reso_set);
		}

		if (iin_reso_set == 0) {
			iin_reso = 0.5;
		} else if (iin_reso_set == 1) {
			iin_reso = 0.25;
		} else if (iin_reso_set == 2) {
			iin_reso = 0.125;
		} else {
			LOG_WRN("iin_reso_set not supported: 0x%x\n", iin_reso_set);
		}

		if (pout_reso_set == 0) {
			pout_reso = 2;
		} else if (pout_reso_set == 1) {
			pout_reso = 1;
		} else if (pout_reso_set == 2) {
			pout_reso = 0.5;
		} else {
			LOG_WRN("pout_reso_set not supported: 0x%x\n", pout_reso_set);
		}

	} else if (page == 1) {
		vout_reso_set = (mfr_reso_set & GENMASK(4, 3)) >> 3;
		iout_reso_set = (mfr_reso_set & GENMASK(2, 2)) >> 2;
		pout_reso_set = (mfr_reso_set & GENMASK(0, 0));

		if (vout_reso_set & BIT(1)) {
			vout_reso = 0.001;
		} else {
			LOG_WRN("vout_reso_set not supported: 0x%x\n", vout_reso_set);
		}

		if (iout_reso_set == 0) {
			iout_reso = 1;
		} else if (iout_reso_set == 1) {
			iout_reso = 0.5;
		} else {
			LOG_WRN("iout_reso_set not supported: 0x%x\n", iout_reso_set);
		}

		iin_reso = 0.125;

		if (pout_reso_set == 0) {
			pout_reso = 1;
		} else if (pout_reso_set == 1) {
			pout_reso = 0.5;
		} else {
			LOG_WRN("pout_reso_set not supported: 0x%x\n", pout_reso_set);
		}
	} else {
		LOG_WRN("Page not supported: 0x%d\n", page);
	}

	uint8_t offset = cfg->offset;

	switch (offset) {
	case PMBUS_READ_VOUT:
		return vout_reso;
		break;
	case PMBUS_READ_IOUT:
		return iout_reso;
		break;
	case PMBUS_READ_IIN:
		return iin_reso;
		break;
	case PMBUS_READ_TEMPERATURE_1:
		return temp_reso;
		break;
	case PMBUS_READ_POUT:
		return pout_reso;
		break;
	default:
		LOG_WRN("offset not supported: 0x%x\n", offset);
		break;
	}
	return 0;
}

uint8_t mp2971_read(uint8_t sensor_num, int *reading)
{
	CHECK_NULL_ARG_WITH_RETURN(reading, SENSOR_UNSPECIFIED_ERROR);
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	uint8_t i2c_max_retry = 5;
	int val = 0;
	sensor_val *sval = (sensor_val *)reading;
	I2C_MSG msg;
	memset(sval, 0, sizeof(sensor_val));

	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = cfg->offset;

	if (i2c_master_read(&msg, i2c_max_retry)) {
		/* read fail */
		return SENSOR_FAIL_TO_ACCESS;
	}

	uint8_t offset = cfg->offset;
	val = (msg.data[1] << 8) | msg.data[0];

	switch (offset) {
	case PMBUS_READ_VOUT:
		/* 1 mV/LSB, unsigned integer */
		val = val & BIT_MASK(12);
		break;
	case PMBUS_READ_IOUT:
		val = val & BIT_MASK(11);
		break;
	case PMBUS_READ_IIN:
		val = val & BIT_MASK(11);
		break;
	case PMBUS_READ_TEMPERATURE_1:
		val = val & BIT_MASK(8);
		break;
	case PMBUS_READ_POUT:
		val = val & BIT_MASK(11);
		break;
	default:
		LOG_WRN("offset not supported: 0x%x\n", offset);
		return SENSOR_FAIL_TO_ACCESS;
		break;
	}

	float resolution = get_resolution(sensor_num);
	if (resolution == 0) {
		return SENSOR_FAIL_TO_ACCESS;
	}
	sval->integer = (int16_t)(val * resolution);
	sval->fraction = (int16_t)((val - (sval->integer / resolution)) * (resolution * 1000));

	return SENSOR_READ_SUCCESS;
}

uint8_t mp2971_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = mp2971_read;
	return SENSOR_INIT_SUCCESS;
}

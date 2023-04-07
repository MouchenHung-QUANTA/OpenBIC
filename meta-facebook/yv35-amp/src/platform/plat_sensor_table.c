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

#include <stdlib.h>
#include <stdio.h>
#include <logging/log.h>
#include "plat_sensor_table.h"
#include "sensor.h"
#include "ast_adc.h"
#include "plat_hook.h"
#include "pmbus.h"
#include "plat_i2c.h"
#include "apml.h"
#include "plat_class.h"

LOG_MODULE_REGISTER(plat_sensor_table);

sensor_poll_time_cfg diff_poll_time_sensor_table[] = {
	// sensor_number, last_access_time
	{ SENSOR_NUM_VOL_P3V_BAT, 0 },
};

sensor_cfg plat_sensor_config[] = {
	/* number, type, port, address, offset, access check, arg0, arg1, cache, cache_status,
	   pre_sensor_read_fn, pre_sensor_read_args, post_sensor_read_fn, post_sensor_read_fn,
	   init_arg */

	/* temperature */
	{ SENSOR_NUM_TEMP_TMP75_IN, sensor_dev_tmp75, I2C_BUS2, TMP75_IN_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_TMP75_OUT, sensor_dev_tmp75, I2C_BUS2, TMP75_OUT_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_TMP75_FIO, sensor_dev_tmp75, I2C_BUS2, TMP75_FIO_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_HSC, sensor_dev_nct7718w, I2C_BUS5, TEMP_HSC_ADDR,
	  NCT7718W_REMOTE_TEMP_MSB_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT,
	  POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &nct7718w_init_args[0] },

	/* NVME */
	{ SENSOR_NUM_TEMP_SSD, sensor_dev_nvme, I2C_BUS2, SSD_ADDR, SSD_TEMP_OFFSET, post_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  pre_nvme_read, &mux_conf_addr_0xe2[1], NULL, NULL, NULL },

	/* CPU */

	/* adc voltage */
	{ SENSOR_NUM_VOL_P12V_STBY, sensor_dev_ast_adc, ADC_PORT0, NONE, NONE, stby_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_PVDD18_S5, sensor_dev_ast_adc, ADC_PORT1, NONE, NONE, dc_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P3V3_STBY, sensor_dev_ast_adc, ADC_PORT2, NONE, NONE, stby_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_PVDD11_S3, sensor_dev_ast_adc, ADC_PORT3, NONE, NONE, dc_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P3V_BAT, sensor_dev_ast_adc, ADC_PORT4, NONE, NONE, stby_access, 31, 10,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_BAT3V, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  pre_vol_bat3v_read, NULL, post_vol_bat3v_read, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_PVDD33_S5, sensor_dev_ast_adc, ADC_PORT5, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P5V_STBY, sensor_dev_ast_adc, ADC_PORT14, NONE, NONE, stby_access, 711,
	  200, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P12V_MEM_1, sensor_dev_ast_adc, ADC_PORT13, NONE, NONE, dc_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P12V_MEM_0, sensor_dev_ast_adc, ADC_PORT12, NONE, NONE, dc_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P1V2_STBY, sensor_dev_ast_adc, ADC_PORT10, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P3V3_M2, sensor_dev_ast_adc, ADC_PORT9, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P1V8_STBY, sensor_dev_ast_adc, ADC_PORT8, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &ast_adc_init_args[0] },

	/* VR voltage */

	/* VR current */

	/* VR temperature */

	/* VR power */
};

sensor_cfg adm1278_sensor_config_table[] = {
	{ SENSOR_NUM_VOL_HSCIN, sensor_dev_adm1278, I2C_BUS5, ADM1278_ADDR, PMBUS_READ_VIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &adm1278_init_args[0] },
	{ SENSOR_NUM_CUR_HSCOUT, sensor_dev_adm1278, I2C_BUS5, ADM1278_ADDR, PMBUS_READ_IOUT,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, post_adm1278_cur_read, NULL, &adm1278_init_args[0] },
	{ SENSOR_NUM_PWR_HSCIN, sensor_dev_adm1278, I2C_BUS5, ADM1278_ADDR, PMBUS_READ_PIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, post_adm1278_pwr_read, NULL, &adm1278_init_args[0] },

};

sensor_cfg ltc4282_sensor_config_table[] = {
	{ SENSOR_NUM_VOL_HSCIN, sensor_dev_ltc4282, I2C_BUS5, LTC4282_ADDR, LTC4282_VSOURCE_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &ltc4282_init_args[0] },
	{ SENSOR_NUM_CUR_HSCOUT, sensor_dev_ltc4282, I2C_BUS5, LTC4282_ADDR, LTC4282_VSENSE_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, post_ltc4282_cur_read, NULL, &ltc4282_init_args[0] },
	{ SENSOR_NUM_PWR_HSCIN, sensor_dev_ltc4282, I2C_BUS5, LTC4282_ADDR, LTC4282_POWER_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, post_ltc4282_pwr_read, NULL, &ltc4282_init_args[0] },
};

sensor_cfg mp5990_sensor_config_table[] = {
	{ SENSOR_NUM_VOL_HSCIN, sensor_dev_mp5990, I2C_BUS5, MP5990_ADDR, PMBUS_READ_VIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &mp5990_init_args[0] },
	{ SENSOR_NUM_CUR_HSCOUT, sensor_dev_mp5990, I2C_BUS5, MP5990_ADDR, PMBUS_READ_IOUT,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, post_mp5990_cur_read, NULL, &mp5990_init_args[0] },
	{ SENSOR_NUM_PWR_HSCIN, sensor_dev_mp5990, I2C_BUS5, MP5990_ADDR, PMBUS_READ_PIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, post_mp5990_pwr_read, NULL, &mp5990_init_args[0] },
};

sensor_cfg mp5990_temp_sensor_config_table[] = {
	{ SENSOR_NUM_TEMP_HSC, sensor_dev_mp5990, I2C_BUS5, MP5990_ADDR, PMBUS_READ_TEMPERATURE_1,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &mp5990_init_args[0] },
};

void pal_extend_sensor_config()
{
	uint8_t hsc_module = get_hsc_module();

	/* Determine which HSC module is used */
	switch (hsc_module) {
	case HSC_MODULE_ADM1278:
		LOG_INF("HSC vendor: ADM1278");
		break;
	case HSC_MODULE_LTC4282:
		LOG_INF("HSC vendor: LTC4282");
		break;
	case HSC_MODULE_MP5990:
		LOG_INF("HSC vendor: MP5990");
		break;
	default:
		LOG_ERR("Unsupported HSC module, HSC module: 0x%x", hsc_module);
		break;
	}

	uint8_t board_revision = get_board_revision();
	uint8_t vr_vender = (board_revision & 0x30) >> 4;
	uint8_t board_stage = (board_revision & 0x0F);

	LOG_INF("BOARD config: 0x%x - 0x%x - 0x%x", board_revision, vr_vender, board_stage);

	/* Determine which VR is used */
	switch (vr_vender) {
	case VR_VENDER_INFINEON:
		LOG_INF("VR vendor: INFINEON");
		break;
	case VR_VENDER_MPS:
		LOG_INF("VR vendor: MPS");
		break;
	case VR_VENDER_TI:
		LOG_INF("VR vendor: TI");
		break;
	default:
		LOG_INF("VR vendor: RNS(default)");
		break;
	}

	if (sensor_config_count != sdr_count) {
		LOG_ERR("Extend sensor SDR and config table not match, sdr size: 0x%x, sensor config size: 0x%x",
			sdr_count, sensor_config_count);
	}
}

uint8_t pal_get_extend_sensor_config()
{
	uint8_t extend_sensor_config_size = 0;
	uint8_t hsc_module = get_hsc_module();
	switch (hsc_module) {
	case HSC_MODULE_ADM1278:
		extend_sensor_config_size += ARRAY_SIZE(adm1278_sensor_config_table);
		break;
	case HSC_MODULE_LTC4282:
		extend_sensor_config_size += ARRAY_SIZE(ltc4282_sensor_config_table);
		break;
	case HSC_MODULE_MP5990:
		extend_sensor_config_size += ARRAY_SIZE(mp5990_sensor_config_table);
		break;
	default:
		LOG_ERR("Unsupported HSC module, HSC module: 0x%x", hsc_module);
		break;
	}

	return extend_sensor_config_size;
}

bool pal_is_time_to_poll(uint8_t sensor_num, int poll_time)
{
	int i = 0;
	int table_size = sizeof(diff_poll_time_sensor_table) / sizeof(sensor_poll_time_cfg);

	for (i = 0; i < table_size; i++) {
		if (sensor_num == diff_poll_time_sensor_table[i].sensor_num) {
			int64_t current_access_time = k_uptime_get();
			int64_t last_access_time = diff_poll_time_sensor_table[i].last_access_time;
			int64_t diff_time = (current_access_time - last_access_time) / 1000;
			if ((last_access_time != 0) && (diff_time < poll_time)) {
				return false;
			} else {
				diff_poll_time_sensor_table[i].last_access_time =
					current_access_time;
				return true;
			}
		}
	}

	LOG_ERR("Can't find sensor 0x%x last access time", sensor_num);
	return true;
}

const int SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_sensor_config);

void load_sensor_config(void)
{
	memcpy(sensor_config, plat_sensor_config, SENSOR_CONFIG_SIZE * sizeof(sensor_cfg));
	sensor_config_count = SENSOR_CONFIG_SIZE;

	pal_extend_sensor_config();
}

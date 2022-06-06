#include "plat_sensor_table.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sensor.h"
#include "ast_adc.h"
#include "intel_peci.h"
#include "hal_gpio.h"
#include "plat_class.h"
#include "plat_gpio.h"
#include "plat_hook.h"
#include "plat_i2c.h"
#include "power_status.h"
#include "pmbus.h"
#include "tmp431.h"
#include "libutil.h"

// Scron: Replace A_P3V_BAT_SCALED_EN_R by FM_P3V_BAT_SCALED_EN_R.
SET_GPIO_VALUE_CFG pre_bat_3v = { FM_P3V_BAT_SCALED_EN_R, GPIO_HIGH };
SET_GPIO_VALUE_CFG post_bat_3v = { FM_P3V_BAT_SCALED_EN_R, GPIO_LOW };

sensor_cfg plat_sensor_config[] = {
	/* number,                  type,       port,      address,      offset,
	   access check arg0, arg1, sample_count, cache, cache_status, mux_ADDRess, mux_offset,
	   pre_sensor_read_fn, pre_sensor_read_args, post_sensor_read_fn, post_sensor_read_fn  */

	// TMP
	{ SENSOR_NUM_TEMP_TMP75_IN, sensor_dev_tmp75, I2C_BUS2, TMP75_IN_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_TEMP_TMP75_OUT, sensor_dev_tmp75, I2C_BUS2, TMP75_OUT_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_TEMP_TMP75_IOM, sensor_dev_tmp75, I2C_BUS8, TMP75_IOM_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },

	// NVME, slave address need to be changed
	{ SENSOR_NUM_TEMP_SSD0, sensor_dev_nvme, I2C_BUS2, SSD0_ADDR, SSD0_OFFSET, post_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },

	// PECI, slave address need to be changed
	{ SENSOR_NUM_TEMP_CPU, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR, PECI_TEMP_CPU,
	  post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_TEMP_CPU_MARGIN, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR,
	  PECI_TEMP_CPU_MARGIN, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, post_cpu_margin_read, NULL, NULL },
	{ SENSOR_NUM_TEMP_CPU_TJMAX, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR,
	  PECI_TEMP_CPU_TJMAX, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL,
	  NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_DIMM_A, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR,
	  PECI_TEMP_CHANNEL0_DIMM0, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_DIMM_C, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR,
	  PECI_TEMP_CHANNEL2_DIMM0, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_DIMM_E, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR,
	  PECI_TEMP_CHANNEL4_DIMM0, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_DIMM_G, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR,
	  PECI_TEMP_CHANNEL6_DIMM0, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_CPU, sensor_dev_intel_peci, NONE, CPU_PECI_ADDR, PECI_PWR_CPU, post_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },

	// adc voltage (Sort in the ADC port number order)
	{ SENSOR_NUM_VOL_STBY12V, sensor_dev_ast_adc, ADC_PORT0, NONE, NONE, stby_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_DIMM12V, sensor_dev_ast_adc, ADC_PORT1, NONE, NONE, dc_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_STBY3V, sensor_dev_ast_adc, ADC_PORT2, NONE, NONE, stby_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_STBY1V05, sensor_dev_ast_adc, ADC_PORT3, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_BAT3V, sensor_dev_ast_adc, ADC_PORT4, NONE, NONE, stby_access, 3, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vol_bat3v_read, NULL,
	  post_vol_bat3v_read, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_M2_3V3, sensor_dev_ast_adc, ADC_PORT5, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_STBY5V, sensor_dev_ast_adc, ADC_PORT6, NONE, NONE, stby_access, 711, 200,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_STBY1V8, sensor_dev_ast_adc, ADC_PORT7, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_STBY1V2, sensor_dev_ast_adc, ADC_PORT8, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adc_asd_init_args[0] },

	// VR voltage
	{ SENSOR_NUM_VOL_PVCCD_HV, sensor_dev_isl69259, I2C_BUS5, PVCCD_HV_ADDR, VR_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVCCINFAON, sensor_dev_isl69259, I2C_BUS5, PVCCINFAON_ADDR, VR_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVCCFA_EHV, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_ADDR, VR_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVCCIN, sensor_dev_isl69259, I2C_BUS5, PVCCIN_ADDR, VR_VOL_CMD, vr_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read, &vr_pre_read_args[0],
	  NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVCCFA_EHV_FIVRA, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_FIVRA_ADDR,
	  VR_VOL_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },

	// VR current
	{ SENSOR_NUM_CUR_PVCCD_HV, sensor_dev_isl69259, I2C_BUS5, PVCCD_HV_ADDR, VR_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVCCINFAON, sensor_dev_isl69259, I2C_BUS5, PVCCINFAON_ADDR, VR_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVCCFA_EHV, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_ADDR, VR_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVCCIN, sensor_dev_isl69259, I2C_BUS5, PVCCIN_ADDR, VR_CUR_CMD, vr_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read, &vr_pre_read_args[0],
	  NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVCCFA_EHV_FIVRA, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_FIVRA_ADDR,
	  VR_CUR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },

	// VR temperature
	{ SENSOR_NUM_TEMP_PVCCD_HV, sensor_dev_isl69259, I2C_BUS5, PVCCD_HV_ADDR, VR_TEMP_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVCCINFAON, sensor_dev_isl69259, I2C_BUS5, PVCCINFAON_ADDR, VR_TEMP_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVCCFA_EHV, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_ADDR, VR_TEMP_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVCCIN, sensor_dev_isl69259, I2C_BUS5, PVCCIN_ADDR, VR_TEMP_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVCCFA_EHV_FIVRA, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_FIVRA_ADDR,
	  VR_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },

	// VR power
	{ SENSOR_NUM_PWR_PVCCD_HV, sensor_dev_isl69259, I2C_BUS5, PVCCD_HV_ADDR, VR_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVCCINFAON, sensor_dev_isl69259, I2C_BUS5, PVCCINFAON_ADDR, VR_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVCCFA_EHV, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_ADDR, VR_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVCCIN, sensor_dev_isl69259, I2C_BUS5, PVCCIN_ADDR, VR_PWR_CMD, vr_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read, &vr_pre_read_args[0],
	  NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVCCFA_EHV_FIVRA, sensor_dev_isl69259, I2C_BUS5, PVCCFA_EHV_FIVRA_ADDR,
	  VR_PWR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vr_read,
	  &vr_pre_read_args[1], NULL, NULL, NULL },

	// ME, slave address need to be changed
	{ SENSOR_NUM_TEMP_PCH, sensor_dev_pch, I2C_BUS3, PCH_ADDR, ME_SENSOR_NUM_TEMP_PCH,
	  me_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_DIMMA_PMIC, sensor_dev_pmic, I2C_BUS3, PCH_ADDR, NONE, me_access, 0, 0,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_pmic_read, NULL, NULL, NULL,
	  &pmic_init_args[0] },
	{ SENSOR_NUM_PWR_DIMMC_PMIC, sensor_dev_pmic, I2C_BUS3, PCH_ADDR, NONE, me_access, 0, 0,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_pmic_read, NULL, NULL, NULL,
	  &pmic_init_args[1] },
	{ SENSOR_NUM_PWR_DIMME_PMIC, sensor_dev_pmic, I2C_BUS3, PCH_ADDR, NONE, me_access, 0, 0,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_pmic_read, NULL, NULL, NULL,
	  &pmic_init_args[3] },
	{ SENSOR_NUM_PWR_DIMMG_PMIC, sensor_dev_pmic, I2C_BUS3, PCH_ADDR, NONE, me_access, 0, 0,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_pmic_read, NULL, NULL, NULL,
	  &pmic_init_args[4] },

	// INA230
	{ SENSOR_NUM_PWR_IOM_P12V, sensor_dev_ina230, I2C_BUS8, INA230_ADDR, INA230_PWR_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ina230_init_args[0] },
	{ SENSOR_NUM_CUR_IOM_P12V, sensor_dev_ina230, I2C_BUS8, INA230_ADDR, INA230_CUR_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ina230_init_args[0] },

	// HSC
	{ SENSOR_NUM_TEMP_HSC, sensor_dev_mp5990, I2C_BUS5, MPS_MP5990_ADDR,
	  PMBUS_READ_TEMPERATURE_1, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &mp5990_init_args[0] },
	{ SENSOR_NUM_VOL_HSCIN, sensor_dev_mp5990, I2C_BUS5, MPS_MP5990_ADDR, PMBUS_READ_VIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &mp5990_init_args[0] },
	{ SENSOR_NUM_CUR_HSCOUT, sensor_dev_mp5990, I2C_BUS5, MPS_MP5990_ADDR, PMBUS_READ_IOUT,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &mp5990_init_args[0] },
	{ SENSOR_NUM_PWR_HSCIN, sensor_dev_mp5990, I2C_BUS5, MPS_MP5990_ADDR, PMBUS_READ_PIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &mp5990_init_args[0] },
};

uint8_t load_sensor_config(void)
{
	memcpy(sensor_config, plat_sensor_config, sizeof(plat_sensor_config));
	return ARRAY_SIZE(plat_sensor_config);
}

void check_vr_type(uint8_t index)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	char data = PMBUS_IC_DEVICE_ID;

	/* Get IC Device ID from VR chip
	 * - Command code: 0xAD
	 * - The response data 
	 *   byte-1: Block read count
	 *   byte-2: Device ID
	 * For the ISL69259 chip,
	 * the byte-1 of response data is 4 and the byte-2 to 5 is 49D28100h.
	 * For the TPS53689 chip,
	 * the byte-1 of response data is 6 and the byte-2 to 7 is 544953689000h.
	 * For the XDPE15284 chip,
	 * the byte-1 is returned as 2 and the byte-2 is 8Ah(XDPE15284).
	 */
	uint8_t bus = sensor_config[index].port;
	uint8_t target_addr = sensor_config[index].target_addr;
	uint8_t tx_len = 1;
	uint8_t rx_len = 7;
	msg = construct_i2c_message(bus, target_addr, tx_len, &data, rx_len);

	if (i2c_master_read(&msg, retry)) {
		printf("Failed to read VR register(0x%x)\n", data);
		return;
	}

	if ((msg.data[0] == 0x06) && (msg.data[1] == 0x54) && (msg.data[2] == 0x49) &&
	    (msg.data[3] == 0x53) && (msg.data[4] == 0x68) && (msg.data[5] == 0x90) &&
	    (msg.data[6] == 0x00)) {
		printf("VR type: TPS53689\n");
		sensor_config[index].type = sensor_dev_tps53689;
	} else if ((msg.data[0] == 0x02) && (msg.data[2] == 0x8A)) {
		printf("VR type: XDPE15284\n");
		sensor_config[index].type = sensor_dev_xdpe15284;
	} else if ((msg.data[0] == 0x04) && (msg.data[1] == 0x00) && (msg.data[2] == 0x81) &&
		   (msg.data[3] == 0xD2) && (msg.data[4] == 0x49)) {
		printf("VR type: ISL69259\n");
	} else {
		printf("Unknown VR type\n");
	}
}

void pal_fix_sensor_config()
{
/* mcadd: TODO: should check whether read by i2c or gpio */
#if 0
	uint8_t sensor_count = ARRAY_SIZE(plat_sensor_config);

	/* Check the VR sensor type */
	for (uint8_t index = 0; index < sensor_count; index++) {
		if (sensor_config[index].type == sensor_dev_isl69259) {
			check_vr_type(index);
		}
	}
#endif

	if (sensor_config_num != SDR_NUM) {
		printf("fix sensor SDR and config table not match\n");
	}
}

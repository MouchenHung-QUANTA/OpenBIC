#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "plat_i2c.h"
#include "plat_gpio.h"
#include "plat_hook.h"
#include "plat_sensor_table.h"

#include "i2c-mux-tca9548.h"

#define ADJUST_ADM1278_POWER(x) (x * 0.98)
#define ADJUST_ADM1278_CURRENT(x) ((x * 0.98) + 0.1)

/**************************************************************************************************
 * INIT ARGS
**************************************************************************************************/
adc_asd_init_arg adc_asd_init_args[] = { [0] = { .is_init = false } };

adm1278_init_arg adm1278_init_args[] = {
	[0] = { .is_init = false, .config = { 0x3F1C }, .r_sense = 0.25 }
};
mp5990_init_arg mp5990_init_args[] = {
	[0] = { .is_init = false, .iout_cal_gain = 0x0104, .iout_oc_fault_limit = 0x0028 },
	[1] = { .is_init = false, .iout_cal_gain = 0x01BF, .iout_oc_fault_limit = 0x0046 }
};

ina230_init_arg ina230_init_args[] = {
	[0] = { .is_init = false,
			.config = {
				.MODE = 0b111,		// Measure voltage of shunt resistor and bus(default).
				.VSH_CT = 0b100,	// The Vshunt conversion time is 1.1ms(default).
				.VBUS_CT = 0b100,	// The Vbus conversion time is 1.1ms(default).
				.AVG = 0b000,		// Average number is 1(default).
			},
			.r_shunt = 0.001,
			.i_max = 6.0 },
};

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS
 **************************************************************************************************/
struct tca9548 mux_conf_addr_0xe2[8] = {
	[0] = { .addr = 0xe2, .chan = 0 }, [1] = { .addr = 0xe2, .chan = 1 },
	[2] = { .addr = 0xe2, .chan = 2 }, [3] = { .addr = 0xe2, .chan = 3 },
	[4] = { .addr = 0xe2, .chan = 4 }, [5] = { .addr = 0xe2, .chan = 5 },
	[6] = { .addr = 0xe2, .chan = 6 }, [7] = { .addr = 0xe2, .chan = 7 },
};

vr_pre_proc_arg vr_pre_read_args[] = {
	[0] = { 0x0 },
	[1] = { 0x1 },
};

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
/* VR sensor pre read function
 *
 * set mux and VR page
 *
 * @param sensor_num sensor number
 * @param args pointer to vr_pre_proc_arg
 * @param reading pointer to reading from previous step
 * @retval true if setting mux and page is successful.
 * @retval false if setting mux or page fails.
 */
bool pre_vr_read(uint8_t sensor_num, void *args)
{
	if (args == NULL) {
		return false;
	}

	vr_pre_proc_arg *pre_proc_args = (vr_pre_proc_arg *)args;
	uint8_t retry = 5;
	I2C_MSG msg;

	/* set page */
	msg.bus = sensor_config[sensor_config_index_map[sensor_num]].port;
	msg.target_addr = sensor_config[sensor_config_index_map[sensor_num]].target_addr;
	msg.tx_len = 2;
	msg.data[0] = 0x00;
	msg.data[1] = pre_proc_args->vr_page;
	if (i2c_master_write(&msg, retry)) {
		printf("pre_vr_read, set page fail\n");
		return false;
	}
	return true;
}

/* AST ADC pre read function
 *
 * set gpio high if sensor is "SENSOR_NUM_VOL_BAT3V"
 *
 * @param sensor_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true always.
 * @retval false NULL
 */
bool pre_vol_bat3v_read(uint8_t sensor_num, void *args)
{
	ARG_UNUSED(args);

	// Scron: Replace A_P3V_BAT_SCALED_EN_R by FM_P3V_BAT_SCALED_EN_R.
	if (sensor_num == SENSOR_NUM_VOL_BAT3V) {
		gpio_set(FM_P3V_BAT_SCALED_EN_R, GPIO_HIGH);
		k_msleep(1);
	}

	return true;
}

/* AST ADC post read function
 *
 * set gpio low if sensor is "SENSOR_NUM_VOL_BAT3V"
 *
 * @param sensor_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true always.
 * @retval false NULL
 */
bool post_vol_bat3v_read(uint8_t sensor_num, void *args, int *reading)
{
	ARG_UNUSED(args);
	ARG_UNUSED(reading);

	// Scron: Replace A_P3V_BAT_SCALED_EN_R by FM_P3V_BAT_SCALED_EN_R.
	if (sensor_num == SENSOR_NUM_VOL_BAT3V)
		gpio_set(FM_P3V_BAT_SCALED_EN_R, GPIO_LOW);

	return true;
}

/* INTEL PECI post read function
 *
 * modify certain sensor value after reading
 *
 * @param sensor_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true if no error
 * @retval false if reading get NULL
 */

bool post_cpu_margin_read(uint8_t sensor_num, void *args, int *reading)
{
	if (!reading)
		return false;
	ARG_UNUSED(args);

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = -sval->integer; /* for BMC minus */
	return true;
}

/* ADM1278 post read function
 *
 * modify ADM1278 power value after reading
 *
 * @param sensor_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true if no error
 * @retval false if reading get NULL
 */
bool post_adm1278_power_read(uint8_t sensor_num, void *args, int *reading)
{
	if (!reading)
		return false;
	ARG_UNUSED(args);

	sensor_val *sval = (sensor_val *)reading;
	float val = (float)sval->integer + (sval->fraction / 1000.0);

	val = ADJUST_ADM1278_POWER(val);
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;
	return true;
}

/* ADM1278 post read function
 *
 * modify ADM1278 current value after reading
 *
 * @param sensor_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true if no error
 * @retval false if reading get NULL
 */
bool post_adm1278_current_read(uint8_t sensor_num, void *args, int *reading)
{
	if (!reading)
		return false;
	ARG_UNUSED(args);

	sensor_val *sval = (sensor_val *)reading;
	float val = (float)sval->integer + (sval->fraction / 1000.0);

	val = ADJUST_ADM1278_CURRENT(val);
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;
	return true;
}

#include <stdio.h>
#include <string.h>

#include "sensor.h"
#include "hal_i2c.h"

static double cur_lsb = 0.0;

uint8_t ina230_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	return SENSOR_READ_SUCCESS;
}

uint8_t ina230_init(uint8_t sensor_num)
{
	uint16_t calibration = 0;
	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	ina230_init_arg *init_args;

	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	init_args = (ina230_init_arg *)sensor_config[sensor_config_index_map[sensor_num]].init_args;

	if (init_args->r_shunt <= 0.0 || init_args->i_max <= 0.0) {
		printf("<error> INA230 has invalid initail arguments\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	if (init_args->is_init) {
		goto skip_init;
	}

	/* Calibration = 0.00512 / (cur_lsb * r_shunt)
	 * - 0.00512 : The fixed value in ina230 used to ensure scaling is maintained properly.
	 * - cur_lsb : Maximum Expected Current(i_max) / 2^15.
	 * - r_shunt : Value of the shunt resistor.
	 * Ref: https://www.ti.com/product/INA230
	 */
	cur_lsb = init_args->i_max / 32768.0;
	calibration = (uint16_t)(0.00512 / (cur_lsb * init_args->r_shunt));

	msg.bus = sensor_config[sensor_config_index_map[sensor_num]].port;
	msg.target_addr = sensor_config[sensor_config_index_map[sensor_num]].target_addr;
	msg.tx_len = 3;
	msg.data[0] = INA230_CFG_OFFSET;
	msg.data[1] = init_args->config.value & 0xFF;
	msg.data[2] = (init_args->config.value >> 8) & 0xFF;

	if (i2c_master_write(&msg, retry)) {
		printf("<error> INA230 initial failed while i2c writing\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	memset(msg.data, 0, I2C_BUFF_SIZE);
	msg.data[0] = INA230_CAL_OFFSET;

	if (calibration & 0x8000) {
		// The size of calibration is 16 bits, and the MSB is unused.
		printf("<warning> INA230 the calibration register overflowed (0x%.2X)\n",
		       calibration);
		calibration = calibration & 0x7FFF;
	}

	msg.data[1] = calibration & 0xFF;
	msg.data[2] = (calibration >> 8) & 0xFF;

	if (i2c_master_write(&msg, retry)) {
		printf("<error> INA230 initial failed while i2c writing\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

skip_init:
	sensor_config[sensor_config_index_map[sensor_num]].read = ina230_read;
	return SENSOR_INIT_SUCCESS;
}

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
#include "plat_sdr_table.h"
#include "sdr.h"
#include "plat_ipmb.h"
#include "plat_sensor_table.h"

SDR_Full_sensor plat_sdr_table[] = {
	/* =============================== TEMPARUTURE SENSOR =============================== */
	{
		// MB Inlet temperature
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_TEMP_TMP75_IN, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_TEMPERATURE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_DEGREE_C, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x01, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x3C, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_INLET_TEMP_C",
	},
	{
		// MB Outlet temperature
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_TEMP_TMP75_OUT, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_TEMPERATURE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_DEGREE_C, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x01, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x50, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_OUTLET_TEMP_C",
	},
	{
		// Front IO temperature
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_TEMP_TMP75_FIO, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_TEMPERATURE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_DEGREE_C, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x01, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x28, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"FIO_FRONT_TEMP_C",
	},
	{
		// SSD temperature
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_TEMP_SSD, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_TEMPERATURE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_DEGREE_C, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x01, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x46, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_SSD0_M2A_TEMP_C",
	},
	{
		// HSC temperature
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_TEMP_HSC, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_TEMPERATURE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_DEGREE_C, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x01, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x50, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_HSC_TEMP_C",
	},

	/* =============================== VOLTAGE SENSOR =============================== */
	{
		// ADC0 - P12V_STBY Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC0_P12V_STBY, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x06, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0xEF, // UNRT
		0xDF, // UCT
		0xDC, // UNCT
		0xA8, // LNRT
		0xB2, // LCT
		0xB4, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P12V_STBY_VOLT_V",
	},
	{
		// ADC1 - SOC_RC_DDR0 Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC1_SOC_RC_DDR0, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x00, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x00, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_SOC_RC_DDR0_VOLT_V",
	},
	{
		// P3V3_STBY Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC2_P3V3_STBY, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x19, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xD0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x8B, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x7D, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P3V3_STBY_VOLT_V",
	},
	{
		// ADC3 - PCP Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC3_P0V75_PCP, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x00, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x00, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P0V75_PCP_VOLT_V",
	},
	{
		// ADC4 - P3V_BAT Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC4_P3V_BAT, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x37, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xD0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x42, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x31, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P3V_BAT_VOLT_V",
	},
	{
		// ADC5 - P0V8_D2D Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC5_P0V8_D2D, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x00, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x00, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P0V8_D2D_VOLT_V",
	},
	{
		// ADC8 - EXT_VREF_ADC_S0 Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC8_EXT_VREF_ADC_S0, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x00, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x00, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P0V8_D2D_VOLT_V",
	},
	{
		// ADC9 - P3V3_M2 Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC9_P3V3_M2, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x03, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x7B, // UCT
		0x7A, // UNCT
		0x00, // LNRT
		0x61, // LCT
		0x62, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P3V3_M2_VOLT_V",
	},
	{
		// ADC10 - P1V2_STBY Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC10_P1V2_STBY, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x01, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x7F, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x71, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P1V2_STBY_VOLT_V",
	},
	{
		// ADC11 - SOC_RC_DDR1 Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC11_SOC_RC_DDR1, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x00, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x00, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_SOC_RC_DDR1_VOLT_V",
	},
	{
		// ADC12 - P12V_MEM_0 Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC12_P12V_S0_DIMM0, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x06, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0xEF, // UNRT
		0xDF, // UCT
		0xDC, // UNCT
		0xA8, // LNRT
		0xB2, // LCT
		0xB4, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P12V_MEM_0_VOLT_V",
	},
	{
		// ADC13 - P12V_MEM_1 Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC13_P12V_S0_DIMM1, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x06, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0xEF, // UNRT
		0xDF, // UCT
		0xDC, // UNCT
		0xA8, // LNRT
		0xB2, // LCT
		0xB4, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P12V_MEM_1_VOLT_V",
	},
	{
		// ADC14 - P5V_STBY Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_ADC14_P5V_STBY, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x41, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xD0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x00, // UNRT
		0x51, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x49, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_ADC_P5V_STBY_VOLT_V",
	},
	{
		// HSC Input Voltage
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_VOL_HSCIN, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_VOLTAGE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_VOL, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x06, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0xEF, // UNRT
		0xDF, // UCT
		0xDC, // UNCT
		0xA8, // LNRT
		0xB2, // LCT
		0xB4, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_HSC_INPUT_VOLT_V",
	},

	/* =============================== CURRENT SENSOR =============================== */
	{
		// HSC Output Current
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_CUR_HSCOUT, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_CURRENT, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_AMP, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x8B, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0xE0, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x24, // UNRT
		0x21, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"HSC_OUTPUT_CURR_A",
	},

	/* =============================== POWER SENSOR =============================== */
	{
		// HSC Input Power
		0x00,
		0x00, // record ID
		IPMI_SDR_VER_15, // SDR ver
		IPMI_SDR_FULL_SENSOR, // record type
		IPMI_SDR_FULL_SENSOR_MIN_LEN, // size of struct

		SELF_I2C_ADDRESS << 1, // owner id
		0x00, // owner lun
		SENSOR_NUM_PWR_HSCIN, // sensor number

		IPMI_SDR_ENTITY_ID_SYS_BOARD, // entity id
		0x00, // entity instance
		IPMI_SDR_SENSOR_INIT_SCAN | IPMI_SDR_SENSOR_INIT_EVENT |
			IPMI_SDR_SENSOR_INIT_THRESHOLD | IPMI_SDR_SENSOR_INIT_TYPE |
			IPMI_SDR_SENSOR_INIT_DEF_EVENT |
			IPMI_SDR_SENSOR_INIT_DEF_SCAN, // sensor init
		IPMI_SDR_SENSOR_CAP_THRESHOLD_RW |
			IPMI_SDR_SENSOR_CAP_EVENT_CTRL_NO, // sensor capabilities
		IPMI_SDR_SENSOR_TYPE_OTHER_UNIT_BASE, // sensor type
		IPMI_SDR_EVENT_TYPE_THRESHOLD, // event/reading type
		0x00, // assert event mask
		IPMI_SDR_CMP_RETURN_LCT | IPMI_SDR_ASSERT_MASK_UCT_HI |
			IPMI_SDR_ASSERT_MASK_LCT_LO, // assert threshold reading mask
		0x00, // deassert event mask
		IPMI_SDR_CMP_RETURN_UCT | IPMI_SDR_DEASSERT_MASK_UCT_LO |
			IPMI_SDR_DEASSERT_MASK_LCT_HI, // deassert threshold reading mask
		0x00, // discrete reading mask/ settable
		IPMI_SDR_UCT_SETTABLE | IPMI_SDR_LCT_SETTABLE | IPMI_SDR_UCT_READABLE |
			IPMI_SDR_LCT_READABLE, // threshold mask/ readable threshold mask
		0x00, // sensor unit
		IPMI_SENSOR_UNIT_WATT, // base unit
		0x00, // modifier unit
		IPMI_SDR_LINEAR_LINEAR, // linearization
		0x32, // [7:0] M bits
		0x00, // [9:8] M bits, tolerance
		0x00, // [7:0] B bits
		0x00, // [9:8] B bits, tolerance
		0x00, // [7:4] accuracy , [3:2] accuracy exp, [1:0] sensor direction
		0x00, // Rexp, Bexp
		0x00, // analog characteristic
		0x00, // nominal reading
		0x00, // normal maximum
		0x00, // normal minimum
		0x00, // sensor maximum reading
		0x00, // sensor minimum reading
		0x0C, // UNRT
		0x0B, // UCT
		0x00, // UNCT
		0x00, // LNRT
		0x00, // LCT
		0x00, // LNCT
		0x00, // positive-going threshold
		0x00, // negative-going threshold
		0x00, // reserved
		0x00, // reserved
		0x00, // OEM
		IPMI_SDR_STRING_TYPE_ASCII_8, // ID len, should be same as "size of struct"
		"MB_HSC_INPUT_PWR_W",
	},
};

const int SDR_TABLE_SIZE = ARRAY_SIZE(plat_sdr_table);

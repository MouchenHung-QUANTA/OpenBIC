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

#ifndef PLAT_SENSOR_TABLE_H
#define PLAT_SENSOR_TABLE_H

#include <stdint.h>
#include "sensor.h"
#include "plat_def.h"

/* SENSOR POLLING TIME(second) */
#define POLL_TIME_BAT3V 3600

/* SENSOR ADDRESS(7-bit)/OFFSET */
#define E1S_SSD_ADDR (0xD4 >> 1)
#define FPGA_ADDR (0xC0 >> 1)
#define SATMC_ADDR 0xFF //TODO: Define satmc addr
#define TEMP_HSC_ADDR (0x98 >> 1)
#define MP5990_ADDR (0xA0 >> 1)
#define INA230_ADDR (0x8A >> 1)
#define INA3221_ADDR (0x80 >> 1)

#ifdef NO_FPGA
#define TMP451_ADDR (0x98 >> 1)
#define TMP75_ADDR (0x90 >> 1)
#endif

/* SENSOR OFFSET */
#define SSD_TEMP_OFFSET 0x00

#ifdef NO_FPGA
#define TMP75_TEMP_OFFSET 0x00
#endif

/*  threshold sensor number, 1 based - temperature */
#define SENSOR_NUM_TEMP_TMP451_IN 0x1
#define SENSOR_NUM_TEMP_TMP451_OUT 0x2
#define SENSOR_NUM_TEMP_TMP75_FIO 0x3
#define SENSOR_NUM_TEMP_CPU 0x4
#define SENSOR_NUM_TEMP_FPGA 0x5
#define SENSOR_NUM_TEMP_E1S_SSD 0x6
#define SENSOR_NUM_TEMP_HSC 0x7

/* SENSOR NUMBER(1 based) - voltage */
#define SENSOR_NUM_VOL_HSCIN 0x10
#define SENSOR_NUM_VOL_ADC0_P12V_STBY 0x11
#define SENSOR_NUM_VOL_ADC1_VDD_1V8 0x12
#define SENSOR_NUM_VOL_ADC2_P3V3_STBY 0x13
#define SENSOR_NUM_VOL_ADC3_SOCVDD 0x14
#define SENSOR_NUM_VOL_ADC4_P3V_BAT 0x15
#define SENSOR_NUM_VOL_ADC5_CPUVDD 0x16
#define SENSOR_NUM_VOL_ADC6_FPGA_VCC_AO 0x17
#define SENSOR_NUM_VOL_ADC7_1V2 0x18
#define SENSOR_NUM_VOL_ADC9_VDD_M2 0x19
#define SENSOR_NUM_VOL_ADC10_P1V2_STBY 0x1A
#define SENSOR_NUM_VOL_ADC11_FBVDDQ 0x1B
#define SENSOR_NUM_VOL_ADC12_FBVDDP2 0x1C
#define SENSOR_NUM_VOL_ADC13_FBVDD1 0x1D
#define SENSOR_NUM_VOL_ADC14_P5V_STBY 0x1E
#define SENSOR_NUM_VOL_ADC15_CPU_DVDD 0x1F
#define SENSOR_NUM_VOL_E1S 0x20

/* SENSOR NUMBER(1 based) - current */
#define SENSOR_NUM_CUR_HSCOUT 0x25
#define SENSOR_NUM_CUR_E1S 0x26

/* SENSOR NUMBER(1 based) - power */
#define SENSOR_NUM_PWR_CPU 0x30
#define SENSOR_NUM_PWR_HSCIN 0x31
#define SENSOR_NUM_PWR_E1S 0x32

uint8_t plat_get_config_size();
void load_sensor_config(void);

#endif

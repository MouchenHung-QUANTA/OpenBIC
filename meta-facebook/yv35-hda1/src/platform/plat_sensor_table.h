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

/* SENSOR POLLING TIME(second) */
#define POLL_TIME_BAT3V 3600

/* SENSOR ADDRESS(7-bit)/OFFSET */
#define TMP75_IN_ADDR (0x94 >> 1)
#define TMP75_OUT_ADDR (0x92 >> 1)
#define TMP75_FIO_ADDR (0x90 >> 1)
#define SSD_ADDR (0xD4 >> 1)
#define MPRO_ADDR (0x9E >> 1)

#define ADM1278_ADDR (0x80 >> 1)
#define LTC4282_ADDR (0x82 >> 1)
#define TEMP_HSC_ADDR (0x98 >> 1)
#define MP5990_ADDR (0xA0 >> 1)

/* SENSOR OFFSET */
#define TMP75_TEMP_OFFSET 0x00
#define SSD_TEMP_OFFSET 0x00

/* SENSOR NUMBER(1 based) - temperature */
#define SENSOR_NUM_TEMP_TMP75_IN 0x1
#define SENSOR_NUM_TEMP_TMP75_OUT 0x2
#define SENSOR_NUM_TEMP_TMP75_FIO 0x3
#define SENSOR_NUM_TEMP_CPU 0x4
#define SENSOR_NUM_TEMP_SSD 0x5
#define SENSOR_NUM_TEMP_HSC 0x6

#define SENSOR_NUM_TEMP_DIMM_CH0 0x7
#define SENSOR_NUM_TEMP_DIMM_CH1 0x8
#define SENSOR_NUM_TEMP_DIMM_CH2 0x9
#define SENSOR_NUM_TEMP_DIMM_CH3 0xA
#define SENSOR_NUM_TEMP_DIMM_CH4 0xB
#define SENSOR_NUM_TEMP_DIMM_CH5 0xC
#define SENSOR_NUM_TEMP_DIMM_CH6 0xD
#define SENSOR_NUM_TEMP_DIMM_CH7 0xE

#define SENSOR_NUM_TEMP_PCP_VR 0xF //AMP Mpro spec #71 VRD0
#define SENSOR_NUM_TEMP_SOC_VR 0x10 //AMP Mpro spec #76 VRD1
#define SENSOR_NUM_TEMP_VDDQ_DDR0123_VR 0x11 //AMP Mpro spec #81 VRD2
#define SENSOR_NUM_TEMP_VDDQ_DDR4567_VR 0x12 //AMP Mpro spec #86 VRD3
#define SENSOR_NUM_TEMP_D2D_VR 0x13 //AMP Mpro spec #91 VRD4
#define SENSOR_NUM_TEMP_RC_DDR0_VR 0x14 //AMP Mpro spec #96 VRD5
#define SENSOR_NUM_TEMP_RC_DDR1_VR 0x15 //AMP Mpro spec #101 VRD6
#define SENSOR_NUM_TEMP_PCI_D_VR 0x16 //AMP Mpro spec #106 VRD7
#define SENSOR_NUM_TEMP_PCI_A_VR 0x17 //AMP Mpro spec #111 VRD8

/* SENSOR NUMBER(1 based) - voltage */
#define SENSOR_NUM_VOL_HSCIN 0x21

#define SENSOR_NUM_VOL_ADC0_P12V_STBY 0x22
#define SENSOR_NUM_VOL_ADC1_SOC_RC_DDR0 0x23
#define SENSOR_NUM_VOL_ADC2_P3V3_STBY 0x24
#define SENSOR_NUM_VOL_ADC3_P0V75_PCP 0x25
#define SENSOR_NUM_VOL_ADC4_P3V_BAT 0x26
#define SENSOR_NUM_VOL_ADC5_P0V8_D2D 0x27
#define SENSOR_NUM_VOL_ADC8_EXT_VREF_ADC_S0 0x28
#define SENSOR_NUM_VOL_ADC9_P3V3_M2 0x29
#define SENSOR_NUM_VOL_ADC10_P1V2_STBY 0x2A
#define SENSOR_NUM_VOL_ADC11_SOC_RC_DDR1 0x2B
#define SENSOR_NUM_VOL_ADC12_P12V_S0_DIMM0 0x2C
#define SENSOR_NUM_VOL_ADC13_P12V_S0_DIMM1 0x2D
#define SENSOR_NUM_VOL_ADC14_P5V_STBY 0x2E

#define SENSOR_NUM_VOL_PCP_VR 0x2F //AMP Mpro spec #73 VRD0
#define SENSOR_NUM_VOL_SOC_VR 0x30 //AMP Mpro spec #78 VRD1
#define SENSOR_NUM_VOL_VDDQ_DDR0123_VR 0x31 //AMP Mpro spec #83 VRD2
#define SENSOR_NUM_VOL_VDDQ_DDR4567_VR 0x32 //AMP Mpro #88 VRD3
#define SENSOR_NUM_VOL_D2D_VR 0x33 //AMP Mpro #93 VRD4
#define SENSOR_NUM_VOL_RC_DDR0_VR 0x34 //AMP Mpro #98 VRD5
#define SENSOR_NUM_VOL_RC_DDR1_VR 0x35 //AMP Mpro #103 VRD6
#define SENSOR_NUM_VOL_PCI_D_VR 0x36 //AMP Mpro #108 VRD7
#define SENSOR_NUM_VOL_PCI_A_VR 0x37 //AMP Mpro #113 VRD8

/* SENSOR NUMBER(1 based) - current */
#define SENSOR_NUM_CUR_HSCOUT 0x41
#define SENSOR_NUM_CUR_PCP_VR 0x42 //AMP Mpro spec #74 VRD0
#define SENSOR_NUM_CUR_SOC_VR 0x43 //AMP Mpro spec #79 VRD1
#define SENSOR_NUM_CUR_VDDQ_DDR0123_VR 0x44 //AMP Mpro spec #84 VRD2
#define SENSOR_NUM_CUR_VDDQ_DDR4567_VR 0x45 //AMP Mpro #89 VRD3
#define SENSOR_NUM_CUR_D2D_VR 0x46 //AMP Mpro #94 VRD4
#define SENSOR_NUM_CUR_RC_DDR0_VR 0x47 //AMP Mpro #99 VRD5
#define SENSOR_NUM_CUR_RC_DDR1_VR 0x48 //AMP Mpro #104 VRD6
#define SENSOR_NUM_CUR_PCI_D_VR 0x49 //AMP Mpro #109 VRD7
#define SENSOR_NUM_CUR_PCI_A_VR 0x4A //AMP Mpro #114 VRD8

/* SENSOR NUMBER(1 based) - power */
#define SENSOR_NUM_PWR_CPU 0x61
#define SENSOR_NUM_PWR_HSCIN 0x62

#define SENSOR_NUM_PWR_DIMM_CH0 0x63
#define SENSOR_NUM_PWR_DIMM_CH1 0x64
#define SENSOR_NUM_PWR_DIMM_CH2 0x65
#define SENSOR_NUM_PWR_DIMM_CH3 0x66
#define SENSOR_NUM_PWR_DIMM_CH4 0x67
#define SENSOR_NUM_PWR_DIMM_CH5 0x68
#define SENSOR_NUM_PWR_DIMM_CH6 0x69
#define SENSOR_NUM_PWR_DIMM_CH7 0x6A

#define SENSOR_NUM_PWR_PCP_VR 0x6B //AMP Mpro spec #72 VRD0
#define SENSOR_NUM_PWR_SOC_VR 0x6C //AMP Mpro spec #77 VRD1
#define SENSOR_NUM_PWR_VDDQ_DDR0123_VR 0x6D //AMP Mpro spec #82 VRD2
#define SENSOR_NUM_PWR_VDDQ_DDR4567_VR 0x6E //AMP Mpro #87 VRD3
#define SENSOR_NUM_PWR_D2D_VR 0x6F //AMP Mpro #92 VRD4
#define SENSOR_NUM_PWR_RC_DDR0_VR 0x70 //AMP Mpro #97 VRD5
#define SENSOR_NUM_PWR_RC_DDR1_VR 0x71 //AMP Mpro #102 VRD6
#define SENSOR_NUM_PWR_PCI_D_VR 0x72 //AMP Mpro #107 VRD7
#define SENSOR_NUM_PWR_PCI_A_VR 0x73 //AMP Mpro #112 VRD8

/* SENSOR NUMBER - sel */
#define SENSOR_NUM_SYSTEM_STATUS 0x10
#define SENSOR_NUM_POWER_ERROR 0x56
#define SENSOR_NUM_PROC_FAIL 0x65
#define SENSOR_NUM_VR_OCP 0x71
#define SENSOR_NUM_PMIC_ERROR 0xB4

/* MPRO SENSOR NUMBER MAPPING */
struct plat_mpro_sensor_mapping {
    uint8_t sensor_num;
    uint16_t mpro_sensor_num;
};

extern struct plat_mpro_sensor_mapping mpro_sensor_map[];

uint8_t plat_get_config_size();
void load_sensor_config(void);

#endif

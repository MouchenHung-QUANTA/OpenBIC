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

#ifndef MPRO_H
#define MPRO_H

#include <stdint.h>

enum mpro_sensor_num_table {
	MPRO_SENSOR_NUM_TMP_VRD0 = 71,
	MPRO_SENSOR_NUM_PWR_VRD0,
	MPRO_SENSOR_NUM_VOL_VRD0,
	MPRO_SENSOR_NUM_CUR_VRD0,
	MPRO_SENSOR_NUM_STA_VRD0,

	MPRO_SENSOR_NUM_TMP_VRD1,
	MPRO_SENSOR_NUM_PWR_VRD1,
	MPRO_SENSOR_NUM_VOL_VRD1,
	MPRO_SENSOR_NUM_CUR_VRD1,
	MPRO_SENSOR_NUM_STA_VRD1,

	MPRO_SENSOR_NUM_TMP_VRD2,
	MPRO_SENSOR_NUM_PWR_VRD2,
	MPRO_SENSOR_NUM_VOL_VRD2,
	MPRO_SENSOR_NUM_CUR_VRD2,
	MPRO_SENSOR_NUM_STA_VRD2,

	MPRO_SENSOR_NUM_TMP_VRD3,
	MPRO_SENSOR_NUM_PWR_VRD3,
	MPRO_SENSOR_NUM_VOL_VRD3,
	MPRO_SENSOR_NUM_CUR_VRD3,
	MPRO_SENSOR_NUM_STA_VRD3,

	MPRO_SENSOR_NUM_TMP_VRD4,
	MPRO_SENSOR_NUM_PWR_VRD4,
	MPRO_SENSOR_NUM_VOL_VRD4,
	MPRO_SENSOR_NUM_CUR_VRD4,
	MPRO_SENSOR_NUM_STA_VRD4,

	MPRO_SENSOR_NUM_TMP_VRD5,
	MPRO_SENSOR_NUM_PWR_VRD5,
	MPRO_SENSOR_NUM_VOL_VRD5,
	MPRO_SENSOR_NUM_CUR_VRD5,
	MPRO_SENSOR_NUM_STA_VRD5,

	MPRO_SENSOR_NUM_TMP_VRD6,
	MPRO_SENSOR_NUM_PWR_VRD6,
	MPRO_SENSOR_NUM_VOL_VRD6,
	MPRO_SENSOR_NUM_CUR_VRD6,
	MPRO_SENSOR_NUM_STA_VRD6,

	MPRO_SENSOR_NUM_TMP_VRD7,
	MPRO_SENSOR_NUM_PWR_VRD7,
	MPRO_SENSOR_NUM_VOL_VRD7,
	MPRO_SENSOR_NUM_CUR_VRD7,
	MPRO_SENSOR_NUM_STA_VRD7,

	MPRO_SENSOR_NUM_TMP_VRD8,
	MPRO_SENSOR_NUM_PWR_VRD8,
	MPRO_SENSOR_NUM_VOL_VRD8,
	MPRO_SENSOR_NUM_CUR_VRD8,
	MPRO_SENSOR_NUM_STA_VRD8,
};

uint16_t copy_mpro_read_buffer(uint16_t start, uint16_t length, uint8_t *buffer,
			       uint16_t buffer_len);
void mpro_postcode_read_init();
void mpro_postcode_insert(uint32_t postcode);
void reset_mpro_postcode_buffer();
bool get_4byte_postcode_ok();
void reset_4byte_postcode_ok();

#endif

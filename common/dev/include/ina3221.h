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

#ifndef __INA3221__
#define __INA3221__

#include <stdint.h>
#include "sensor.h"

enum INA3221_OFFSET {
	INA3221_CFG_OFFSET = 0x00,
	INA3221_CH1_VSH_VOL_OFFSET = 0x01,
	INA3221_CH1_BUS_VOL_OFFSET = 0x02,
	INA3221_CH2_VSH_VOL_OFFSET = 0x03,
	INA3221_CH2_BUS_VOL_OFFSET = 0x04,
	INA3221_CH3_VSH_VOL_OFFSET = 0x05,
	INA3221_CH3_BUS_VOL_OFFSET = 0x06,
	INA3221_CH1_CRIT_ALT_OFFSET = 0x07,
	INA3221_CH1_WARN_ALT_OFFSET = 0x08,
	INA3221_CH2_CRIT_ALT_OFFSET = 0x09,
	INA3221_CH2_WARN_ALT_OFFSET = 0x0A,
	INA3221_CH3_CRIT_ALT_OFFSET = 0x0B,
	INA3221_CH3_WARN_ALT_OFFSET = 0x0C,
	INA3221_VSH_SUM_OFFSET = 0x0D,
	INA3221_VSH_SUM_LIMIT_OFFSET = 0x0E,
	INA3221_MSK_OFFSET = 0x0F,
	INA3221_PWR_VLD_UP_LIMIT_OFFSET = 0x10,
	INA3221_PWR_VLD_LO_LIMIT_OFFSET = 0x11,
};

#endif

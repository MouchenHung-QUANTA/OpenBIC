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

#include <string.h>
#include <logging/log.h>
#include "plat_gpio.h"
#include "plat_power_status.h"
#include "libutil.h"

LOG_MODULE_REGISTER(plat_power_status);

static bool is_satmc_ready = false;
void set_satmc_status()
{
	//is_satmc_ready = (gpio_get(S0_BMC_GPIOA5_FW_BOOT_OK) == 1) ? true : false;
	LOG_WRN("SatMC_STATUS: %s", (is_satmc_ready) ? "on" : "off");
}

bool get_satmc_status()
{
	return is_satmc_ready;
}

bool satmc_access(uint8_t sensor_num)
{
	return get_satmc_status();
}

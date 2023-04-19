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

#include "hal_gpio.h"
#include "power_status.h"
#include "util_sys.h"
#include "plat_gpio.h"
#include "plat_class.h"
#include "snoop.h"
#include "pcc.h"
#include "plat_i2c.h"
#include "plat_pmic.h"
#include "plat_apml.h"
#include "plat_kcs.h"
#include "rg3mxxb12.h"
#include "util_worker.h"

SCU_CFG scu_cfg[] = {
	//register    value
	{ 0x7e6e2610, 0xffffffff },
	{ 0x7e6e2618, 0xdc000000 },
	{ 0x7e6e261c, 0x00000F32 },
};

void pal_device_init()
{
	start_monitor_pmic_error_thread();
}

void pal_pre_init()
{
	init_platform_config();

	scu_init(scu_cfg, sizeof(scu_cfg) / sizeof(SCU_CFG));
	init_plat_worker(CONFIG_MAIN_THREAD_PRIORITY + 1); // work queue for low priority jobs
}

void pal_post_init()
{
	kcs_init();
}

void pal_set_sys_status()
{
	set_DC_status(BMC_GPIOL1_SYS_PWRGD);
	set_DC_on_delayed_status();
	set_post_status(FM_BIOS_POST_CMPLT_BIC_N);
	if (get_post_status()) {
		set_tsi_threshold();
		read_cpuid();
	}
	set_sys_ready_pin(BMC_GPIOD4_SW_HBLED);
}

#define DEF_PROJ_GPIO_PRIORITY 78

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME", &gpio_init, NULL, NULL, NULL,
	      POST_KERNEL, DEF_PROJ_GPIO_PRIORITY, NULL);

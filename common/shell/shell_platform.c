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

#include "commands/gpio_shell.h"
#include "commands/info_shell.h"
#include "commands/sensor_shell.h"
#include "commands/flash_shell.h"
#include "commands/ipmi_shell.h"
#include "commands/power_shell.h"

#include "plat_mctp.h"

int cmd_thread_ctl(const struct shell *shell, size_t argc, char **argv)
{
	static uint8_t cnt = 0;
	if (cnt % 2) {
		stop_flag = 0;
	} else {
		stop_flag = t1_en | t2_en | t3_en;
	}
	cnt++;

	return 0;
}

int cmd_thread_ctl1(const struct shell *shell, size_t argc, char **argv)
{
	static uint8_t cnt = 0;
	if (cnt % 2) {
		stop_flag = stop_flag & (t2_en | t3_en) ;
	} else {
		stop_flag |= t1_en;
	}
	cnt++;

	return 0;
}

int cmd_thread_ctl2(const struct shell *shell, size_t argc, char **argv)
{
	static uint8_t cnt = 0;
	if (cnt % 2) {
		stop_flag = stop_flag & (t1_en | t3_en);
	} else {
		stop_flag |= t2_en;
	}
	cnt++;

	return 0;
}

int cmd_thread_ctl3(const struct shell *shell, size_t argc, char **argv)
{
	static uint8_t cnt = 0;
	if (cnt % 2) {
		stop_flag = stop_flag & (t1_en | t2_en);
	} else {
		stop_flag |= t3_en;
	}
	cnt++;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_stress_cmds,
	SHELL_CMD(t0, NULL, "Thread all", cmd_thread_ctl),
	SHELL_CMD(t1, NULL, "Thread1", cmd_thread_ctl1),
	SHELL_CMD(t2, NULL, "Thread2", cmd_thread_ctl2),
	SHELL_CMD(t3, NULL, "Thread3", cmd_thread_ctl3),
	SHELL_SUBCMD_SET_END);

/* MAIN command */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_platform_cmds, SHELL_CMD(info, NULL, "Platform info.", cmd_info_print),
	SHELL_CMD(gpio, &sub_gpio_cmds, "GPIO relative command.", NULL),
	SHELL_CMD(sensor, &sub_sensor_cmds, "SENSOR relative command.", NULL),
	SHELL_CMD(flash, &sub_flash_cmds, "FLASH(spi) relative command.", NULL),
	SHELL_CMD(ipmi, &sub_ipmi_cmds, "IPMI relative command.", NULL),
	SHELL_CMD(stress, &sub_stress_cmds, "dd.", NULL),
	SHELL_CMD(power, &sub_power_cmds, "POWER relative command.", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(platform, &sub_platform_cmds, "Platform commands", NULL);

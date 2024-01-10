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

#include "info_shell.h"
#include "plat_version.h"
#include "util_sys.h"
#include "sbmr.h"
#include "nvidia.h"
#include "sensor.h"

#ifndef CONFIG_BOARD
#define CONFIG_BOARD "unknown"
#endif

#define RTOS_TYPE "Zephyr"

int cmd_info_print(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
	shell_print(shell, "* NAME:          Platform command");
	shell_print(shell, "* DESCRIPTION:   Commands that could be used to debug or validate.");
	shell_print(shell, "* DATE/VERSION:  none");
	shell_print(shell, "* CHIP/OS:       %s - %s", CONFIG_BOARD, RTOS_TYPE);
	shell_print(shell, "* Note:          none");
	shell_print(shell, "------------------------------------------------------------------");
	shell_print(shell, "* PLATFORM:      %s-%s", PLATFORM_NAME, PROJECT_NAME);
	shell_print(shell, "* BOARD ID:      %d", BOARD_ID);
	shell_print(shell, "* STAGE:         %d", PROJECT_STAGE);
	shell_print(shell, "* SYSTEM:        %d", get_system_class());
	shell_print(shell, "* FW VERSION:    %d.%d", FIRMWARE_REVISION_1, FIRMWARE_REVISION_2);
	shell_print(shell, "* FW DATE:       %x%x.%x.%x", BIC_FW_YEAR_MSB, BIC_FW_YEAR_LSB,
		    BIC_FW_WEEK, BIC_FW_VER);
	shell_print(shell, "* FW IMAGE:      %s.bin", CONFIG_KERNEL_BIN_NAME);
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
	
	uint32_t val = 0;
	if (nv_smbpbi_access(0x0d, NV_GPU1, 0x02, 0x00, 0x00, &val) == false ) {
		return 0;
	}

	k_msleep(1000);

	sensor_val sval = {0};
	sval.integer = (int16_t)(val >> 8);
	sval.fraction = 0;
	shell_print(shell, "---> 0x%08x (%d.%d)", val, sval.integer, sval.fraction);

	val = 0;
	if (nv_smbpbi_access(0x0d, NV_GPU1, 0x03, 0x00, 0x00, &val) == false ) {
		return 0;
	}

	k_msleep(1000);

	sval.integer = (int16_t)(val >> 8);
	sval.fraction = (int16_t)(val & 0xFF);;
	shell_print(shell, "---> 0x%08x (%d.%d)", val, sval.integer, sval.fraction);
	return 0;
}

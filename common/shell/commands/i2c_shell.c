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

#include "i2c_shell.h"
#include <stdio.h>
#include <zephyr.h>
#include <string.h>
#include "hal_i2c_target.h"

void cmd_i2c_target_status(const struct shell *shell, size_t argc, char **argv)
{
	for (int bus_idx = 0; bus_idx < MAX_TARGET_NUM; bus_idx++) {
		if (i2c_target_status_print(bus_idx)) {
			shell_print(shell, "Get i2c %d target failed!", bus_idx);
		}
	}

	return;
}

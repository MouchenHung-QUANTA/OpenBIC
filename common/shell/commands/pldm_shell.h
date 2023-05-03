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

#ifndef PLDM_SHELL_H
#define PLDM_SHELL_H

#include <shell/shell.h>

void cmd_pldm_send_req(const struct shell *shell, size_t argc, char **argv);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pldm_cmds,
			       SHELL_CMD(sendreq, NULL, "Send out PLDM request.",
					 cmd_pldm_send_req),
			       SHELL_SUBCMD_SET_END);

#endif

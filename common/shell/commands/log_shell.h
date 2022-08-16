#ifndef LOG_SHELL_H
#define LOG_SHELL_H

#include "log_util.h"
#include "shell_platform.h"
#include <stdlib.h>
#include <shell/shell.h>

void cmd_log_list_all(const struct shell *shell, size_t argc, char **argv);
void cmd_log_control(const struct shell *shell, size_t argc, char **argv);
void cmd_log_halt(const struct shell *shell, size_t argc, char **argv);

/* Log sub command */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_log_cmds, SHELL_CMD(list_all, NULL, "List all debug log status.", cmd_log_list_all),
	SHELL_CMD(control, NULL, "Enable/Disable certain debug log.", cmd_log_control),
	SHELL_CMD(halt, NULL, "Disable all debug log.", cmd_log_halt), SHELL_SUBCMD_SET_END);

#endif

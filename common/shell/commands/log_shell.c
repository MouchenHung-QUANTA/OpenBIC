#include "log_shell.h"
#include <stdio.h>

/*
    Command LOG
*/
void cmd_log_list_all(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "Help: platform log list_all");
		return;
	}

	shell_print(shell, "--------------------------");
	for (int i = 0; i < DEBUG_MAX; i++) {
		char *log_status = (is_log_en(i) == LOG_ENABLE) ? "o" : "x";
		shell_print(shell, "[%-2d] %-15s: %s", i, log_name[i], log_status);
	}
	shell_print(shell, "--------------------------");

	return;
}

void cmd_log_control(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: platform log control <log_idx> <log_status>");
		return;
	}

	uint8_t log_idx = strtol(argv[1], NULL, 10);
	uint8_t log_status = strtol(argv[2], NULL, 10);

	if (log_idx >= DEBUG_MAX) {
		shell_error(shell, "Invalid <log_idx>, should lower than %d.", log_idx);
		return;
	}

	if (log_status != LOG_ENABLE && log_status != LOG_DISABLE) {
		shell_error(shell, "Invalid <log_status>, try 0:enable / 1:disable.");
		return;
	}

	if (!log_status_ctl(log_idx, log_status)) {
		shell_error(shell, "Log %d status set %d failed!", log_idx, log_status);
		return;
	}

	shell_print(shell, "Log %d status set %d success!", log_idx, log_status);
	return;
}

void cmd_log_halt(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "Help: platform log halt");
		return;
	}

	for (int i = 0; i < DEBUG_MAX; i++) {
		log_status_ctl(i, LOG_DISABLE);
	}

	shell_print(shell, "All log has been halt!");
	return;
}

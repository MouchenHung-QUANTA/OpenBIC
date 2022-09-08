#include "commands/info_shell.h"
#include "commands/gpio_shell.h"
#include "commands/sensor_shell.h"
#include "commands/flash_shell.h"
#include "commands/log_shell.h"
#include "commands/ipmi_shell.h"

/* MAIN command */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_platform_cmds, SHELL_CMD(info, NULL, "Platform info.", cmd_info_print),
	SHELL_CMD(gpio, &sub_gpio_cmds, "GPIO relative command.", NULL),
	SHELL_CMD(sensor, &sub_sensor_cmds, "SENSOR relative command.", NULL),
	SHELL_CMD(flash, &sub_flash_cmds, "FLASH(spi) relative command.", NULL),
	SHELL_CMD(log, &sub_log_cmds, "Debug log relative command.", NULL),
	SHELL_CMD(ipmi, &sub_ipmi_cmds, "Test BIC IPMI command.", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(platform, &sub_platform_cmds, "Platform commands", NULL);

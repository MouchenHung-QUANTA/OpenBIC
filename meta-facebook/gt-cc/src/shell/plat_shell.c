#include <zephyr.h>
#include <shell/shell.h>
#include "plat_shell_e1s.h"
#include "plat_shell_pex.h"

/* Sub-command Level 2 of command test */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_e1s_cmds,
			       SHELL_CMD(power, NULL, "Stress E1S power consumption",
					 cmd_stress_e1s_pwr),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pex_cmds,
			       SHELL_CMD(read, NULL, "Read PEX register.", cmd_pex_read),
			       SHELL_CMD(write, NULL, "Write PEX register.", cmd_pex_write),
			       SHELL_SUBCMD_SET_END);

/* Sub-command Level 1 of command test */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_test_cmds,
			       SHELL_CMD(e1s, &sub_e1s_cmds, "E1S related command", NULL),
			       SHELL_CMD(pex, &sub_pex_cmds, "PEX related command", NULL),
			       SHELL_SUBCMD_SET_END);

/* Root of command test */
SHELL_CMD_REGISTER(test, &sub_test_cmds, "Test commands for GT", NULL);

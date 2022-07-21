#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <shell/shell.h>

#include "sensor.h"
#include "hal_i2c.h"
#include "pex89000.h"
#include "plat_gpio.h"
#include "plat_hook.h"
#include "plat_sensor_table.h"

typedef enum {
	PEX_PRE_ACCESS,
	PEX_POST_ACCESS,
} pex_access_moment_t;

uint8_t pex_access_check(const struct shell *shell, pex89000_i2c_msg_t *pex_info,
			 pex_access_moment_t moment)
{
	switch (moment) {
	case PEX_PRE_ACCESS:
		if (!is_mb_dc_on()) {
			shell_warn(shell, "%s: Try again after dc on!", __func__);
			return 1;
		}
		disable_sensor_poll();

		for (int i = 0; i < plat_get_config_size(); i++) {
			sensor_cfg *cfg = &sensor_config[i];
			if (cfg->type != sensor_dev_pex89000)
				continue;

			pex_info->bus = cfg->port;
			pex_info->address = cfg->target_addr;

			pex89000_init_arg *init_arg = cfg->init_args;
			if (!init_arg)
				continue;
			if (init_arg->idx != pex_info->idx)
				continue;
			if (!cfg->pre_sensor_read_hook)
				continue;
			if (!cfg->pre_sensor_read_hook(cfg->num, cfg->pre_sensor_read_args)) {
				shell_error(shell, "%s: PEX[%d] pre-access failed!\n", __func__,
					    pex_info->idx);
				return 1;
			}
			break;
		}
		break;

	case PEX_POST_ACCESS:
		for (int i = 0; i < plat_get_config_size(); i++) {
			sensor_cfg *cfg = &sensor_config[i];
			if (cfg->type != sensor_dev_pex89000)
				continue;

			pex89000_init_arg *init_arg = cfg->init_args;
			if (!init_arg)
				continue;
			if (init_arg->idx != pex_info->idx)
				continue;
			if (!cfg->post_sensor_read_hook)
				continue;
			if (!cfg->post_sensor_read_hook(cfg->num, cfg->post_sensor_read_args,
							NULL)) {
				shell_error(shell, "%s: PEX[%d] post-access failed!\n", __func__,
					    pex_info->idx);
				return 1;
			}
			break;
		}
		enable_sensor_poll();
		break;

	default:
		shell_error(shell, "%s: Invalid access moment %d\n", __func__, moment);
		return 1;
	}

	return 0;
}

void cmd_pex_read(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: test pex read <pex_idx> <pex_reg>");
		return;
	}

	int pex_idx = strtol(argv[1], NULL, 10);
	char *endptr;
	uint32_t pex_reg = strtoul(argv[2], &endptr, 16);

	pex89000_i2c_msg_t pex_msg = { 0 };
	pex_msg.idx = pex_idx;
	if (pex_access_check(shell, &pex_msg, PEX_PRE_ACCESS)) {
		shell_error(shell, "Failed to do pex pre-access");
		goto exit;
	}

	pex_msg.axi_reg = pex_reg;

	if (pex_write_read(&pex_msg, pex_do_read)) {
		shell_error(shell, "pex read failed!");
		goto exit;
	}
	shell_print(shell, "PEX_IDX[%d] - PEX_REG[0x%x]: 0x%x", pex_idx, pex_reg, pex_msg.axi_data);

exit:
	if (pex_access_check(shell, &pex_msg, PEX_POST_ACCESS)) {
		shell_error(shell, "Failed to do pex post-access");
		return;
	}
}

void cmd_pex_write(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_warn(shell, "Help: test pex write <pex_idx> <pex_reg> <pex_data>");
		return;
	}

	int pex_idx = strtol(argv[1], NULL, 10);
	char *endptr;
	uint32_t pex_reg = strtoul(argv[2], &endptr, 16);
	uint32_t pex_data = strtoul(argv[3], &endptr, 16);

	pex89000_i2c_msg_t pex_msg = { 0 };
	pex_msg.idx = pex_idx;
	if (pex_access_check(shell, &pex_msg, PEX_PRE_ACCESS)) {
		shell_error(shell, "Failed to do pex pre-access");
		goto exit;
	}

	pex_msg.axi_reg = pex_reg;
	pex_msg.axi_data = pex_data;

	if (pex_write_read(&pex_msg, pex_do_write)) {
		shell_error(shell, "pex write failed!");
		goto exit;
	}

	shell_print(shell, "PEX_IDX[%d] - PEX_REG[0x%x] wr DATA[0x%x] success!", pex_idx, pex_reg,
		    pex_msg.axi_data);

exit:
	if (pex_access_check(shell, &pex_msg, PEX_POST_ACCESS)) {
		shell_error(shell, "Failed to do pex post-access");
		return;
	}
}

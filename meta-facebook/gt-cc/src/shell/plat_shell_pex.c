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
	pex_pre_access,
	pex_post_access,
} pex_access_moment_t;

typedef struct {
	int idx;
	uint8_t bus;
	uint8_t addr;
} ps_info_t;

uint8_t pex_access_check(pex_access_moment_t moment, ps_info_t *ps_info)
{
	switch (moment) {
	case pex_pre_access:
		if (!is_mb_dc_on()) {
			printf("<warn> wait for dc on!\n");
			return 1;
		}
		disable_sensor_poll();

		/* before access pex */
		for (int i = 0; i < plat_get_config_size(); i++) {
			sensor_cfg *cfg = &sensor_config[i];
			if (cfg->type != sensor_dev_pex89000)
				continue;

			ps_info->bus = cfg->port;
			ps_info->addr = cfg->target_addr;

			pex89000_init_arg *init_arg = cfg->init_args;
			if (!init_arg)
				continue;
			if (init_arg->idx != ps_info->idx)
				continue;
			if (!cfg->pre_sensor_read_hook)
				continue;
			if (cfg->pre_sensor_read_hook(cfg->num, cfg->pre_sensor_read_args) ==
			    false) {
				printf("PEX[%d] pre-access failed!\n", ps_info->idx);
				return 1;
			}
			break;
		}
		break;

	case pex_post_access:
		/* after access pex */
		for (int i = 0; i < plat_get_config_size(); i++) {
			sensor_cfg *cfg = &sensor_config[i];
			if (cfg->type != sensor_dev_pex89000)
				continue;

			pex89000_init_arg *init_arg = cfg->init_args;
			if (!init_arg)
				continue;
			if (init_arg->idx != ps_info->idx)
				continue;
			if (!cfg->post_sensor_read_hook)
				continue;
			if (cfg->post_sensor_read_hook(cfg->num, cfg->post_sensor_read_args,
						       NULL) == false) {
				printf("PEX[%d] post-access failed!\n", ps_info->idx);
				return 1;
			}
			break;
		}
		enable_sensor_poll();
		break;

	default:
		printf("%s: Invalid access moment %d\n", __func__, moment);
		return 1;
	}

	return 0;
}

void cmd_pex_read(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: test pex read <ps_idx> <pex_reg>");
		return;
	}

	int ps_idx = strtol(argv[1], NULL, 10);
	char *endptr;
	uint32_t pex_reg = strtoul(argv[2], &endptr, 16);

	ps_info_t ps_info = { 0 };
	ps_info.idx = ps_idx;
	if (pex_access_check(pex_pre_access, &ps_info)) {
		shell_error(shell, "Failed to do pex pre-access");
		goto exit;
	}

	pex89000_i2c_msg_t pex_msg;
	pex_msg.idx = ps_idx;
	pex_msg.bus = ps_info.bus;
	pex_msg.address = ps_info.addr;
	pex_msg.axi_reg = pex_reg;

	if (pex_write_read(&pex_msg, pex_do_read)) {
		shell_error(shell, "pex read failed!");
		goto exit;
	}
	shell_print(shell, "PX_IDX[%d] - PEX_REG[0x%x]: 0x%x", ps_idx, pex_reg, pex_msg.axi_data);

exit:
	if (pex_access_check(pex_post_access, &ps_info)) {
		shell_error(shell, "Failed to do pex post-access");
		return;
	}
}

void cmd_pex_write(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_warn(shell, "Help: test pex write <ps_idx> <pex_reg> <pex_data>");
		return;
	}

	int ps_idx = strtol(argv[1], NULL, 10);
	char *endptr;
	uint32_t pex_reg = strtoul(argv[2], &endptr, 16);
	uint32_t pex_data = strtoul(argv[3], &endptr, 16);

	ps_info_t ps_info = { 0 };
	ps_info.idx = ps_idx;
	if (pex_access_check(pex_pre_access, &ps_info)) {
		shell_error(shell, "Failed to do pex pre-access");
		goto exit;
	}

	pex89000_i2c_msg_t pex_msg;
	pex_msg.idx = ps_idx;
	pex_msg.bus = ps_info.bus;
	pex_msg.address = ps_info.addr;
	pex_msg.axi_reg = pex_reg;
	pex_msg.axi_data = pex_data;

	if (pex_write_read(&pex_msg, pex_do_write)) {
		shell_error(shell, "pex write failed!");
		goto exit;
	}

	shell_print(shell, "PX_IDX[%d] - PEX_REG[0x%x] wr DATA[0x%x] success!", ps_idx, pex_reg,
		    pex_msg.axi_data);

exit:
	if (pex_access_check(pex_post_access, &ps_info)) {
		shell_error(shell, "Failed to do pex post-access");
		return;
	}
}

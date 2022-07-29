/*
    NAME: PLATFORM COMMAND
    FILE: shell_platform.c
    DESCRIPTION: OEM commands including gpio, sensor relative function access.
    CHIP/OS: AST1030 - Zephyr
    Note:
    (1) User command table 
          [topic]               [description]               [support]   [command]
        * GPIO                                              O
            * List group        List gpios' info in group   o           platform gpio list_group <gpio_device>
            * List all          List all gpios' info        o           platform gpio list_all
            * List multifnctl   List multi-fn-ctl regs      o           platform gpio multifnctl
            * Get               Get one gpio info           o           platform gpio get <gpio_num>
            * Set value         Set one gpio value          o           platform gpio set val <gpio_num> <value>
            * Set direction     Set one gpio direction      x           TODO
            * Set INT type      Set one gpio interrupt T    o           platform gpio set int_type <gpio_num> <type>
        * SENSOR                                            O
            * List all          List all sensors' info      o           platform sensor list_all
            * Get               Get one sensor info         o           platform sensor get <sensor_num>
            * Set polling en    Set one sensor polling      x           TODO
            * Set mbr           Set one sensor mbr          x           TODO
            * Set threshold     Set one sensor threshold    x           TODO
    (2) Some hard code features need to be modified if CHIP is different
    (3) GPIO List all/Get/Set value/Set direction are not protected by PINMASK_RESERVE_CHECK. It might cause system-hang problem!
    (4) "List multifnctl" command only lists relative registers mentioned in chip spec
        chapter "5.Multi-function Pins Mapping and Control" 
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <device.h>
#include <devicetree.h>

/* Include VERSION */
#include "plat_version.h"

/* Include GPIO */
#include <drivers/gpio.h>
#include "plat_gpio.h"
#include "hal_gpio.h"

/* Include SENSOR */
#include "sensor.h"
#include "sdr.h"

/* Include FLASH */
#include <drivers/spi_nor.h>
#include <drivers/flash.h>

/* Include config settings */
#include "shell_platform.h"

/*********************************************************************************************************
 * TOOL SECTION
**********************************************************************************************************/
/* 
    Command GPIO 
*/
#if PINMASK_RESERVE_CHECK
enum CHECK_RESV { CHECK_BY_GROUP_IDX, CHECK_BY_GLOBAL_IDX };
static int gpio_check_reserve(const struct device *dev, int gpio_idx, enum CHECK_RESV mode)
{
	if (!dev) {
		return 1;
	}

	const struct gpio_driver_config *const cfg = (const struct gpio_driver_config *)dev->config;

	if (!cfg->port_pin_mask) {
		return 1;
	}

	int gpio_idx_in_group;
	if (mode == CHECK_BY_GROUP_IDX) {
		gpio_idx_in_group = gpio_idx;
	} else if (mode == CHECK_BY_GLOBAL_IDX) {
		gpio_idx_in_group = gpio_idx % GPIO_GROUP_SIZE;
	} else {
		return 1;
	}

	if ((cfg->port_pin_mask & (gpio_port_pins_t)BIT(gpio_idx_in_group)) == 0U) {
		return 1;
	}

	return 0;
}
#endif

static int gpio_access_cfg(const struct shell *shell, int gpio_idx, enum GPIO_ACCESS mode,
			   int *data)
{
	if (!shell) {
		return 1;
	}

	if (gpio_idx >= GPIO_CFG_SIZE || gpio_idx < 0) {
		shell_error(shell, "gpio_access_cfg - gpio index out of bound!");
		return 1;
	}

	switch (mode) {
	case GPIO_READ:
		if (gpio_cfg[gpio_idx].is_init == DISABLE) {
			return 1;
		}

		uint32_t g_val = sys_read32(GPIO_GROUP_REG_ACCESS[gpio_idx / 32]);
		uint32_t g_dir = sys_read32(GPIO_GROUP_REG_ACCESS[gpio_idx / 32] + 0x4);

		char *pin_prop = (gpio_cfg[gpio_idx].property == OPEN_DRAIN) ? "OD" : "PP";
		char *pin_dir = (gpio_cfg[gpio_idx].direction == GPIO_INPUT) ? "input" : "output";

		char *pin_dir_reg = "I";
		if (g_dir & BIT(gpio_idx % 32))
			pin_dir_reg = "O";

		int val = gpio_get(gpio_idx);
		if (val == 0 || val == 1) {
			shell_print(shell, "[%-3d] %-35s: %-3s | %-6s(%s) | %d(%d)", gpio_idx,
				    gpio_name[gpio_idx], pin_prop, pin_dir, pin_dir_reg, val,
				    GET_BIT_VAL(g_val, gpio_idx % 32));
		} else {
			shell_print(shell, "[%-3d] %-35s: %-3s | %-6s(%s) | %s", gpio_idx,
				    gpio_name[gpio_idx], pin_prop, pin_dir, pin_dir_reg, "resv");
		}

		break;

	case GPIO_WRITE:
		if (!data) {
			shell_error(shell, "gpio_access_cfg - GPIO_WRITE value empty!");
			return 1;
		}

		if (*data != 0 && *data != 1) {
			shell_error(
				shell,
				"gpio_access_cfg - GPIO_WRITE value should only accept 0 or 1!");
			return 1;
		}

		if (gpio_set(gpio_idx, *data)) {
			shell_error(shell, "gpio_access_cfg - GPIO_WRITE failed!");
			return 1;
		}

		break;

	default:
		shell_error(shell, "gpio_access_cfg - No such mode %d!", mode);
		break;
	}

	return 0;
}

static int gpio_get_group_idx_by_dev_name(const char *dev_name)
{
	if (!dev_name) {
		return -1;
	}

	int group_idx = -1;
	for (int i = 0; i < ARRAY_SIZE(GPIO_GROUP_NAME_LST); i++) {
		if (!strcmp(dev_name, GPIO_GROUP_NAME_LST[i]))
			group_idx = i;
	}

	return group_idx;
}

static const char *gpio_get_name(const char *dev_name, int pin_num)
{
	if (!dev_name) {
		return NULL;
	}

	int name_idx = -1;
	name_idx = pin_num + 32 * gpio_get_group_idx_by_dev_name(dev_name);

	if (name_idx == -1) {
		return NULL;
	}

	if (name_idx >= GPIO_CFG_SIZE) {
		return "Undefined";
	}

	return gpio_name[name_idx];
}

/* 
    Command SENSOR 
*/
static bool sensor_access_check(uint8_t sensor_num)
{
	bool (*access_checker)(uint8_t);
	access_checker = sensor_config[sensor_config_index_map[sensor_num]].access_checker;

	return (access_checker)(sensor_config[sensor_config_index_map[sensor_num]].num);
}

static int sensor_get_idx_by_sensor_num(uint16_t sensor_num)
{
	for (int sen_idx = 0; sen_idx < sensor_config_count; sen_idx++) {
		if (sensor_num == sensor_config[sen_idx].num)
			return sen_idx;
	}

	return -1;
}

static int get_sdr_index_by_sensor_num(uint8_t sensor_num)
{
	int index = 0;
	for (index = 0; index < sdr_count; ++index) {
		if (sensor_num == full_sdr_table[index].sensor_num) {
			return index;
		}
	}

	return -1;
}

static int sensor_access(const struct shell *shell, int sensor_num, enum SENSOR_ACCESS mode)
{
	if (!shell) {
		return -1;
	}

	if (sensor_num >= SENSOR_NUM_MAX || sensor_num < 0) {
		return -1;
	}

	int sdr_index = get_sdr_index_by_sensor_num(sensor_num);
	if (sdr_index == -1) {
		shell_error(shell, "[%s] can't find sensor number in sdr table.\n", __func__);
		return -1;
	}

	switch (mode) {
	/* Get sensor info by "sensor_config" table */
	case SENSOR_READ:;
		int sen_idx = sensor_get_idx_by_sensor_num(sensor_num);
		if (sen_idx == -1) {
			shell_error(shell, "No such sensor number!");
			return -1;
		}
		char sensor_name[MAX_SENSOR_NAME_LENGTH] = { 0 };
		snprintf(sensor_name, sizeof(sensor_name), "%s", full_sdr_table[sdr_index].ID_str);

		char *check_access =
			(sensor_access_check(sensor_config[sen_idx].num) == true) ? "O" : "X";
		shell_print(shell, "[0x%-2x] %-25s: %-10s | access[%s] | %-25s | %-8d",
			    sensor_config[sen_idx].num, sensor_name,
			    sensor_type_name[sensor_config[sen_idx].type], check_access,
			    sensor_status_name[sensor_config[sen_idx].cache_status],
			    sensor_config[sen_idx].cache);
		break;

	case SENSOR_WRITE:
		/* TODO */
		break;

	default:
		break;
	}

	return 0;
}

/*********************************************************************************************************
 * COMMAND FUNCTION SECTION
**********************************************************************************************************/
/* 
    Command header 
*/
static int cmd_info_print(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
	shell_print(shell, "* NAME:          Platform command");
	shell_print(shell, "* DESCRIPTION:   Commands that could be used to debug or validate.");
	shell_print(shell, "* DATE/VERSION:  none");
	shell_print(shell, "* CHIP/OS:       AST1030 - Zephyr");
	shell_print(shell, "* Note:          none");
	shell_print(shell, "------------------------------------------------------------------");
	shell_print(shell, "* PLATFORM:      %s-%s", PLATFORM_NAME, PROJECT_NAME);
	shell_print(shell, "* FW VERSION:    %d.%d", FIRMWARE_REVISION_1, FIRMWARE_REVISION_2);
	shell_print(shell, "* FW DATE:       %x%x.%x.%x", BIC_FW_YEAR_MSB, BIC_FW_YEAR_LSB,
		    BIC_FW_WEEK, BIC_FW_VER);
	shell_print(shell, "* FW IMAGE:      %s.bin", CONFIG_KERNEL_BIN_NAME);
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
	return 0;
}

/* 
    Command GPIO
*/
static void cmd_gpio_cfg_list_group(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: platform gpio list_group <gpio_device>");
		return;
	}

	const struct device *dev;
	dev = device_get_binding(argv[1]);

	if (!dev) {
		shell_error(shell, "Device [%s] not found!", argv[1]);
		return;
	}

	int g_idx = gpio_get_group_idx_by_dev_name(dev->name);
	int max_group_pin = num_of_pin_in_one_group_lst[g_idx];

	uint32_t g_val = sys_read32(GPIO_GROUP_REG_ACCESS[g_idx]);
	uint32_t g_dir = sys_read32(GPIO_GROUP_REG_ACCESS[g_idx] + 0x4);

	for (int index = 0; index < max_group_pin; index++) {
		if (gpio_cfg[g_idx * 32 + index].is_init == DISABLE) {
			shell_print(shell, "[%-3d][%s %-3d] %-35s: -- | %-9s | NA",
				    g_idx * 32 + index, dev->name, index, "gpio_disable", "i/o");
			continue;
		}

#if PINMASK_RESERVE_CHECK
		/* avoid pin_mask from devicetree "gpio-reserved" */
		if (gpio_check_reserve(dev, index, CHECK_BY_GROUP_IDX)) {
			shell_print(shell, "[%-3d][%s %-3d] %-35s: -- | %-9s | NA",
				    g_idx * 32 + index, dev->name, index, "gpio_reserve", "i/o");
			continue;
		}
#endif
		char *pin_dir = "output";
		if (gpio_cfg[g_idx * 32 + index].direction == GPIO_INPUT) {
			pin_dir = "input";
		}

		char *pin_dir_reg = "I";
		if (g_dir & BIT(index)) {
			pin_dir_reg = "O";
		}

		char *pin_prop =
			(gpio_cfg[g_idx * 32 + index].property == OPEN_DRAIN) ? "OD" : "PP";

		int rc;
		rc = gpio_pin_get(dev, index);
		if (rc >= 0) {
			shell_print(shell, "[%-3d][%s %-3d] %-35s: %2s | %-6s(%s) | %d(%d)",
				    g_idx * 32 + index, dev->name, index,
				    gpio_get_name(dev->name, index), pin_prop, pin_dir, pin_dir_reg,
				    rc, GET_BIT_VAL(g_val, index));
		} else {
			shell_error(shell, "[%-3d][%s %-3d] %-35s: %2s | %-6s | err[%d]",
				    g_idx * 32 + index, dev->name, index,
				    gpio_get_name(dev->name, index), pin_prop, pin_dir, rc);
		}
	}

	return;
}

static void cmd_gpio_cfg_list_all(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "Help: platform gpio list_all");
		return;
	}

	for (int gpio_idx = 0; gpio_idx < GPIO_CFG_SIZE; gpio_idx++)
		gpio_access_cfg(shell, gpio_idx, GPIO_READ, NULL);

	return;
}

static void cmd_gpio_cfg_get(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: platform gpio get <gpio_idx>");
		return;
	}

	int gpio_index = strtol(argv[1], NULL, 10);
	if (gpio_access_cfg(shell, gpio_index, GPIO_READ, NULL))
		shell_error(shell, "gpio[%d] get failed!", gpio_index);

	return;
}

static void cmd_gpio_cfg_set_val(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: platform gpio set val <gpio_idx> <data>");
		return;
	}

	int gpio_index = strtol(argv[1], NULL, 10);
	int data = strtol(argv[2], NULL, 10);

	if (gpio_access_cfg(shell, gpio_index, GPIO_WRITE, &data))
		shell_error(shell, "gpio[%d] --> %d ,failed!", gpio_index, data);
	else
		shell_print(shell, "gpio[%d] --> %d ,success!", gpio_index, data);

	return;
}

static void cmd_gpio_cfg_set_int_type(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: platform gpio set int_type <gpio_idx> <type>");
		shell_warn(
			shell,
			"     type: [0]disable [1]edge-rise [2]edge-fall [3]edge-both [4]low [5]high");
		return;
	}

	int gpio_index = strtol(argv[1], NULL, 10);
	int type_idx = strtol(argv[2], NULL, 10);

	if (type_idx >= ARRAY_SIZE(int_type_table) || type_idx < 0) {
		shell_error(shell, "Wrong index of type!");
		shell_warn(
			shell,
			"type: [0]disable [1]edge-rise [2]edge-fall [3]edge-both [4]low [5]high");
		return;
	}

	if (gpio_interrupt_conf(gpio_index, int_type_table[type_idx]))
		shell_error(shell, "gpio[%d] --> type[%d] failed!", gpio_index, type_idx);
	else
		shell_print(shell, "gpio[%d] --> type[%d] success!", gpio_index, type_idx);

	return;
}

static void cmd_gpio_muti_fn_ctl_list(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "Help: platform gpio multifnctl");
		return;
	}

	printf("[   REG    ]  hi                                      lo\n");
	for (int lst_idx = 0; lst_idx < ARRAY_SIZE(GPIO_MULTI_FUNC_PIN_CTL_REG_ACCESS); lst_idx++) {
		uint32_t cur_status = sys_read32(GPIO_MULTI_FUNC_PIN_CTL_REG_ACCESS[lst_idx]);
		printf("[0x%x]", GPIO_MULTI_FUNC_PIN_CTL_REG_ACCESS[lst_idx]);
		for (int i = 32; i > 0; i--) {
			if (!(i % 4)) {
				printf(" ");
			}
			if (!(i % 8)) {
				printf(" ");
			}
			printf("%d", (int)GET_BIT_VAL(cur_status, i - 1));
		}
		printf("\n");
	}

	shell_print(shell, "\n");
}

/*
    Command SENSOR
*/
static void cmd_sensor_cfg_list_all(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 1) {
		shell_warn(shell, "Help: platform sensor list_all");
		return;
	}

	if (sensor_config_count == 0) {
		shell_warn(shell, "[%s]: sensor monitor count is zero", __func__);
		return;
	}

	shell_print(
		shell,
		"---------------------------------------------------------------------------------");
	for (int sen_idx = 0; sen_idx < sensor_config_count; sen_idx++)
		sensor_access(shell, sensor_config[sen_idx].num, SENSOR_READ);

	shell_print(
		shell,
		"---------------------------------------------------------------------------------");
}

static void cmd_sensor_cfg_get(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: platform sensor get <sensor_num>");
		return;
	}

	int sen_num = strtol(argv[1], NULL, 16);

	sensor_access(shell, sen_num, SENSOR_READ);
}

static void cmd_control_sensor_polling(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "[%s]: input parameter count is invalid", __func__);
		return;
	}

	uint8_t sensor_num = strtol(argv[1], NULL, 16);
	uint8_t operation = strtol(argv[2], NULL, 16);
	int sensor_index = sensor_get_idx_by_sensor_num(sensor_num);
	if (sensor_index == -1) {
		shell_warn(
			shell,
			"[%s]: can't find sensor number in sensor config table, sensor number: 0x%x",
			__func__, sensor_num);
		return;
	}

	if ((operation != DISABLE_SENSOR_POLLING) && (operation != ENABLE_SENSOR_POLLING)) {
		shell_warn(shell, "[%s]: operation is invalid, operation: %d", __func__, operation);
		return;
	}

	sensor_config[sensor_index].is_enable_polling =
		((operation == DISABLE_SENSOR_POLLING) ? DISABLE_SENSOR_POLLING :
							 ENABLE_SENSOR_POLLING);
	shell_print(shell, "Sensor number 0x%x %s sensor polling success", sensor_num,
		    ((operation == DISABLE_SENSOR_POLLING) ? "disable" : "enable"));
	return;
}

/* 
    Command FLASH
*/
static void cmd_flash_re_init(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: platform flash re_init <spi_device>");
		return;
	}

	const struct device *flash_dev;
	flash_dev = device_get_binding(argv[1]);

	if (!flash_dev) {
		shell_error(shell, "Can't find any binding device with label %s", argv[1]);
	}

	if (spi_nor_re_init(flash_dev)) {
		shell_error(shell, "%s re-init failed!", argv[1]);
	}

	shell_print(shell, "%s re-init success!", argv[1]);
	return;
}

typedef void (*dw_extractor)(const struct jesd216_param_header *php,
			     const struct jesd216_bfp *bfp);

/* Indexed from 1 to match JESD216 data word numbering */
#define JESD216_MAX_MODE_SUPPORT 11

struct mode_str {
	enum jesd216_mode_type mode;
	char *mode_str;
};

static const struct mode_str mode_tags[JESD216_MAX_MODE_SUPPORT] = {
	{JESD216_MODE_044, "QSPI XIP"},
	{JESD216_MODE_088, "OSPI XIP"},
	{JESD216_MODE_111, "1-1-1"},
	{JESD216_MODE_112, "1-1-2"},
	{JESD216_MODE_114, "1-1-4"},
	{JESD216_MODE_118, "1-1-8"},
	{JESD216_MODE_122, "1-2-2"},
	{JESD216_MODE_144, "1-4-4"},
	{JESD216_MODE_188, "1-8-8"},
	{JESD216_MODE_222, "2-2-2"},
	{JESD216_MODE_444, "4-4-4"},
};

static void summarize_dw1(const struct jesd216_param_header *php,
			  const struct jesd216_bfp *bfp)
{
	uint32_t dw1 = sys_le32_to_cpu(bfp->dw1);

	printf("DTR Clocking %ssupported\n",
	       (dw1 & JESD216_SFDP_BFP_DW1_DTRCLK_FLG) ? "" : "not ");

	static const char *const addr_bytes[] = {
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B] = "3-Byte only",
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B] = "3- or 4-Byte",
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B] = "4-Byte only",
		[3] = "Reserved",
	};

	printf("Addressing: %s\n", addr_bytes[(dw1 & JESD216_SFDP_BFP_DW1_ADDRBYTES_MASK)
					      >> JESD216_SFDP_BFP_DW1_ADDRBYTES_SHFT]);

	static const char *const bsersz[] = {
		[0] = "Reserved 00b",
		[JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KSUP] = "uniform",
		[2] = "Reserved 01b",
		[JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KNOTSUP] = "not uniform",
	};

	printf("4-KiBy erase: %s\n", bsersz[(dw1 & JESD216_SFDP_BFP_DW1_BSERSZ_MASK)
					    >> JESD216_SFDP_BFP_DW1_BSERSZ_SHFT]);

	for (size_t mi = 0; mi < JESD216_MAX_MODE_SUPPORT; ++mi) {
		struct jesd216_instr cmd;
		int rc = jesd216_bfp_read_support(php, bfp, mode_tags[mi].mode, &cmd);

		if (rc == 0) {
			printf("Support %s\n", mode_tags[mi].mode_str);
		} else if (rc > 0) {
			printf("Support %s: instr %02Xh, %u mode clocks, %u waits\n",
			       mode_tags[mi].mode_str, cmd.instr, cmd.mode_clocks,
			       cmd.wait_states);
		}
	}
}

static void summarize_dw2(const struct jesd216_param_header *php,
			  const struct jesd216_bfp *bfp)
{
	printf("Flash density: %u bytes\n", (uint32_t)(jesd216_bfp_density(bfp) / 8));
}

static void summarize_dw89(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_erase_type etype;
	uint32_t typ_ms;
	int typ_max_mul;

	for (uint8_t idx = 1; idx < JESD216_NUM_ERASE_TYPES; ++idx) {
		if (jesd216_bfp_erase(bfp, idx, &etype) == 0) {
			typ_max_mul = jesd216_bfp_erase_type_times(php, bfp,
								   idx, &typ_ms);

			printf("ET%u: instr %02Xh for %u By", idx, etype.cmd,
			       (uint32_t)BIT(etype.exp));
			if (typ_max_mul > 0) {
				printf("; typ %u ms, max %u ms",
				       typ_ms, typ_max_mul * typ_ms);
			}
			printf("\n");
		}
	}
}

static void summarize_dw11(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw11 dw11;

	if (jesd216_bfp_decode_dw11(php, bfp, &dw11) != 0) {
		return;
	}

	printf("Chip erase: typ %u ms, max %u ms\n",
	       dw11.chip_erase_ms, dw11.typ_max_factor * dw11.chip_erase_ms);

	printf("Byte program: type %u + %u * B us, max %u + %u * B us\n",
	       dw11.byte_prog_first_us, dw11.byte_prog_addl_us,
	       dw11.typ_max_factor * dw11.byte_prog_first_us,
	       dw11.typ_max_factor * dw11.byte_prog_addl_us);

	printf("Page program: typ %u us, max %u us\n",
	       dw11.page_prog_us,
	       dw11.typ_max_factor * dw11.page_prog_us);

	printf("Page size: %u By\n", dw11.page_size);
}

static void summarize_dw12(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	uint32_t dw12 = sys_le32_to_cpu(bfp->dw10[2]);
	uint32_t dw13 = sys_le32_to_cpu(bfp->dw10[3]);

	/* Inverted logic flag: 1 means not supported */
	if ((dw12 & JESD216_SFDP_BFP_DW12_SUSPRESSUP_FLG) != 0) {
		return;
	}

	uint8_t susp_instr = dw13 >> 24;
	uint8_t resm_instr = dw13 >> 16;
	uint8_t psusp_instr = dw13 >> 8;
	uint8_t presm_instr = dw13 >> 0;

	printf("Suspend: %02Xh ; Resume: %02Xh\n",
	       susp_instr, resm_instr);
	if ((susp_instr != psusp_instr)
	    || (resm_instr != presm_instr)) {
		printf("Program suspend: %02Xh ; Resume: %02Xh\n",
		       psusp_instr, presm_instr);
	}
}

static void summarize_dw14(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw14 dw14;

	if (jesd216_bfp_decode_dw14(php, bfp, &dw14) != 0) {
		return;
	}
	printf("DPD: Enter %02Xh, exit %02Xh ; delay %u ns ; poll 0x%02x\n",
	       dw14.enter_dpd_instr, dw14.exit_dpd_instr,
	       dw14.exit_delay_ns, dw14.poll_options);
}

static void summarize_dw15(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw15 dw15;

	if (jesd216_bfp_decode_dw15(php, bfp, &dw15) != 0) {
		return;
	}
	printf("HOLD or RESET Disable: %ssupported\n",
	       dw15.hold_reset_disable ? "" : "un");
	printf("QER: %u\n", dw15.qer);
	if (dw15.support_044) {
		printf("0-4-4 Mode methods: entry 0x%01x ; exit 0x%02x\n",
		       dw15.entry_044, dw15.exit_044);
	} else {
		printf("0-4-4 Mode: not supported");
	}
	printf("4-4-4 Mode sequences: enable 0x%02x ; disable 0x%01x\n",
	       dw15.enable_444, dw15.disable_444);
}

static void summarize_dw16(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw16 dw16;

	if (jesd216_bfp_decode_dw16(php, bfp, &dw16) != 0) {
		return;
	}

	uint8_t addr_support = jesd216_bfp_addrbytes(bfp);

	/* Don't display bits when 4-byte addressing is not supported. */
	if (addr_support != JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B) {
		printf("4-byte addressing support: enter 0x%02x, exit 0x%03x\n",
		       dw16.enter_4ba, dw16.exit_4ba);
	}
	printf("Soft Reset and Rescue Sequence support: 0x%02x\n",
	       dw16.srrs_support);
	printf("Status Register 1 support: 0x%02x\n",
	       dw16.sr1_interface);
}

static const dw_extractor extractor[] = {
	[1] = summarize_dw1,
	[2] = summarize_dw2,
	[8] = summarize_dw89,
	[11] = summarize_dw11,
	[12] = summarize_dw12,
	[14] = summarize_dw14,
	[15] = summarize_dw15,
	[16] = summarize_dw16,
};

static void dump_bfp(const struct jesd216_param_header *php,
		     const struct jesd216_bfp *bfp)
{
	uint8_t dw = 1;
	uint8_t limit = MIN(1U + php->len_dw, ARRAY_SIZE(extractor));

	printf("Summary of BFP content:\n");
	while (dw < limit) {
		dw_extractor ext = extractor[dw];

		if (ext != 0) {
			ext(php, bfp);
		}
		++dw;
	}
}

static void dump_bytes(const struct jesd216_param_header *php,
		       const uint32_t *dw)
{
	char buffer[4 * 3 + 1]; /* B1 B2 B3 B4 */
	uint8_t nw = 0;

	printf(" [\n\t");
	while (nw < php->len_dw) {
		const uint8_t *u8p = (const uint8_t *)&dw[nw];
		++nw;

		bool emit_nl = (nw == php->len_dw) || ((nw % 4) == 0);

		sprintf(buffer, "%02x %02x %02x %02x",
			u8p[0], u8p[1], u8p[2], u8p[3]);
		if (emit_nl) {
			printf("%s\n\t", buffer);
		} else {
			printf("%s  ", buffer);
		}
	}
	printf("];\n");
}

static void cmd_flash_sfdp_read(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: platform flash sfdp_read <spi_device>");
		return;
	}

	const struct device *flash_dev;
	flash_dev = device_get_binding(argv[1]);

	if (!flash_dev) {
		shell_error(shell, "Can't find any binding device with label %s", argv[1]);
	}

	if (!device_is_ready(flash_dev)) {
		shell_error(shell, "%s: device not ready", flash_dev->name);
		return;
	}

	uint8_t raw[256] = {0};
	int rc1 = flash_sfdp_read(flash_dev, 0, raw, sizeof(raw));
	if (rc1) {
		shell_error(shell, "read err!");
		return;
	}
	printf("sfdp raw:");
	for (int i=0; i<256; i++) {
		if (!(i%10))
			printf("\n");
		printf("0x%.2x ", raw[i]);
	}
	printf("\n");

	const uint8_t decl_nph = 5;
	union {
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;
	int rc = flash_sfdp_read(flash_dev, 0, u.raw, sizeof(u.raw));

	if (rc != 0) {
		shell_error(shell, "Read SFDP not supported: device not JESD216-compliant "
		       "(err %d)", rc);
		return;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		shell_error(shell, "SFDP magic %08x invalid", magic);
		return;
	}

	shell_print(shell, "%s: SFDP v %u.%u AP %x with %u PH", flash_dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	k_msleep(1000);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php + MIN(decl_nph, 1 + hp->nph);
	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);
		uint32_t addr = jesd216_param_addr(php);

		printf("PH%u: %04x rev %u.%u: %u DW @ %x\n",
		       (uint32_t)(php - hp->phdr), id, php->rev_major, php->rev_minor,
		       php->len_dw, addr);

		uint32_t dw[php->len_dw];

		rc = flash_sfdp_read(flash_dev, addr, dw, sizeof(dw));
		if (rc != 0) {
			printf("Read failed: %d\n", rc);
			return;
		}

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			const struct jesd216_bfp *bfp = (struct jesd216_bfp *)dw;

			dump_bfp(php, bfp);
			printf("size = <%u>;\n", (uint32_t)jesd216_bfp_density(bfp));
			printf("sfdp-bfp =");
		} else {
			printf("sfdp-%04x =", id);
		}

		dump_bytes(php, dw);

		++php;
	}

	uint8_t id[3];

	rc = flash_read_jedec_id(flash_dev, id);
	if (rc == 0) {
		printf("jedec-id = [%02x %02x %02x];\n",
		       id[0], id[1], id[2]);
	} else {
		printf("JEDEC ID read failed: %d\n", rc);
	}

	return;
}

/*********************************************************************************************************
 * COMMAND DECLARE SECTION
**********************************************************************************************************/

/* GPIO sub command */
static void device_gpio_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, GPIO_DEVICE_PREFIX);

	if (entry == NULL) {
		printf("device_gpio_name_get passed null entry\n");
		return;
	}

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}
SHELL_DYNAMIC_CMD_CREATE(gpio_device_name, device_gpio_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio_set_cmds,
			       SHELL_CMD(val, NULL, "Set pin value.", cmd_gpio_cfg_set_val),
			       SHELL_CMD(int_type, NULL, "Set interrupt pin type.",
					 cmd_gpio_cfg_set_int_type),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gpio_cmds,
	SHELL_CMD(list_group, &gpio_device_name, "List all GPIO config from certain group.",
		  cmd_gpio_cfg_list_group),
	SHELL_CMD(list_all, NULL, "List all GPIO config.", cmd_gpio_cfg_list_all),
	SHELL_CMD(get, NULL, "Get GPIO config", cmd_gpio_cfg_get),
	SHELL_CMD(set, &sub_gpio_set_cmds, "Set GPIO config", NULL),
	SHELL_CMD(multifnctl, NULL, "List all GPIO multi-function control regs.",
		  cmd_gpio_muti_fn_ctl_list),
	SHELL_SUBCMD_SET_END);

/* Sensor sub command */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensor_cmds,
			       SHELL_CMD(list_all, NULL, "List all SENSOR config.",
					 cmd_sensor_cfg_list_all),
			       SHELL_CMD(get, NULL, "Get SENSOR config", cmd_sensor_cfg_get),
			       SHELL_CMD(control_sensor_polling, NULL,
					 "Enable/Disable sensor polling",
					 cmd_control_sensor_polling),
			       SHELL_SUBCMD_SET_END);

/* Flash sub command */
static void device_spi_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, SPI_DEVICE_PREFIX);

	if (entry == NULL) {
		printf("%s passed null entry\n", __func__);
		return;
	}

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}
SHELL_DYNAMIC_CMD_CREATE(spi_device_name, device_spi_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_flash_cmds,
			       SHELL_CMD(re_init, &spi_device_name, "Re-init spi config",
					 cmd_flash_re_init),
					 SHELL_CMD(sfpd_read, &spi_device_name, "SFPD read",
					 cmd_flash_sfdp_read),
			       SHELL_SUBCMD_SET_END);

/* MAIN command */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_platform_cmds, SHELL_CMD(info, NULL, "Platform info.", cmd_info_print),
	SHELL_CMD(gpio, &sub_gpio_cmds, "GPIO relative command.", NULL),
	SHELL_CMD(sensor, &sub_sensor_cmds, "SENSOR relative command.", NULL),
	SHELL_CMD(flash, &sub_flash_cmds, "FLASH(spi) relative command.", NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(platform, &sub_platform_cmds, "Platform commands", NULL);

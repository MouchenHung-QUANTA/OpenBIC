#include "flash_shell.h"
#include <devicetree.h>
#include <device.h>
#include <stdio.h>

/* 
    Command FLASH
*/
void cmd_flash_re_init(const struct shell *shell, size_t argc, char **argv)
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
		return;
	}

	shell_print(shell, "%s re-init success!", argv[1]);
	return;
}

void cmd_flash_sfdp_read(const struct shell *shell, size_t argc, char **argv)
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

	uint8_t raw[SFDP_BUFF_SIZE] = { 0 };
	int rc1 = flash_sfdp_read(flash_dev, 0, raw, sizeof(raw));
	if (rc1) {
		shell_error(shell, "read err!");
		return;
	}
	printf("sfdp raw with %d:", SFDP_BUFF_SIZE);
	for (int i = 0; i < SFDP_BUFF_SIZE; i++) {
		if (!(i % 4)) {
			printf("\n[%-3x] ", i);
		}
		printf("%.2x ", raw[i]);
	}
	printf("\n");

	return;
}

/* Flash sub command */
void device_spi_name_get(size_t idx, struct shell_static_entry *entry)
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

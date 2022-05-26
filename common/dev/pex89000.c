/* 
  PEX89000 Hardware I2C Slave UG_v1.0.pdf
  PEX89000_RM100.pdf
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <shell/shell.h>

#include "sensor.h"
#include "hal_i2c.h"
#include "pex89000.h"

#define BRCM_I2C5_CMD_READ 0b100
#define BRCM_I2C5_CMD_WRITE 0b011

#define BRCM_CHIME_AXI_CSR_ADDR 0x001F0100
#define BRCM_CHIME_AXI_CSR_DATA 0x001F0104
#define BRCM_CHIME_AXI_CSR_CTL 0x001F0108

#define BRCM_TEMP_SNR0_CTL_REG1 0xFFE78504
#define BRCM_TEMP_SNR0_STAT_REG0 0xFFE78538

#define BRCM_TEMP_SNR0_CTL_REG1_RESET 0x000653E8

#define BRCM_DEV_VENDOR_ID_ADDR 0x60800B78

static K_MUTEX_DEFINE(mutex_pex89000);

static sys_slist_t pex89000_list;

typedef struct {
	uint8_t idx; // Create index based on init variable
	struct k_mutex mutex;
	sys_snode_t node; // linked list node
} pex89000_unit;

typedef struct {
	uint8_t cmd : 3;
	uint8_t reserve1 : 5;
	uint8_t oft21_14bit : 8;
	uint8_t oft11_10bit : 2;
	uint8_t be : 4;
	uint8_t oft13_12bit : 2;
	uint8_t oft9_2bit : 8;
} __packed __aligned(4) HW_I2C_Cmd;

static void pex89000_i2c_encode(uint32_t oft, uint8_t be, uint8_t cmd, HW_I2C_Cmd *buf);
static uint8_t pex89000_chime_read(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *resp,
				   uint16_t resp_len);
static uint8_t pex89000_chime_write(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *data,
				    uint8_t data_len);
static uint8_t pend_for_read_valid(uint8_t bus, uint8_t addr);
static uint8_t pex89000_chime_to_axi_write(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t data);
static uint8_t pex89000_chime_to_axi_read(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t *resp);
static uint8_t pex89000_temp(uint8_t bus, uint8_t addr, uint32_t *val);

/*
 * be: byte enables
 * oft: Atlas register address
 * cmd: read or write command
 * buf: encoded byte array to send to pesw
 */
static void pex89000_i2c_encode(uint32_t oft, uint8_t be, uint8_t cmd, HW_I2C_Cmd *buf)
{
	if (!buf) {
		printf("pex89000_i2c_encode buf fail!\n");
		return;
	}

	buf->reserve1 = 0;
	buf->cmd = cmd;
	buf->oft21_14bit = (oft >> 14) & 0xFF;
	buf->oft13_12bit = (oft >> 12) & 0x3;
	buf->be = be;
	buf->oft11_10bit = (oft >> 10) & 0x3;
	buf->oft9_2bit = (oft >> 2) & 0xFF;
}

static uint8_t pex89000_chime_read(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *resp,
				   uint16_t resp_len)
{
	if (!resp) {
		printf("%s: *resp does not exist !!\n", __func__);
		return 1;
	}

	HW_I2C_Cmd cmd;
	pex89000_i2c_encode(oft, 0xF, BRCM_I2C5_CMD_READ, &cmd);

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = sizeof(cmd);
	msg.rx_len = resp_len;
	memcpy(&msg.data[0], &cmd, sizeof(cmd));

	if (i2c_master_read(&msg, retry)) {
		/* read fail */
		printf("pex89000 read failed!!\n");
		return 1;
	}

	memcpy(resp, &msg.data[0], resp_len);

	return 0;
}

static uint8_t pex89000_chime_write(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *data,
				    uint8_t data_len)
{
	if (!data) {
		printf("%s: *data does not exist !!\n", __func__);
		return 1;
	}

	HW_I2C_Cmd cmd;
	pex89000_i2c_encode(oft, 0xF, BRCM_I2C5_CMD_WRITE, &cmd);

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = sizeof(cmd) + data_len;
	memcpy(&msg.data[0], &cmd, sizeof(cmd));
	memcpy(&msg.data[4], data, data_len);

	if (i2c_master_write(&msg, retry)) {
		/* write fail */
		printf("pex89000 write failed!!\n");
		return 1;
	}

	return 0;
}

static uint8_t pend_for_read_valid(uint8_t bus, uint8_t addr)
{
	uint8_t rty = 50;
	uint32_t resp = 0;

	while (rty--) {
		if (pex89000_chime_read(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&resp,
					sizeof(resp))) {
			k_msleep(10);
			continue;
		}

		if (resp & BIT(27)) { // CHIME_to_AXI_CSR Control Status -> Read_data_vaild
			return 1; //  success
		}

		k_msleep(10);
	}

	return 0;
}

static uint8_t pex89000_chime_to_axi_write(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t data)
{
	uint8_t rc = 1;
	uint32_t wbuf;

	wbuf = sys_cpu_to_be32(oft);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&wbuf,
				 sizeof(wbuf))) {
		goto exit;
	}
	wbuf = sys_cpu_to_be32(data);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)&wbuf,
				 sizeof(wbuf))) {
		goto exit;
	}
	wbuf = sys_cpu_to_be32(0x1); // CHIME_to_AXI_CSR Control Status: write command
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&wbuf,
				 sizeof(wbuf))) {
		goto exit;
	}

	rc = 0;

exit:
	return rc;
}

static uint8_t pex89000_chime_to_axi_read(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t *resp)
{
	uint8_t rc = 1;

	if (!resp) {
		printf("%s: *resp does not exist !!\n", __func__);
		return rc;
	}

	uint32_t data;
	data = sys_cpu_to_be32(oft);

	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&data,
				 sizeof(data))) {
		goto exit;
	}
	data = sys_cpu_to_be32(0x2); // CHIME_to_AXI_CSR Control Status: read command
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&data,
				 sizeof(data))) {
		goto exit;
	}

	k_msleep(10);
	if (!pend_for_read_valid(bus, addr)) {
		printf("read data invaild\n");
		goto exit;
	}

	if (pex89000_chime_read(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)resp,
				sizeof(resp))) {
		goto exit;
	}

	*resp = sys_cpu_to_be32(*resp);
	rc = 0;

exit:
	return rc;
}

uint8_t pex_access_engine(uint8_t bus, uint8_t addr, pex_access_t key, uint32_t *resp)
{
	uint8_t rc = 1;

	if (!resp) {
		printf("%s: *resp does not exist !!\n", __func__);
		return 1;
	}

	if (k_mutex_lock(&mutex_pex89000, K_MSEC(1000))) {
		printk("%s: mutex get fail!\n", __func__);
		return 1;
	}

	switch (key) {
	case pex_access_temp:
		if (pex89000_temp(bus, addr, resp)) {
			printf("%s: TEMP access failed!\n", __func__);
			goto exit;
		}
		break;

	case pex_access_adc:
		printf("%s: ADC value get not support yet!\n", __func__);
		goto exit;

	case pex_access_id:
		if (pex89000_chime_to_axi_read(bus, addr, 0xFFF00000, resp)) {
			printf("%s: ID access failed!\n", __func__);
			goto exit;
		}
		break;

	case pex_access_rev_id:
		if (pex89000_chime_to_axi_read(bus, addr, 0xFFF00004, resp)) {
			printf("%s: REVISION ID access failed!\n", __func__);
			goto exit;
		}
		break;

	case pex_access_sbr_ver:
		if (pex89000_chime_to_axi_read(bus, addr, 0xFFF00008, resp)) {
			printf("%s: SVR VERSION access failed!\n", __func__);
			goto exit;
		}
		break;

	case pex_access_flash_ver:
		if (pex89000_chime_to_axi_read(bus, addr, 0x100005f8, resp)) {
			printf("%s: FLASH VERSION access failed!\n", __func__);
			goto exit;
		}
		break;

	default:
		printf("%s: Invalid key %d\n", __func__, key);
		break;
	}

	rc = 0;
exit:
	if (k_mutex_unlock(&mutex_pex89000))
		printf("--> unlock filed!\n");
	return rc;
}

static uint8_t pex89000_temp(uint8_t bus, uint8_t addr, uint32_t *val)
{
	if (!val) {
		printf("%s: *val does not exist !\n", __func__);
		return 1;
	}

	float pre_highest_temp = 0;
	float temp = 0;
	float temp_arr[12];
	uint32_t CmdAddr;
	uint32_t resp = 0;
	uint8_t rc = 1;

	if (pex89000_chime_to_axi_read(bus, addr, 0xFFF00000, &resp)) {
		printf("ID access failed!\n");
		goto exit;
	}

	pex_dev_t dev;
	uint16_t dev_id = (resp >> 16) & 0xFFFF;
	if (dev_id == 0xC010 || dev_id == 0xC012)
		dev = pex_dev_atlas1;
	else if (dev_id == 0xC030)
		dev = pex_dev_atlas2;
	else
		dev = pex_dev_unknown;

	if (dev == pex_dev_atlas1) {
		//check 0xFFE78504 value
		if (pex89000_chime_to_axi_read(bus, addr, BRCM_TEMP_SNR0_CTL_REG1, &resp)) {
			printf("CHIME to AXI Read 0xFFE78504 fail!\n");
			goto exit;
		}
		if (resp != BRCM_TEMP_SNR0_CTL_REG1_RESET) {
			printf("ADC temperature control register1 check fail!\n");
			goto exit;
		}

		//Write 0xFFE78504 = 200653E8
		if (pex89000_chime_to_axi_write(bus, addr, BRCM_TEMP_SNR0_CTL_REG1,
						BRCM_TEMP_SNR0_CTL_REG1_RESET)) {
			printf("CHIME to AXI Write 0xFFE78504 fail!\n");
			goto exit;
		}

		//Read 0xFFE78538
		if (pex89000_chime_to_axi_read(bus, addr, BRCM_TEMP_SNR0_STAT_REG0, &resp)) {
			printf("CHIME to AXI Write 0xFFE78538 fail!\n");
			goto exit;
		}

		temp = (resp & 0xFFFF) / 128;
	} else if (dev == pex_dev_atlas2) {
		for (int8_t i = 7; i < 12; i++) {
			CmdAddr = (0x21 << 16) | (0x4C << 8) | (0x0B);
			if (pex89000_chime_to_axi_write(bus, addr, 0xFFE00004, CmdAddr)) {
				printf("CHIME to AXI Write 0xFFE00004 fail!\n");
				goto exit;
			}

			if (pex89000_chime_to_axi_write(bus, addr, 0xFFE00008, i | 0x10000)) {
				printf("CHIME to AXI Write 0xFFE00008 fail!\n");
				goto exit;
			}

			CmdAddr = (0x22 << 16) | (0x4C << 8) | (0x14);
			if (pex89000_chime_to_axi_write(bus, addr, 0xFFE0000C, CmdAddr)) {
				printf("CHIME to AXI Write 0xFFE0000C fail!\n");
				goto exit;
			}

			if (pex89000_chime_to_axi_read(bus, addr, 0xFFE00010, &resp)) {
				printf("CHIME to AXI Write 0xFFE00010 fail!\n");
				goto exit;
			}

			temp = (float)(366.812 - 0.23751 * (float)(resp & 0x7FF));
			temp_arr[i] = temp;

			if (temp > pre_highest_temp)
				pre_highest_temp = temp;
		}

		temp = pre_highest_temp;
	} else {
		printf("%s: device not support!\n", __func__);
		goto exit;
	}

	*val = (((int)temp & 0xFFFF) << 16) | (((int)(temp * 1000) % 1000) & 0xFFFF);
	rc = 0;
exit:
	return rc;
}

pex89000_unit *find_pex89000_from_idx(uint8_t idx)
{
	sys_snode_t *node;
	SYS_SLIST_FOR_EACH_NODE (&pex89000_list, node) {
		pex89000_unit *p;
		p = CONTAINER_OF(node, pex89000_unit, node);
		if (p->idx == idx) {
			return p;
		}
	}

	return NULL;
}

uint8_t pex89000_read(uint8_t sensor_num, int *reading)
{
	if (!reading) {
		printf("%s: *reading does not exist !!\n", __func__);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t rc = SENSOR_UNSPECIFIED_ERROR;

	switch (sensor_config[sensor_config_index_map[sensor_num]].offset) {
	case PEX_TEMP:
		if (pex_access_engine(sensor_config[sensor_config_index_map[sensor_num]].port,
				      sensor_config[sensor_config_index_map[sensor_num]].target_addr,
				      pex_access_temp, reading)) {
			printf("%s: read temp fail!\n", __func__);
			rc = SENSOR_FAIL_TO_ACCESS;
			goto exit;
		}

		break;
	default:
		printf("%s: type fail!\n", __func__);
		rc = SENSOR_UNSPECIFIED_ERROR;
		goto exit;
	}

	sensor_val temp;
	temp.integer = (*reading >> 16) & 0xffff;
	temp.fraction = *reading & 0xffff;

	printf("temp[0x%x]: %d.%d\n", sensor_num, temp.integer, temp.fraction);
	rc = SENSOR_READ_SUCCESS;

exit:
	return rc;
}

bool pex89000_init(uint8_t sensor_num)
{
	sensor_config[sensor_config_index_map[sensor_num]].read = pex89000_read;
	return SENSOR_INIT_SUCCESS;
}

static uint8_t mux_switch(uint8_t ps_idx)
{
	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	msg.bus = 9;
	/* change address to 7-bit */
	msg.target_addr = ((0xe0) >> 1);
	msg.tx_len = 1;
	msg.data[0] = (1 << (ps_idx));

	if (i2c_master_write(&msg, retry))
		return 1;

	return 0;
}

static int cmd_pex_temp(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: pex temp <ps_idx>");
		return 0;
	}
	int ps_idx = strtol(argv[1], NULL, 10);

	if (ps_idx > 3) {
		shell_warn(shell, "<ps_idx> should lower than 4");
		return 0;
	}

	printf("pex89000 temp!\n");

	uint8_t bus = 9;
	uint8_t sa = 0x18 + ps_idx;
	uint32_t val;

	if (mux_switch(ps_idx)) {
		printf("--> fail to switch mux!\n");
		return 0;
	}

	if (pex_access_engine(bus, sa, pex_access_temp, &val)) {
		printf("[pex test] F\n");
		return 0;
	}

	printf("[pex test] T: %d.%d\n", (val >> 16) & 0xFFFF, val & 0xFFFF);

	return 0;
}

static int cmd_pex_conf(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: pex conf <ps_idx> <access_idx>");
		return 0;
	}

	printf("pex89000 conf!\n");

	int ps_idx = strtol(argv[1], NULL, 10);
	int access_idx = strtol(argv[2], NULL, 10);

	if (ps_idx > 3) {
		shell_warn(shell, "<ps_idx> should lower than 4");
		return 0;
	}

	if (access_idx > pex_access_unknown) {
		shell_warn(shell, "<access_idx> should lower than %d", pex_access_engine);
		return 0;
	}

	uint8_t bus = 9;
	uint8_t sa = 0x18 + ps_idx;

	if (mux_switch(ps_idx)) {
		printf("--> fail to switch mux!\n");
		return 0;
	}

	uint32_t resp;
	if (pex_access_engine(bus, sa, access_idx, &resp)) {
		printf("--> fail to get resp!\n");
		return 0;
	}

	printf("--> 0x%x\n", resp);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pex, SHELL_CMD(temp, NULL, "pex89000 temp ", cmd_pex_temp),
			       SHELL_CMD(conf, NULL, "pex89000 conf ", cmd_pex_conf),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(pex, &sub_pex, "pex89000 commands", NULL);
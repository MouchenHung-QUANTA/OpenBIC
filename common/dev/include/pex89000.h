#ifndef PEX_89000_H
#define PEX_89000_H

#include <stdint.h>

#define PEX_TOTAL_PORT_NUMBER 72
#define PEX_AXI_PORT_OFFSET 0x1000

/* Boardcom AXI register */
#define BRCM_REG_TEMP_SNR0_CTL 0xFFE78504
#define BRCM_REG_TEMP_SNR0_STAT 0xFFE78538
#define BRCM_REG_CHIP_ID 0xFFF00000
#define BRCM_REG_CHIP_REVID 0xFFF00004
#define BRCM_REG_SBR_ID 0xFFF00008
#define BRCM_REG_FLASH_VER 0x100005f8
#define BRCM_REG_TEC_ERROR_STATUS 0x6080085c
#define BRCM_REG_SWITCH_ERROR_STATUS_6 0x60800738
/* Boardcom AXI value */
#define BRCM_VAL_TEMP_SNR0_CTL_RESET 0x000653E8

typedef enum pex_dev { pex_dev_atlas1, pex_dev_atlas2, pex_dev_unknown } pex_dev_t;

typedef enum pex_w_r_mode {
	pex_do_write,
	pex_do_read,
} pex_w_r_mode_t;

typedef enum pex_access {
	pex_access_temp,
	pex_access_adc,
	pex_access_id,
	pex_access_rev_id,
	pex_access_sbr_ver,
	pex_access_flash_ver,
	pex_access_unknown
} pex_access_t;

enum pex_api_ret {
	pex_api_success,
	pex_api_unspecific_err,
	pex_api_mutex_err,
};

/* sensor offset */
enum pex_sensor_offset {
	PEX_TEMP,
	PEX_ADC,
};

typedef struct {
	uint8_t idx;
	uint8_t bus;
	uint8_t address;
	uint32_t axi_reg;
	uint32_t axi_data;
} pex89000_i2c_msg_t;

typedef struct {
	uint8_t idx; // Create index based on init variable
	struct k_mutex mutex;
	pex_dev_t pex_type;
	sys_snode_t node; // linked list node
} pex89000_unit;

/* Note: Could be used only after pex89000 sensor init successed */
uint8_t pex_write_read(pex89000_i2c_msg_t *pex_msg, pex_w_r_mode_t key);
uint8_t pex_access_engine(uint8_t bus, uint8_t addr, uint8_t idx, pex_access_t key, uint32_t *resp);
uint8_t pex89000_init(uint8_t sensor_num);

#endif

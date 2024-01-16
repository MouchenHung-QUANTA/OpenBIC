/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NVIDIA_H
#define NVIDIA_H

#include "plat_def.h"
#ifdef ENABLE_NVIDIA

#include <stdint.h>
#include "pldm_monitor.h"

#define SATMC_NUMERIC_SENSOR_MAX_COUNT 10

#define SMBPBI_OPCODE_GET_TEMP_SINGLE 0x02
#define SMBPBI_OPCODE_GET_TEMP_EXT 0x03

enum nv_satmc_sensor_num_table {
	/* DIMM sensor 1 ~ 70 */
	NV_SATMC_SENSOR_NUM_PWR_VDD_CPU = 0x0010,
	NV_SATMC_SENSOR_NUM_VOL_VDD_CPU,
	NV_SATMC_SENSOR_NUM_PWR_VDD_SOC = 0x0020,
	NV_SATMC_SENSOR_NUM_VOL_VDD_SOC,
	NV_SATMC_SENSOR_NUM_PWR_MODULE = 0x0030,
	NV_SATMC_SENSOR_NUM_ENG_MODULE,
	NV_SATMC_SENSOR_NUM_PWR_GRACE = 0x0040,
	NV_SATMC_SENSOR_NUM_ENG_GRACE,
	NV_SATMC_SENSOR_NUM_PWR_TOTAL_MODULE,
	NV_SATMC_SENSOR_NUM_ENG_TOTAL_MODULE,
	NV_SATMC_SENSOR_NUM_CNT_PAGE_RETIRE = 0x0050,
	NV_SATMC_SENSOR_NUM_TMP_GRACE = 0x00A0,
	NV_SATMC_SENSOR_NUM_TMP_GRACE_LIMIT,
	NV_SATMC_SENSOR_NUM_FRQ_MEMORY = 0x00B0,
	NV_SATMC_SENSOR_NUM_FRQ_MAX_CPU = 0x00C0,
};

#define NV_SMBPBI_CMD_STAT_REG 0x5C
#define NV_SMBPBI_DATA_REG 0x5D

/* 7bit addr (from spec SMBPBI-for-NV-Baseboard v21) */
#define NV_FPGA_ADDR 0x60
#define NV_HMC_ADDR 0x54
#define NV_GPU1_ADDR 0x4C
#define NV_GPU2_ADDR 0x4D
#define NV_GPU3_ADDR 0x4E
#define NV_GPU4_ADDR 0x4F
#define NV_GPU5_ADDR 0x44
#define NV_GPU6_ADDR 0x45
#define NV_GPU7_ADDR 0x46
#define NV_GPU8_ADDR 0x47

#define NV_SMBPBI_STATUS_SUCCESS 0x1F

enum nv_device_id {
	NV_FPGA,
	NV_HMC,
	NV_GPU1,
	NV_GPU2,
	NV_GPU3,
	NV_GPU4,
	NV_GPU5,
	NV_GPU6,
	NV_GPU7,
	NV_GPU8,
};

struct nv_smbpbi_cmd_stat_reg {
	uint8_t opcode;
	uint8_t arg1;
	uint8_t arg2;
	struct {
		uint8_t status : 5;
		uint8_t rsvd : 2;
		uint8_t cmd_exe : 1;
	};
} __attribute__((packed));

/* Y = (mX + b) * 10^r */
struct nv_satmc_sensor_parm {
	uint16_t nv_satmc_sensor_id;
	pldm_sensor_pdr_parm cal_parm;
};

extern struct nv_satmc_sensor_parm satmc_sensor_cfg_list[];
extern const int SATMC_SENSOR_CFG_LIST_SIZE;

void nv_satmc_pdr_collect(uint8_t type, uint8_t *pdr_buff, uint16_t pdr_buff_len);
bool nv_smbpbi_access(uint8_t bus, uint8_t dev_id, uint8_t opcode, uint8_t arg1, uint8_t arg2, uint32_t *val);

#endif /* ENABLE_NVIDIA */

#endif /* NVIDIA_H */

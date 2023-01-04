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

#ifndef lcmx0203_H
#define lcmx0203_H

#include "stdint.h"

typedef enum lattice_dev_type {
	LATTICE_LCMX02_2000HC,
	LATTICE_LCMX02_4000HC,
	LATTICE_LCMX02_7000HC,
	LATTICE_LCMX03_2100C,
	LATTICE_LCMX03_4300C,
	LATTICE_LCMX03_9400C,
	LATTICE_LFMNX_50,
	LATTICE_UNKNOWN,
} lattice_dev_type_t;

enum { CPLD_TAR_I2C = 0x01,
       CPLD_TAR_JTAG = 0x02,
};

struct source_config {
	uint8_t select_src_protocol; //IPMB or PLDM
	/* for pldm update */
	void *mctp_p;
	void *ext_params;
	/* for ipmb update */
	ipmi_msg *ipmb_cfg;
};

struct cpld_i2c_cfg {
	uint8_t i2c_bus;
	uint8_t i2c_addr;
};

struct cpld_jtag_cfg {
	uint8_t jtag_bus;
	// TODO
};

struct target_config {
	lattice_dev_type_t type;
	uint8_t select_tar_inf; //i2c or jtag
};

struct lattice_img_config {
	unsigned long int QF;
	uint32_t *CF;
	uint32_t CF_Line;
	uint32_t *UFM;
	uint32_t UFM_Line;
	uint32_t Version;
	uint32_t CheckSum;
	uint32_t FEARBits;
	uint32_t FeatureRow;
};

struct lattice_dev_config;

/* routing in platform code */
typedef struct lattice_usr_config {
	struct target_config tar_cfg; //get from user
	struct lattice_dev_config *dev_cfg; //get from platform table
	struct lattice_img_config img_cfg; //get from source
} lattice_usr_config_t;

typedef bool (*cpld_i2C_update_func)(void *fw_update_param);
typedef bool (*cpld_jtag_update_func)(void *fw_update_param);

struct lattice_dev_config {
	char name[20];
	uint32_t id;
	cpld_i2C_update_func cpld_i2C_update;
	cpld_jtag_update_func cpld_jtag_update;
};

uint8_t lattice_fwupdate(void *fw_update_param);

#endif

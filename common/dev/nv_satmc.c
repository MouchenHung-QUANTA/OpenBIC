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

#include "plat_def.h"
#ifdef ENABLE_NVIDIA

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <logging/log.h>
#include "libutil.h"
#include "sensor.h"
#include "pldm.h"
#include "hal_i2c.h"
#include "nvidia.h"
#include "pdr.h"

LOG_MODULE_REGISTER(nv_satmc);

struct nv_satmc_sensor_parm satmc_sensor_cfg_list[] = {
	{NV_SATMC_SENSOR_NUM_PWR_VDD_CPU, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_VOL_VDD_CPU, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_PWR_VDD_SOC, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_VOL_VDD_SOC, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_PWR_MODULE, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_ENG_MODULE, {0x3f800000, 0, 0}},
	{NV_SATMC_SENSOR_NUM_PWR_GRACE, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_ENG_GRACE, {0x3f800000, 0, 0}},
	{NV_SATMC_SENSOR_NUM_PWR_TOTAL_MODULE, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_ENG_TOTAL_MODULE, {0x3f800000, 0, 0}},
	{NV_SATMC_SENSOR_NUM_CNT_PAGE_RETIRE, {0x3f800000, 0, 0}},
	{NV_SATMC_SENSOR_NUM_TMP_GRACE, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_TMP_GRACE_LIMIT, {0x3f800000, 0, 0xfd}},
	{NV_SATMC_SENSOR_NUM_FRQ_MEMORY, {0x3f800000, 0, 0x3}},
	{NV_SATMC_SENSOR_NUM_FRQ_MAX_CPU, {0x3f800000, 0, 0}},
};

const int SATMC_SENSOR_CFG_LIST_SIZE = ARRAY_SIZE(satmc_sensor_cfg_list);

pldm_sensor_pdr_parm *find_sensor_parm_by_id(uint16_t sensor_id)
{
	for (int i=0; i<SATMC_SENSOR_CFG_LIST_SIZE; i++) {
		if (satmc_sensor_cfg_list[i].nv_satmc_sensor_id == sensor_id)
			return &satmc_sensor_cfg_list[i].cal_parm;
	}

	return NULL;
}

uint8_t nv_satmc_read(sensor_cfg *cfg, int *reading)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(reading, SENSOR_UNSPECIFIED_ERROR);
	CHECK_NULL_ARG_WITH_RETURN(cfg->init_args, SENSOR_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		LOG_ERR("sensor num: 0x%x is invalid", cfg->num);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	nv_satmc_init_arg *init_arg = (nv_satmc_init_arg *)cfg->init_args;

	if (init_arg->is_init == false) {
		pldm_sensor_pdr_parm *parm_cfg = find_sensor_parm_by_id(init_arg->sensor_id);
		if (!parm_cfg) {
			LOG_WRN("SatMC sensor #0x%x PDR not ready", init_arg->sensor_id);
			return SENSOR_UNSPECIFIED_ERROR;
		}
		init_arg->parm = *parm_cfg;
		init_arg->is_init = true;
	}

	mctp *mctp_inst = NULL;
	mctp_ext_params ext_params = { 0 };
	if (get_mctp_info_by_eid(init_arg->endpoint, &mctp_inst, &ext_params) == false) {
		LOG_ERR("Failed to get mctp info by eid 0x%x", init_arg->endpoint);
		return SENSOR_FAIL_TO_ACCESS;
	}

	uint8_t resp_buf[10] = { 0 };
	uint8_t req_len = sizeof(struct pldm_get_sensor_reading_req);
	struct pldm_get_sensor_reading_req req = { 0 };
	req.sensor_id = init_arg->sensor_id;
	req.rearm_event_state = 0;

	uint16_t resp_len =
		pldm_platform_monitor_read(mctp_inst, ext_params,
					   PLDM_MONITOR_CMD_CODE_GET_SENSOR_READING,
					   (uint8_t *)&req, req_len, resp_buf, sizeof(resp_buf));

	if (resp_len == 0) {
		LOG_ERR("Failed to get SatMC sensor #0x%x reading", init_arg->sensor_id);
		return SENSOR_FAIL_TO_ACCESS;
	}

	struct pldm_get_sensor_reading_resp *res = (struct pldm_get_sensor_reading_resp *)resp_buf;

	if (res->completion_code != PLDM_SUCCESS) {
		LOG_ERR("Get SatMC sensor #%04x with bad cc 0x%x", init_arg->sensor_id,
			res->completion_code);
		return SENSOR_FAIL_TO_ACCESS;
	}

	if (res->sensor_operational_state != PLDM_SENSOR_ENABLED) {
		LOG_WRN("SatMC sensor #%04x in abnormal op state 0x%x", init_arg->sensor_id,
			res->sensor_operational_state);
		return SENSOR_NOT_ACCESSIBLE;
	}

	LOG_DBG("SatMC sensor#0x%04x", init_arg->sensor_id);
	LOG_HEXDUMP_DBG(res->present_reading, resp_len - 7, "");

	float val =
	pldm_sensor_cal(res->present_reading, resp_len - 7, res->sensor_data_size, init_arg->parm);

	LOG_INF("SatMC sensor#0x%04x --> %.02f", init_arg->sensor_id, val);

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;

	return SENSOR_READ_SUCCESS;
}

uint8_t nv_satmc_init(sensor_cfg *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, SENSOR_INIT_UNSPECIFIED_ERROR);

	if (cfg->num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	cfg->read = nv_satmc_read;
	return SENSOR_INIT_SUCCESS;
}

#endif

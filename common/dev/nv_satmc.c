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
//#ifdef ENABLE_NVIDIA

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

PDR_numeric_sensor nv_satmc_numeric_sensor_pdr[SATMC_NUMERIC_SENSOR_MAX_COUNT] = {0};

static uint8_t byte_trans(pldm_sensor_readings_data_type_t type, uint8_t *buff, uint32_t *ret_buff)
{
	CHECK_NULL_ARG_WITH_RETURN(buff, 0);
	CHECK_NULL_ARG_WITH_RETURN(ret_buff, 0);

	uint8_t byte_size = 0;

	switch (type)
	{
	case PLDM_SENSOR_DATA_SIZE_UINT8:
	case PLDM_SENSOR_DATA_SIZE_SINT8:
		*ret_buff = buff[0];
		byte_size = 1;
		break;

	case PLDM_SENSOR_DATA_SIZE_UINT16:
	case PLDM_SENSOR_DATA_SIZE_SINT16:
		*ret_buff = buff[0] | (buff[1] << 8);
		byte_size = 2;
		break;

	case PLDM_SENSOR_DATA_SIZE_UINT32:
	case PLDM_SENSOR_DATA_SIZE_SINT32:
		*ret_buff = buff[0] | (buff[1] << 8) | (buff[2] << 16) | (buff[3] << 24);
		byte_size = 4;
		break;
	
	default:
		LOG_ERR("Invalid data size type 0x%x", type);
		break;
	}

	return byte_size;
}

pldm_sensor_pdr_parm *find_sensor_parm_by_id(uint16_t sensor_id)
{
	for (int i=0; i<SATMC_SENSOR_CFG_LIST_SIZE; i++) {
		if (satmc_sensor_cfg_list[i].is_pdr_get == false)
			continue;
		if (satmc_sensor_cfg_list[i].nv_satmc_sensor_id == sensor_id)
			return &satmc_sensor_cfg_list[i].cal_parm;
	}

	return NULL;
}

void nv_satmc_pdr_collect(uint8_t type, uint8_t *pdr_buff, uint16_t pdr_buff_len)
{
	CHECK_NULL_ARG(pdr_buff);

	static int pdr_idx;

	switch (type)
	{
	case PLDM_NUMERIC_SENSOR_PDR:
		if (pdr_idx >= SATMC_NUMERIC_SENSOR_MAX_COUNT) {
			LOG_WRN("Numeric Sensor PDR collect out of limit %d", SATMC_NUMERIC_SENSOR_MAX_COUNT);
			return;
		}

		int cur_p_idx = 0;

		// PDR_common_header +  PLDM_terminus_handle ~ minus_tolerance
		memcpy(&nv_satmc_numeric_sensor_pdr[pdr_idx], (PDR_numeric_sensor *)pdr_buff, sizeof(PDR_common_header) + 35);
		cur_p_idx += (sizeof(PDR_common_header) + 35);

		// hysteresis
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].sensor_data_size, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].hysteresis);

		// supported_thresholds ~ update_interval
		memcpy(&nv_satmc_numeric_sensor_pdr[pdr_idx].supported_thresholds, &pdr_buff[cur_p_idx], 10);
		cur_p_idx += 10;

		// max_readable ~ min_readable
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].sensor_data_size, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].max_readable);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].sensor_data_size, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].min_readable);

		// range_field_format ~ range_field_support
		memcpy(&nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], 2);
		cur_p_idx += 2;

		// nominal_value ~ fatal_low
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].nominal_value);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].normal_max);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].normal_min);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].warning_high);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].warning_low);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].critical_high);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].critical_low);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].fatal_high);
		cur_p_idx += byte_trans(nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format, &pdr_buff[cur_p_idx], &nv_satmc_numeric_sensor_pdr[pdr_idx].fatal_low);

		LOG_INF("           ~~~~~~~~ numeric sensor pdr no.%d ~~~~~~~", pdr_idx);
		LOG_INF("           |* record handle: 0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].pdr_common_header.record_handle);
		LOG_INF("           |* sensor id:     0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].sensor_id);
		LOG_INF("           |* unit modfier:  %d", nv_satmc_numeric_sensor_pdr[pdr_idx].unit_modifier);
		LOG_INF("           |* m:             %lf", nv_satmc_numeric_sensor_pdr[pdr_idx].resolution);
		LOG_INF("           |* b:             %lf", nv_satmc_numeric_sensor_pdr[pdr_idx].offset);
		LOG_INF("           |* r:             %d", nv_satmc_numeric_sensor_pdr[pdr_idx].accuracy);
		LOG_INF("           |* sensor d size: 0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].sensor_data_size);
		LOG_INF("           |* range f size:  0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_format);
		LOG_INF("           |* range f sup:   0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].range_field_support);
		LOG_INF("           |* norm max:      0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].normal_max);
		LOG_INF("           |* norm min:      0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].normal_min);
		LOG_INF("           |* fatal hi:      0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].fatal_high);
		LOG_INF("           |* critical hi:   0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].critical_high);
		LOG_INF("           |* warning hi:    0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].warning_high);
		LOG_INF("           |* warning lo:    0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].warning_low);
		LOG_INF("           |* critical lo:   0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].critical_low);
		LOG_INF("           |* fatal lo:      0x%x", nv_satmc_numeric_sensor_pdr[pdr_idx].fatal_low);
		LOG_INF("           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

		// parsing SatMC parm list
		for (int i = 0; i < SATMC_SENSOR_CFG_LIST_SIZE; i++) {
			if (satmc_sensor_cfg_list[i].nv_satmc_sensor_id == nv_satmc_numeric_sensor_pdr[pdr_idx].sensor_id) {
				satmc_sensor_cfg_list[i].cal_parm.ofst = nv_satmc_numeric_sensor_pdr[pdr_idx].offset;
				satmc_sensor_cfg_list[i].cal_parm.resolution = nv_satmc_numeric_sensor_pdr[pdr_idx].resolution;
				satmc_sensor_cfg_list[i].cal_parm.unit_modifier = nv_satmc_numeric_sensor_pdr[pdr_idx].unit_modifier;
				satmc_sensor_cfg_list[i].is_pdr_get = true;
			}	
		}

		pdr_idx++;
		break;

	default:
		break;
	}
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

//#endif

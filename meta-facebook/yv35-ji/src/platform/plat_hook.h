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

#ifndef PLAT_HOOK_H
#define PLAT_HOOK_H

#include "sensor.h"

/**************************************************************************************************
 * INIT ARGS
**************************************************************************************************/
extern adc_asd_init_arg ast_adc_init_args[];
extern mp5990_init_arg mp5990_init_args[];
#ifdef ENABLE_NVIDIA
extern nv_satmc_init_arg satmc_init_args[];
extern nv_smbpbi_init_arg smbpbi_init_args[];
#endif
/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS
 **************************************************************************************************/

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
bool pre_nvme_read(sensor_cfg *cfg, void *args);
bool pre_vol_bat3v_read(sensor_cfg *cfg, void *args);
bool post_vol_bat3v_read(sensor_cfg *cfg, void *args, int *reading);
bool post_mp5990_cur_read(sensor_cfg *cfg, void *args, int *reading);
bool post_mp5990_pwr_read(sensor_cfg *cfg, void *args, int *reading);

#endif

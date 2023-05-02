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

#ifndef _PLAT_PLDM_h
#define _PLAT_PLDM_h

#include "mctp.h"

typedef struct _bridge_store {
	mctp *mctp_inst;
	mctp_ext_params ext_params;
} bridge_store;

bool pldm_request_msg_need_bypass(uint8_t *buf, uint32_t len);
bool pldm_save_mctp_inst_from_ipmb_req(void *mctp_inst, uint8_t inst_num,
				       mctp_ext_params ext_params);
bridge_store *pldm_find_mctp_inst_by_inst_id(uint8_t inst_num);
bool pldm_send_ipmb_rsp(ipmi_msg *msg);

#endif /* _PLAT_PLDM_h */

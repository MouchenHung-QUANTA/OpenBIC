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

#include "info_shell.h"
#include "plat_version.h"
#include "util_sys.h"
#include "mctp_vdm_pci_brcm.h"
#include "plat_mctp.h"

#ifndef CONFIG_BOARD
#define CONFIG_BOARD "unknown"
#endif

#define RTOS_TYPE "Zephyr"

int cmd_info_print(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");
	shell_print(shell, "* NAME:          Platform command");
	shell_print(shell, "* DESCRIPTION:   Commands that could be used to debug or validate.");
	shell_print(shell, "* DATE/VERSION:  none");
	shell_print(shell, "* CHIP/OS:       %s - %s", CONFIG_BOARD, RTOS_TYPE);
	shell_print(shell, "* Note:          none");
	shell_print(shell, "------------------------------------------------------------------");
	shell_print(shell, "* PLATFORM:      %s-%s", PLATFORM_NAME, PROJECT_NAME);
	shell_print(shell, "* BOARD ID:      %d", BOARD_ID);
	shell_print(shell, "* STAGE:         %d", PROJECT_STAGE);
	shell_print(shell, "* SYSTEM:        %d", get_system_class());
	shell_print(shell, "* FW VERSION:    %d.%d", FIRMWARE_REVISION_1, FIRMWARE_REVISION_2);
	shell_print(shell, "* FW DATE:       %x%x.%x.%x", BIC_FW_YEAR_MSB, BIC_FW_YEAR_LSB,
		    BIC_FW_WEEK, BIC_FW_VER);
	shell_print(shell, "* FW IMAGE:      %s.bin", CONFIG_KERNEL_BIN_NAME);
	shell_print(
		shell,
		"========================{SHELL COMMAND INFO}========================================");

	shell_print(shell, "-----------------------------------------------------");
	struct _get_fw_rev_resp *rsp;
	struct _get_fw_rev_req req;
	req.rserv = 0x0000;
	req.switch_id = 0x0000;
	for (int i = 0; i < 4; i++) {
		rsp = NULL;
		rsp = mctp_vd_pci_access(i, &req, SM_API_CMD_FW_REV);
		if (rsp == NULL) {
			shell_error(shell, "PEX %d get fw revision failed!", i);
			continue;
		}

		shell_print(shell, "[ PEX %d get revision ]", i);
		shell_print(shell, "* fw version: %d.%d.%d.%d", rsp->FwVer.Field.Major,
			    rsp->FwVer.Field.Minor, rsp->FwVer.Field.Dev, rsp->FwVer.Field.Unit,
			    rsp->FwVer.Field.Dev);
		shell_print(shell, "* SM api version: %d.%d", rsp->SmApiVer.Field.Major,
			    rsp->SmApiVer.Field.Minor);
	}
#if 1
	sm_switch_attr sw_attr[4];
	memset(sw_attr, 0, sizeof(sm_switch_attr));

	shell_print(shell, "-----------------------------------------------------");
	struct _get_sw_attr_resp rsp1;
	for (int i = 0; i < 4; i++) {
		if (mctp_vd_pci_get_sw_attr(i, &rsp1) == false) {
			shell_error(shell, "PEX %d get sw attributes failed!", i);
			continue;
		}

		shell_print(shell, "[ PEX %d get sw attributes ]", i);
		shell_print(shell, "* Switch id:          %.4xh", rsp1.SwAttr.SwProp.SwitchID);
		shell_print(shell, "* Chip type:          %.4xh", rsp1.SwAttr.SwProp.ChipType);
		shell_print(shell, "* Chip id:            %.4xh", rsp1.SwAttr.SwProp.ChipID);
		shell_print(shell, "* Chip revision:      %xh", rsp1.SwAttr.SwProp.ChipRev);
		shell_print(shell, "* Station mask:       %xh", rsp1.SwAttr.SwProp.StnMask);
		shell_print(shell, "* Station count:      %d", rsp1.SwAttr.SwProp.StnCount);
		shell_print(shell, "* Port per station:   %d", rsp1.SwAttr.SwProp.PortsPerStn);
		shell_print(shell, "* iSSW/BSW port num:  %d", rsp1.SwAttr.SwProp.MgmtPortNum);
		shell_print(shell, "* Flags:              %xh", rsp1.SwAttr.SwProp.Flags);

		for (int stn_idx = 0; stn_idx < PMG_MAX_STN; stn_idx++) {
			shell_print(shell, "* Station%d:", stn_idx);
			shell_print(shell, "  * flags               %xh:",
				    rsp1.SwAttr.SwProp.Stn[stn_idx].Flags);
			shell_print(shell, "  * Active port count:  %d",
				    rsp1.SwAttr.SwProp.Stn[stn_idx].ActivePortCount);
			shell_print(shell, "  * Config:             %.2xh",
				    rsp1.SwAttr.SwProp.Stn[stn_idx].Config);
		}

		memcpy(sw_attr + i, &rsp1.SwAttr, sizeof(sm_switch_attr));
	}

	shell_print(shell, "-----------------------------------------------------");
	struct _get_port_attr_resp *rsp5;
	struct _get_port_attr_req req5;
	for (int i = 0; i < 4; i++) {
		shell_print(shell, "[ PEX %d get port attributes ]", i);
		for (int port_idx = 0; port_idx < PMG_MAX_PORT; port_idx++) {
			req5.Port_gid.u.GID.SwitchNum =
				sw_attr[i].SwProp.SwitchID.u.DN.Number & 0x7F;
			req5.Port_gid.u.GID.AddressType = 0;
			req5.Port_gid.u.GID.Domain = sw_attr[i].SwProp.SwitchID.u.DN.Domain;
			req5.Port_gid.u.GID.Bus = 0;
			req5.Port_gid.u.GID.DevIden.port = port_idx;

			rsp5 = NULL;
			rsp5 = mctp_vd_pci_access(i, &req5, SM_API_CMD_GET_PORT_ATTR);
			if (rsp5 == NULL) {
				shell_error(shell, "PEX %d get port %d attributes failed!", i,
					    port_idx);
				continue;
			}

			shell_print(shell, "  [ Port %d ]", port_idx);
			shell_print(shell, "    * Port number:          %.4xh",
				    rsp5->PortAttr.PortNum);
			shell_print(shell, "    * GID:                  %.4xh", rsp5->PortAttr.GID);
			shell_print(shell, "    * Type:                 %s",
				    (rsp5->PortAttr.Type == PMG_SW_PORT_TYPE_DS) ?
					    "DS" :
					    (rsp5->PortAttr.Type == PMG_SW_PORT_TYPE_FABRIC) ?
					    "FABRIC" :
					    (rsp5->PortAttr.Type == PMG_SW_PORT_TYPE_MGMT) ?
					    "MFMT" :
					    (rsp5->PortAttr.Type == PMG_SW_PORT_TYPE_HOST) ?
					    "HOST" :
					    "N/A");
		}
	}

	shell_print(shell, "-----------------------------------------------------");
	struct _get_sw_temp_resp rsp2;
	for (int i = 0; i < 4; i++) {
		if (mctp_vd_pci_get_sw_temp(i, &rsp2) == false) {
			shell_error(shell, "PEX %d get sw temp failed!", i);
			continue;
		}

		shell_print(shell, "[ PEX %d get sw temperature ]", i);
		shell_print(shell, "* status:  %xh", rsp2.Status);
		shell_print(shell, "* val:     %.4xh", rsp2.TempInCelsius);
	}

	shell_print(shell, "-----------------------------------------------------");
	struct _sm_sw_mfg_info_resp rsp3;
	for (int i = 0; i < 4; i++) {
		if (mctp_vd_pci_get_mfg_info(i, &rsp3) == false) {
			shell_error(shell, "PEX %d get mfg info failed!", i);
			continue;
		}

		shell_print(shell, "[ PEX %d get attributes ]", i);
		shell_print(shell, "* Chip secure:         %xh", rsp3.SwMfgInfo.ChipSecure);
		shell_print(shell, "* Chip secure ver num: %xh", rsp3.SwMfgInfo.ChipSecureVN);
		shell_print(shell, "* Chip revision level: %xh", rsp3.SwMfgInfo.ChipRev);
		shell_print(shell, "* Chip id:             %.2xh", rsp3.SwMfgInfo.ChipID);
		shell_print(shell, "* Chip type:           %.2xh", rsp3.SwMfgInfo.ChipType);
		shell_print(shell, "* Lane numbers:        %.2xh", rsp3.SwMfgInfo.NumLanes);
		shell_print(shell, "* Vendor id:");
		shell_hexdump(shell, rsp3.SwMfgInfo.VendorID, HALI_MFG_CONFIG_VENDOR_ID_LEN);
		shell_print(shell, "* Product id:");
		shell_hexdump(shell, rsp3.SwMfgInfo.ProductID, HALI_MFG_CONFIG_PRODUCT_ID_LEN);
		shell_print(shell, "* Product revsion level:");
		shell_hexdump(shell, rsp3.SwMfgInfo.ProdRevLevel,
			      HALI_MFG_CONFIG_PRODUCT_REV_LEVEL_LEN);
		shell_print(shell, "* Vendor spec data:");
		shell_hexdump(shell, rsp3.SwMfgInfo.VendSpecData,
			      HALI_MFG_CONFIG_VENDOR_SPECIFIC_LEN);
	}
#endif
	return 0;
}

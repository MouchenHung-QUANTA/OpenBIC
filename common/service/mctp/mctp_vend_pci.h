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

#ifndef _MCTP_VEND_PCI_H
#define _MCTP_VEND_PCI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mctp.h"
#include <stdint.h>
#include <zephyr.h>

#define BRDCM_ID 0x1000
#define MCTP_VEND_PCI_INST_ID_MASK 0x3F
#define MAX_MCTP_VEND_PCI_PAYLOAD_LEN 0x20
#define MCTP_VEND_PCI_MSG_TIMEOUT_MS 5000
#define MCTP_VEND_PCI_MSG_RETRY 3
#define PMG_MCPU_MSG_HEADER_VERSION (1)
#define MCTP_BYTES_TO_DWORDS( Value )               ((Value) >> 2)

// API Return Code Values
typedef enum _PEXSM_STATUS_CODE {
	PEXSM_STATUS_SUCCESS = 0, /**< Success */
	PEXSM_STATUS_FAILED, /**< Failed */
	PEXSM_STATUS_UNSUPPORTED, /**< Unsupported */
	PEXSM_STATUS_NULL_PARAM, /**< Function parameter was NULL */
	PEXSM_STATUS_INVALID_ADDR, /**< Invalid address */
	PEXSM_STATUS_INVALID_DATA, /**< Invalid data */
	PEXSM_STATUS_NO_RESOURCE, /**< No resource available */
	PEXSM_STATUS_TIMEOUT, /**< Timeout */
	PEXSM_STATUS_IN_USE, /**< In use */
	PEXSM_STATUS_DISABLED, /**< Disabled */
	PEXSM_STATUS_PENDING, /**< Pending */
	PEXSM_STATUS_NOT_FOUND, /**< Not found */
	PEXSM_STATUS_INVALID_STATE, /**< Invalid state */
	PEXSM_STATUS_INVALID_PORT, /**< Invalid Port */
	PEXSM_STATUS_INVALID_OBJECT, /**< Invalid Object */
	PEXSM_STATUS_BUFFER_TOO_SMALL, /**< Supplied buffer too small for result */
	PEXSM_STATUS_INVALID_SIZE, /**< Invalid Size */
	PEXSM_STATUS_RETRY, /**< Retry */
	PEXSM_STATUS_ABORT, /**< Abort */

	PEXSM_STATUS_NO_DRIVER = 128, /**< No Driver Found */
	PEXSM_STATUS_NO_DEVICES, /**< No Switch Found */
	PEXSM_STATUS_INVALID_SLID, /**< Invalid SLID */
	PEXSM_STATUS_INVALID_DS_PORT, /**< Invalid Downstream port */
	PEXSM_STATUS_INVALID_HS_PORT, /**< Invalid Host port */
	PEXSM_STATUS_INVALID_STN, /**< Invalid Station */
	PEXSM_STATUS_STN_NOT_CFG, /**< Station not configured */
	PEXSM_STATUS_ADDRTYPE_UNSUPPORTED, /**< Invalid address type */
	PEXSM_STATUS_MEM_ALLOC_FAILED, /**< Memory allocation Failed */
	PEXSM_STATUS_SL_LOAD_FAILED, /**< Failed to load shim library */
	PEXSM_STATUS_SL_SYM_ADDR_FAILED, /**< Failed to get the symbol */
	PEXSM_STATUS_INCOMP_DRV_INTFC_VER, /**< Incompatible Driver interface version */
	PEXSM_STATUS_RECV_OVERRUN, /**< Recieved data more than expected */
	PEXSM_STATUS_RECV_ERROR, /**< Receive Error */
	PEXSM_STATUS_SEND_ERROR, /**< Send Error */
	PEXSM_STATUS_LENGTH_EXCEEDED, /**< Length Exceeded */
	PEXSM_STATUS_INVALID_PARAM, /**< Invalid parameter */
	PEXSM_STATUS_DRV_FAULT, /**< Driver is in fault state */
	PEXSM_STATUS_NOT_READY, /**< The bus or driver is not ready */
	PEXSM_STATUS_UNKNOWN, /**< Unknown Error */
	PEXSM_STATUS_UNSUPP_CUR_SM_API_VERSION, /**< Command not supported. The current 
                                                   firmware is using SM API version lesser than 5.0 */

	PEXSM_STATUS_DEFAULT = 0xFFFF /**< Default initialized value */
} PEXSM_STATUS_CODE;

enum mctp_vend_pci_completion_codes {
	MCTP_7E_CC_SUCCESS,
	MCTP_7E_CC_INVALID_PARAMS,
	MCTP_7E_CC_INVALID_PCIID,
	MCTP_7E_CC_LENGTH_EXCEEDED,
	MCTP_7E_CC_TIMEOUT,
	MCTP_7E_CC_PCIID_BUSY,
	MCTP_7E_CC_SEND_ERROR,
	MCTP_7E_CC_RECV_OVERRUN,
	MCTP_7E_CC_DRVR_FAULT,
	MCTP_7E_CC_RECV_ERROR,
	MCTP_7E_CC_INCOMPATIBLE_DRVR_INTFC_VER,
};

/** Enumeration of the PayloadIdentifier */
typedef enum _MCTP_PAYLOADID_ENUM {
	MCTP_PAYLOADID_COMMAND = 0x20, /*<< Command request or response */
	MCTP_PAYLOADID_SL_COMMAND = 0x30, /*<< SwitchLib command request or response */
	MCTP_PAYLOADID_PE_GENERAL = 0x90, /*<< A general error occurred handling the message */
	MCTP_PAYLOADID_PE_OOO = 0x91, /*<< A packet was received out of order */
	MCTP_PAYLOADID_PE_PEC_FAIL = 0x92, /*<< A received packet's PEC failed */
	MCTP_PAYLOADID_PE_INVALID_PARAM =
		0x93, /*<< There is an invalid parameter in received packet */
	MCTP_PAYLOADID_PE_BUSY_RETRY = 0x94, /*<< A complete response is not ready, retry */
	MCTP_PAYLOADID_PE_UNAVAILABLE = 0x95, /*<< Currently not used */
	MCTP_PAYLOADID_PE_AMT_INUSE = 0x96, /*<< The AppMsgTag given is already in use */
	MCTP_PAYLOADID_PE_AMT_DOES_NOT_EXIST = 0x97, /*<< The request AppMsgTag does not exist. */
	MCTP_PAYLOADID_PE_INVALID_VENDORID = 0x98, /*<< Invalid vendor ID */
	MCTP_PAYLOADID_ABORT = 0x99, /*<< Abort command with specified AppMsgTag */
	MCTP_PAYLOADID_RESET = 0xA0, /*<< Reset the MCTP layer, i.e. drop all messages */
	MCTP_PAYLOADID_SERVICE_ERR =
		0xFF, /*<< The MCTP service could not be properly handled, retry */
} MCTP_PAYLOADID_ENUM;

/** Defined MCPU message API commands */
typedef enum _SM_API_COMMANDS {
	SM_API_CMD_NOP = 0, /**< Command NOP - Will be dropped at the receiver */
	SM_API_CMD_FW_REV, /**< Command to retrieve the firwmare version of the remote MCPU */
	SM_API_CMD_ECHO, /**< Command to echo back a request payload as a response payload */
	SM_API_CMD_GET_SW_ATTR, /**< Command to get Switch Attributes */
	SM_API_CMD_GET_PORT_ATTR, /**< Command to get Port Attributes */
	SM_API_CMD_GET_HOST_INFO, /**< Command to get Host Information */
	SM_API_CMD_GET_PORT_ERR_CNTRS, /**< Command to get Port Error Counters */
	SM_API_CMD_RESET_PORT_ERR_CNTRS, /**< Command to Reset Port Error Counters */
	SM_API_CMD_GET_RAM_ERR_CNTRS, /**< Command to get RAM Error Counters */
	SM_API_CMD_RESET_RAM_ERR_CNTRS, /**< Command to Reset RAM Error Counters */
	SM_API_CMD_GET_SW_MODE, /**< Command to get the Switch operating mode */
	SM_API_CMD_REL_DS_PORT, /**< Command to release the DS port from Host */
	SM_API_CMD_ASSGN_DS_TO_HS_PORT, /**< Command to assign a Downstream port to a host port */
	SM_API_CMD_GET_SW_TEMP, /**< Command to get Switch (Chip) Temperature */
	SM_API_CMD_REL_DS_PORT_EX, /**< Command to release DS port from Hs port with Enhanced options */
	SM_API_CMD_ASSGN_DS_TO_HS_PORT_EX, /**< Command to assign DS port to Hs port with Enhanced options */
	SM_API_CMD_RW_BUFF_ACCSESS, /**< Command to get Access to RW Buffer */
	SM_API_CMD_GET_SW_MFG_INFO, /**< Command to get Switch Mfg Info */

	/* ALL API Commands above this line */
	SM_API_CMD_MAX
} SM_API_COMMANDS;

typedef enum _PMG_MCPU_MSG_ADDRESS_TYPE {
	PMG_MCPU_MSG_ADDRESS_TYPE_PCIE = 0, /**< Address type is PCIe.  Id is PCIe bus number */
	PMG_MCPU_MSG_ADDRESS_TYPE_NT =
		1, /**< Address type is synthetic NT (host).  Id is NT number */
	PMG_MCPU_MSG_ADDRESS_TYPE_MCTP = 2 /**< Address type is MCTP.  Id is MCTP EID */
} PMG_MCPU_MSG_ADDRESS_TYPE;

typedef enum _PMG_MCPU_MSG_TYPE {
	PMG_MCPU_MSG_TYPE_API = 0, /**< MCPU messaging specific API command set */
	PMG_MCPU_MSG_TYPE_FABRIC_EVENT =
		1, /**< MCPU messages which relate to fabric events (link change, etc) */
	PMG_MCPU_MSG_TYPE_CSR_RELAY =
		2, /**< MCPU messages used to manage proxying CSR TLPs through fabric */
	PMG_MCPU_MSG_TYPE_HAM = 3 /**< MCPU messages received from Host for Host-Arm Messages */
} PMG_MCPU_MSG_TYPE;

typedef enum {
	FIRST_PACKET,
	SUPP_PACKET,
} mctp_vend_pci_pkt_t;

/** MCTP VDM Message Request First Packet Header */
typedef struct __attribute__((packed)) {
	uint8_t msg_type : 7; /* 0x7E - Vendor defined message */
	uint8_t ic : 1; /* Indicates if this packet includes PEC */
	uint8_t vendor_id_hi; /* PCI Vendor ID in Big Endian notation */
	uint8_t vendor_id_low; /* PCI Vendor ID in Big Endian notation */
	uint8_t payload_id; /* Payload Identifier */
	uint16_t pkt_seq_cnt; /* 0-3 for high, and 8-15 for low bits */
	uint8_t app_msg_tag; /* Msg identifier for the BMC to */
	uint8_t cmd; /* Broadcom defined commands for MCTP Requests */
	uint16_t payload_len; /* Length of Payload in bytes */
	uint16_t resp_len; /* Expected Response length in bytes */
} mctp_vend_pci_req_first_hdr;

/** MCTP VDM Message Request Suppliment Packet Header */
typedef struct __attribute__((packed)) {
	uint8_t vendor_id_hi; /* PCI Vendor ID in Big Endian notation */
	uint8_t vendor_id_low; /* PCI Vendor ID in Big Endian notation */
	uint8_t payload_id; /* Payload Identifier */
	uint8_t rsv; /* Reserved */
	uint16_t pkt_seq_cnt; /* 0-3 for high, and 8-15 for low bits */
	uint8_t app_msg_tag; /* Msg identifier for the BMC to */
	uint8_t cmd; /* Broadcom defined commands for MCTP Requests */
} mctp_vend_pci_req_supp_hdr;

/** MCTP VDM Message Response First Packet Header */
typedef struct __attribute__((packed)) {
	uint8_t msg_type : 7; /* 0x7E - Vendor defined message */
	uint8_t ic : 1; /* Indicates if this packet includes PEC */
	uint8_t vendor_id_hi; /* PCI Vendor ID in Big Endian notation */
	uint8_t vendor_id_low; /* PCI Vendor ID in Big Endian notation */
	uint8_t payload_id; /* Payload Identifier */
	uint16_t pkt_seq_cnt; /* 0-3 for high, and 8-15 for low bits */
	uint8_t app_msg_tag; /* Msg identifier for the BMC to */
	uint8_t cc; /* The status of the requested command */
	uint16_t resp_len; /* Expected Response length in bytes */
	uint16_t residual_data_len; /* Remaining Response length in bytes for overruns */
} mctp_vend_pci_rsp_first_hdr;

/** MCTP VDM Message Response Suppliment Packet Header */
typedef struct __attribute__((packed)) {
	uint8_t vendor_id_hi; /* PCI Vendor ID in Big Endian notation */
	uint8_t vendor_id_low; /* PCI Vendor ID in Big Endian notation */
	uint8_t payload_id; /* Payload Identifier */
	uint8_t rsv; /* Reserved */
	uint16_t pkt_seq_cnt; /* 0-3 for high, and 8-15 for low bits */
	uint8_t app_msg_tag; /* Msg identifier for the BMC to */
	uint8_t cc; /* The status of the requested command */
} mctp_vend_pci_rsp_supp_hdr;

typedef struct __attribute__((packed)) {
	uint32_t msg_len : 5; /* Length of the message packet in dwords */
	uint32_t hdr_ver : 3; /* Version of the packet header format */
	uint32_t src_domain : 8; /* Source PCIe domain of the packet */
	uint32_t src_id : 8; /* Source ID of the packet.  Value defined by SrcAddrType */
	uint32_t src_addr_type : 3; /* Source address type of the packet */
	uint32_t src_type_specific : 5; /* Source address type specific information */

	uint32_t msg_type : 4; /* Message type (category) of the message packet */
	uint32_t msg_flags : 4; /* Message packet related flags */
	uint32_t dest_domain : 8; /* Destination PCIe domain of the packet */
	uint32_t dest_id : 8; /* Destination ID of the packet.  Value defined by DestAddrType */
	uint32_t dest_addr_type : 3; /* Destination address type of the packet */
	uint32_t dest_type_specific : 5; /* Destination address type specific information */

	uint32_t pkt_num : 8; /* Packet number for differentiating packets comprising an MCPU message */
	uint32_t req_tag : 8; /* Request tag for all packets comprising an MCPU message */
	uint32_t msg_type_specific : 16; /* Message type specific information.  Value defined by MsgType. */
} pmg_mcpu_msg_hdr;

typedef struct {
	mctp_vend_pci_req_first_hdr hdr;
	uint8_t *cmd_data;
	uint16_t cmd_data_len;
	mctp_ext_params ext_params;
	void (*recv_resp_cb_fn)(void *, uint8_t *, uint16_t);
	void *recv_resp_cb_args;
	uint16_t timeout_ms;
	void (*timeout_cb_fn)(void *);
	void *timeout_cb_fn_args;
} mctp_vend_pci_msg;

struct _get_fw_rev_req {
	uint16_t switch_id;
	uint16_t rserv;
} __attribute__((packed));

struct _get_fw_rev_resp {
	union {
		/** Structured access to firmware version. */
		struct {
			uint8_t Dev; /**< Development Version */
			uint8_t Unit; /**< Unit Version */
			uint8_t Minor; /**< Minor Version */
			uint8_t Major; /**< Major Version */
		} Field;
		uint32_t Word;
	} FwVer;

	union {
		/** Structured access to API version used in firmware */
		struct {
			uint8_t Reserved[2]; /**< Reserved to DWORD Align */
			uint8_t Minor; /**< Minor Version */
			uint8_t Major; /**< Major Version */
		} Field;
		uint32_t Word;
	} SmApiVer;
} __attribute__((packed));

typedef uint8_t (*mctp_vend_pci_cmd_fn)(void *, uint8_t *, uint16_t, uint8_t *, uint16_t *, void *);

typedef struct {
	uint8_t cmd_code;
	mctp_vend_pci_cmd_fn fn;
} mctp_vend_pci_handler_t;

uint16_t mctp_vend_pci_read(void *mctp_p, mctp_vend_pci_msg *msg, uint8_t *rbuf, uint16_t rbuf_len);
uint8_t mctp_vend_pci_send_msg(void *mctp_p, mctp_vend_pci_msg *msg, uint8_t *buff, uint16_t buff_len);

#ifdef __cplusplus
}
#endif

#endif /* _MCTP_VEND_PCI_H */

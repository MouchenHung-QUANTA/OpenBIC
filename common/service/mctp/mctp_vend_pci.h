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
#define MAX_MCTP_VEND_PCI_PAYLOAD_LEN 0x60
#define MCTP_VEND_PCI_MSG_TIMEOUT_MS 5000
#define MCTP_VEND_PCI_MSG_RETRY 3
#define PMG_MCPU_MSG_HEADER_VERSION (1)

#define PMG_MAX_PORT (128) // Max port number
#define PMG_MAX_STN 8 // Max stations in chip

/** Definitions copied from HAL header. Should remain in-sync if HAL is modified*/
/** Vendor Id length. */
#define HALI_MFG_CONFIG_VENDOR_ID_LEN (8)
/** Product Id length. */
#define HALI_MFG_CONFIG_PRODUCT_ID_LEN (16)
/** Vendor specific length. */
#define HALI_MFG_CONFIG_VENDOR_SPECIFIC_LEN (8)
/** Product revision level length. */
#define HALI_MFG_CONFIG_PRODUCT_REV_LEVEL_LEN (4)

#define MCTP_BYTES_TO_DWORDS(Value) ((Value) >> 2)

/** PMG_PORT_MASK_xx --> PMG_BITMASK conversions */
#define PMG_BITMASK_T(Name, Bits) uint32_t(Name)[((Bits) + 31) / 32]
#define PMG_PORT_MASK_T(Name) PMG_BITMASK_T((Name), PMG_MAX_PORT)

struct _pex_dev_info {
	uint8_t bus;
	uint8_t addr; // i2c slave address (8 bit)
	uint8_t ep;
};

// API Return Code Values
typedef enum _pexsm_status_code {
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
} pexsm_status_code;

/* HALI_ADC_STATUS enumerates values returned by various ADC APIs. */
typedef enum _hali_adc_status {
	HALI_ADC_STATUS_SUCCESS, // API operation successful
	HALI_ADC_STATUS_INIT_FAILED, // ADC initialization failed
	HALI_ADC_STATUS_INVALID_CONFIG, // Configuration parameters validation failed
	HALI_ADC_STATUS_DATA_NOT_READY, // No valid ADC reading available
	HALI_ADC_STATUS_NULL_POINTER, // Null pointer specified
	HALI_ADC_STATUS_INVALID_CHL, // Out of range channel input
	HALI_ADC_STATUS_INVALID_BLK_NUM, // Out of range block number
	HALI_ADC_STATUS_MUTEX_ERROR // Mutex error
} hali_adc_status;

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

/* MCTP VDM Message Request First Packet Header */
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
	mctp_vend_pci_req_first_hdr req_hdr;
	mctp_vend_pci_req_supp_hdr req_sup_hdr;
	mctp_vend_pci_rsp_first_hdr rsp_hdr;
	mctp_vend_pci_rsp_supp_hdr rsp_sup_hdr;
	uint8_t *cmd_data;
	uint16_t cmd_data_len;
	mctp_ext_params ext_params;
	void (*recv_resp_cb_fn)(void *, uint8_t *, uint16_t);
	void *recv_resp_cb_args;
	uint16_t timeout_ms;
	void (*timeout_cb_fn)(void *);
	void *timeout_cb_fn_args;
} mctp_vend_pci_msg;

/** Switch ID unique within fabric */
typedef struct _pmg_switch_id {
	union {
		uint16_t ID; /**< Switch ID */
		struct {
			uint8_t Number; /**< Chip number within domain (0-based) */
			uint8_t Domain; /**< Fabric domain number */
		} DN;
	} u;
} __packed pmg_switch_id;

/** Switch properties */
typedef struct _pmg_switch_prop {
	pmg_switch_id SwitchID; /**< Switch ID */
	uint16_t ChipType; /**< Chip type from HW device ID */
	uint16_t ChipID; /**< Chip ID */
	uint8_t ChipRev; /**< Chip revision */
	uint8_t StnMask; /**< Mask to denote which stations enabled in chip */
	uint8_t StnCount; /**< Number of stations in chip */
	uint8_t PortsPerStn; /**< Number of ports per station */
	uint8_t MgmtPortNum; /**< Management(iSSW) / Upstream(BSW) port number */
	uint8_t Flags; /**< Switch-specific flags */
	struct {
		uint8_t Flags; /**< Station-specific flags */
		uint8_t ActivePortCount; /**< Number of active ports in station */
		uint16_t Config; /**< Station port configuration */
	} Stn[PMG_MAX_STN];
} __packed pmg_switch_prop;

typedef struct _sm_sw_mfg_info {
	uint8_t ChipSecure : 1; /**< 1: Secure, 0: Non-Secure */
	uint8_t ChipSecureVN : 7; /**< Chip Secure Version Number */
	uint8_t ChipRev; /**< Chip Rev Level */
	uint16_t ChipID; /**< Chip ID */
	uint16_t ChipType; /**< Chip Type */
	uint16_t NumLanes; /**< No. of lanes, 0xFFFF represents Unknown */
	uint32_t Reserved; /**< Reserved */
	uint8_t VendorID[HALI_MFG_CONFIG_VENDOR_ID_LEN]; /**< Vendor ID - String not NULL terminated */
	uint8_t ProductID
		[HALI_MFG_CONFIG_PRODUCT_ID_LEN]; /**< Product ID - String not NULL terminated */
	uint8_t ProdRevLevel
		[HALI_MFG_CONFIG_PRODUCT_REV_LEVEL_LEN]; /**< Product Revision Level - String not NULL terminated */
	uint8_t VendSpecData
		[HALI_MFG_CONFIG_VENDOR_SPECIFIC_LEN]; /**< Vendor Specific Data - String not NULL terminated */
} __packed sm_sw_mfg_info;

typedef struct _sm_switch_attr {
	pmg_switch_prop SwProp; /**< Switch Properties */
	PMG_PORT_MASK_T(ActivePortMask); /**< Mask of enabled ports */
	PMG_PORT_MASK_T(HostPortMask); /**< Mask of ports set as host type */
	PMG_PORT_MASK_T(FabricPortMask); /**< Mask of ports set as fabric type */
	PMG_PORT_MASK_T(DsPortMask); /**< Mask of ports set as downstream type */
} __packed sm_switch_attr;

struct _get_fw_rev_req {
	uint16_t switch_id;
	uint16_t rserv;
} __attribute__((packed));

struct _get_fw_rev_resp {
	union {
		struct {
			uint8_t Dev; /**< Development Version */
			uint8_t Unit; /**< Unit Version */
			uint8_t Minor; /**< Minor Version */
			uint8_t Major; /**< Major Version */
		} Field;
		uint32_t Word;
	} FwVer;

	union {
		struct {
			uint8_t Reserved[2]; /**< Reserved to DWORD Align */
			uint8_t Minor; /**< Minor Version */
			uint8_t Major; /**< Major Version */
		} Field;
		uint32_t Word;
	} SmApiVer;
} __attribute__((packed));

struct _get_sw_attr_req {
	uint16_t switch_id;
	uint16_t rserv;
} __attribute__((packed));

struct _get_sw_attr_resp {
	uint16_t Status; /**< Operation Status */
	uint16_t Reserved; /**< Reserved */
	sm_switch_attr SwAttr; /**< struct of type SM_SWITCH_ATTR */
} __attribute__((packed));

struct _sm_sw_mfg_info_req {
	uint16_t switch_id;
	uint16_t rserv;
} __attribute__((packed));

struct _sm_sw_mfg_info_resp {
	uint16_t Status; /**< Operation Status */
	uint16_t Reserved; /**< Reserved */
	sm_sw_mfg_info SwMfgInfo; /**< struct of type SM_SW_MFG_INFO */
} __attribute__((packed));

struct _get_sw_temp_req {
	uint16_t switch_id;
	uint16_t rserv;
} __attribute__((packed));

struct _get_sw_temp_resp {
	int32_t TempInCelsius; /**< Chip Temperature */
	hali_adc_status Status; /**< Operation Status */
} __attribute__((packed));

typedef uint8_t (*mctp_vend_pci_cmd_fn)(void *, uint8_t *, uint16_t, uint8_t *, uint16_t *, void *);

typedef struct {
	uint8_t cmd_code;
	mctp_vend_pci_cmd_fn fn;
} mctp_vend_pci_handler_t;

uint16_t mctp_vend_pci_read(void *mctp_p, mctp_vend_pci_msg *msg, uint8_t *rbuf, uint16_t rbuf_len);
uint8_t mctp_vend_pci_send_msg(void *mctp_p, mctp_vend_pci_msg *msg, uint8_t *buff,
			       uint16_t buff_len);
uint8_t mctp_vdm_pci_cmd_handler(void *mctp_p, uint8_t *buf, uint32_t len,
				 mctp_ext_params ext_params);

#ifdef __cplusplus
}
#endif

#endif /* _MCTP_VEND_PCI_H */

#ifndef PLAT_VERSION_H
#define PLAT_VERSION_H

#define PLATFORM_NAME "wc"
#define PROJECT_NAME "mainboard"
#define IANA_ID 0x009c9c
#define DEVICE_ID 0x00
#define DEVICE_REVISION 0x80
/*
 *  FIRMWARE REVISION_1 
 *  [bit 0-3] 
 *    board id: 
 *      0x01 WC
 *  [bit 4-7]
 *    stage: 
 *      0x00 POC
 *      0x01 EVT
 *      0x02 DVT
 *      0x03 PVT
 *      0x04 MP
 *  FIRMWARE_REVISION_2
 *    Count of release firmware at each stage.
 */
#define FIRMWARE_REVISION_1 0x01
#define FIRMWARE_REVISION_2 0x00
#define IPMI_VERSION 0x02
#define ADDITIONAL_DEVICE_SUPPORT 0xBF
#define PRODUCT_ID 0x0000
#define AUXILIARY_FW_REVISION 0x00000000

#define BIC_FW_YEAR_MSB 0x20
#define BIC_FW_YEAR_LSB 0x22
#define BIC_FW_WEEK 0x24
#define BIC_FW_VER 0x01
#define BIC_FW_platform_0 0x77 // char: w
#define BIC_FW_platform_1 0x63 // char: c
#define BIC_FW_platform_2 0x00 // char: '\0'

#endif

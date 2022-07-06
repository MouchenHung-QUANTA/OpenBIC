#include <stdio.h>
#include "cmsis_os2.h"
#include <string.h>
#include "plat_i2c.h"
#include "plat_ipmb.h"
#include "plat_ipmi.h"
#include "plat_class.h"

IPMB_config pal_IPMB_config_table[] = {
	// index, interface, channel, bus, channel_target_address, enable_status, self_address,
	// rx_thread_name, tx_thread_name
	{ BMC_IPMB_IDX, I2C_IF, BMC_IPMB, IPMB_BMC_BUS, BMC_I2C_ADDRESS, ENABLE, SELF_I2C_ADDRESS,
	  "RX_BMC_IPMB_TASK", "TX_BMC_IPMB_TASK" },
	{ ME_IPMB_IDX, I2C_IF, ME_IPMB, IPMB_ME_BUS, ME_I2C_ADDRESS, ENABLE, SELF_I2C_ADDRESS,
	  "RX_ME_IPMB_TASK", "TX_ME_IPMB_TASK" },
/* mcadd: TODO: Add if knowing what condition peer bmc connected */
#if 0
	{ PEER_BMC_IPMB_IDX, I2C_IF, PEER_BMC_IPMB, IPMB_PEER_BMC_BUS, PEER_BMC_I2C_ADDRESS, ENABLE,
	  SELF_I2C_ADDRESS, "RX_PEER_BMC_IPMB_TASK", "TX_PEER_BMC_IPMB_TASK" },
#endif
	{ RESERVED_IDX, RESERVED_IF, RESERVED, RESERVED_BUS, RESERVED_ADDRESS, DISABLE,
	  RESERVED_ADDRESS, "RESERVED_ATTR", "RESERVED_ATTR" },
};

bool pal_load_ipmb_config(void)
{
	/* mcadd: TODO: whether peer bmc relative to sku_id or project state */

	memcpy(&IPMB_config_table[0], &pal_IPMB_config_table[0], sizeof(pal_IPMB_config_table));
	return true;
};

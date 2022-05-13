#include "hal_gpio.h"
#include "hal_peci.h"
#include "power_status.h"
#include "util_sys.h"
#include "plat_class.h"
#include "plat_gpio.h"

SCU_CFG scu_cfg[] = {
	//register    value
	{ 0x7e6e24b0, 0x00c30000 }, { 0x7e6e2610, 0xffffffff }, { 0x7e6e2614, 0xffffffff },
	{ 0x7e6e2618, 0x30000000 }, { 0x7e6e261c, 0x00000F04 },
};

void pal_pre_init()
{
/* mcadd: TODO: temparary remove until CPLD spec received */
#if 0
	init_platform_config();
#endif
	disable_PRDY_interrupt();
	scu_init(scu_cfg, sizeof(scu_cfg) / sizeof(SCU_CFG));
}

void pal_post_init()
{
	init_me_firmware();
}

void pal_set_sys_status()
{
	set_DC_status(PWRGD_SYS_PWROK);
	set_DC_on_delayed_status();
	set_DC_off_delayed_status();
	// Scron: Replace FM_BIOS_POST_CMPLT_BMC_N by FM_BIOS_POST_CMPLT_BIC_N.
	set_post_status(FM_BIOS_POST_CMPLT_BIC_N);
	set_CPU_power_status(PWRGD_CPU_LVC3);
	set_post_thread();
	// Scron: Replace BIC_READY by FM_BIC_READY.
	set_sys_ready_pin(FM_BIC_READY);
}

#define DEF_PROJ_GPIO_PRIORITY 78

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME", &gpio_init, NULL, NULL, NULL,
	      POST_KERNEL, DEF_PROJ_GPIO_PRIORITY, NULL);

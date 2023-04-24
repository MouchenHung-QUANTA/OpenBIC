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

#include <stdlib.h>
#include "plat_isr.h"
#include "plat_gpio.h"
#include "plat_sensor_table.h"
#include "libipmi.h"
#include "power_status.h"
#include "ipmi.h"
#include "plat_class.h"
#include "plat_i2c.h"
#include "pmbus.h"
#include "kcs.h"
#include "pcc.h"
#include "libutil.h"
#include "logging/log.h"
#include "plat_def.h"
#include "plat_pmic.h"
#include "plat_apml.h"
#include "util_worker.h"

LOG_MODULE_REGISTER(plat_isr);

static void isr_dbg_print(uint8_t gpio_num)
{
	switch (gpio_cfg[gpio_num].int_type) {
	case GPIO_INT_EDGE_FALLING:
		LOG_INF("gpio[%-3d] isr type[fall] trigger 1 -> 0", gpio_num);
		break;

	case GPIO_INT_EDGE_RISING:
		LOG_INF("gpio[%-3d] isr type[rise] trigger 0 -> 1", gpio_num);
		break;

	case GPIO_INT_EDGE_BOTH:
		if (gpio_get(gpio_num))
			LOG_INF("gpio[%-3d] isr type[both] trigger 0 -> 1", gpio_num);
		else
			LOG_INF("gpio[%-3d] isr type[both] trigger 1 -> 0", gpio_num);
		break;

	default:
		LOG_WRN("gpio[%-3d] isr trigger unexpected", gpio_num);
		break;
	}
}

void ISR_CPU_PRSNT()
{
	isr_dbg_print(CPU0_BMC_GPIOA0_PRESENT_L);
}

void ISR_CPU_FAULT_ALERT()
{
	isr_dbg_print(S0_BMC_GPIOA2_FAULT_ALERT);
}

void ISR_CPU_JTAG_CMPL2()
{
	isr_dbg_print(JTAG_CMPL2_PD_BIC);
}

void ISR_MPRO_BOOT_OK()
{
	isr_dbg_print(S0_BMC_GPIOA5_FW_BOOT_OK);
}

void ISR_MPRO_HB()
{
	isr_dbg_print(S0_BMC_MPRO_HEARTBEAT);
}

void ISR_CPU_SHD_ACK()
{
	isr_dbg_print(S0_BMC_GPIOB0_SHD_ACK_L);
}

void ISR_CPU_REBOOT_ACK()
{
	isr_dbg_print(S0_BMC_GPIOB4_REBOOT_ACK_L);
}

void ISR_CPU_OVERTEMP()
{
	isr_dbg_print(S0_BMC_GPIOB6_OVERTEMP_L);
}

void ISR_CPU_HIGHTEMP()
{
	isr_dbg_print(S0_BMC_GPIOB7_HIGHTEMP_L);
}

void ISR_CPU_SYS_AUTH_FAIL()
{
	isr_dbg_print(S0_BMC_GPIOE1_SYS_AUTH_FAILURE_L);
}

void ISR_CPU_SPI_AUTH_FAIL()
{
	isr_dbg_print(CPLD_BMC_GPIOE2_S0_SPI_AUTH_FAIL_L);
}

void ISR_POST_COMPLETE()
{
	isr_dbg_print(FM_BIOS_POST_CMPLT_BIC_N);

	set_post_status(FM_BIOS_POST_CMPLT_BIC_N);
}

void ISR_VRHOT()
{
	isr_dbg_print(S0_BMC_GPIOF3_VRHOT_L);
}

void ISR_VRFAULT()
{
	isr_dbg_print(S0_BMC_GPIOF4_VRD_FAULT_L);
}

void ISR_AC_STATUS()
{
	isr_dbg_print(BTN_PWR_BUF_BMC_GPIOD3_L);
}

void ISR_SYS_RST_BMC()
{
	isr_dbg_print(BTN_SYS_RST_BMC_GPIOF7_L);
}

void ISR_CPU_SPI_ACCESS()
{
	isr_dbg_print(CPLD_SOC_SPI_NOR_ACCESS);
}

void ISR_SALT4()
{
	isr_dbg_print(BIC_SALT4_L);
}

void ISR_SALT7()
{
	isr_dbg_print(BIC_SALT7_L);
}

void ISR_SALT12()
{
	isr_dbg_print(BIC_SALT12_L);
}

void ISR_PLTRST()
{
	isr_dbg_print(RST_PLTRST_BIC_N);
}

K_WORK_DELAYABLE_DEFINE(set_DC_on_5s_work, set_DC_on_delayed_status);
#define DC_ON_5_SECOND 5
#define PROC_FAIL_START_DELAY_SECOND 10
#define READ_PMIC_CRITICAL_ERROR_MS 100
void ISR_DC_ON()
{
	/* TODO: Add postcode relatice code */
	set_DC_status(BMC_GPIOL1_SYS_PWRGD);
	if (get_DC_status() == true) {
		k_work_schedule(&set_DC_on_5s_work, K_SECONDS(DC_ON_5_SECOND));
	} else {
		if (k_work_cancel_delayable(&set_DC_on_5s_work) != 0) {
			LOG_ERR("Failed to cancel set dc on delay work.");
		}
		set_DC_on_delayed_status();
	}
}

void ISR_HSC_THROTTLE()
{
	common_addsel_msg_t sel_msg;
	static bool is_hsc_throttle_assert = false; // Flag for filt out fake alert
	if (1) {
		if (gpio_get(BMC_GPIOL1_SYS_PWRGD) == GPIO_LOW) {
			return;
		} else {
			if ((gpio_get(IRQ_HSC_ALERT1_N) == GPIO_HIGH) &&
			    (is_hsc_throttle_assert == true)) {
				sel_msg.event_type = IPMI_OEM_EVENT_TYPE_DEASSERT;
				is_hsc_throttle_assert = false;
			} else if ((gpio_get(IRQ_HSC_ALERT1_N) == GPIO_LOW) &&
				   (is_hsc_throttle_assert == false)) {
				sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
				is_hsc_throttle_assert = true;
			} else { // Fake alert
				return;
			}

			sel_msg.InF_target = BMC_IPMB;
			sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_SYS_STA;
			sel_msg.sensor_number = SENSOR_NUM_SYSTEM_STATUS;
			sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_PMBUSALERT;
			sel_msg.event_data2 = 0xFF;
			sel_msg.event_data3 = 0xFF;
			if (!common_add_sel_evt_record(&sel_msg)) {
				LOG_ERR("Failed to add HSC Throttle sel.");
			}
		}
	}
}

void ISR_MB_THROTTLE()
{
	/* FAST_PROCHOT_N glitch workaround
	 * FAST_PROCHOT_N has a glitch and causes BIC to record MB_throttle deassertion SEL.
	 * Ignore this by checking whether MB_throttle is asserted before recording the deassertion.
	 */
	static bool is_mb_throttle_assert = false;
	common_addsel_msg_t sel_msg;
	if (1 && get_DC_status()) {
		if ((gpio_get(FAST_PROCHOT_N) == GPIO_HIGH) && (is_mb_throttle_assert == true)) {
			sel_msg.event_type = IPMI_OEM_EVENT_TYPE_DEASSERT;
			is_mb_throttle_assert = false;
		} else if ((gpio_get(FAST_PROCHOT_N) == GPIO_LOW) &&
			   (is_mb_throttle_assert == false)) {
			sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
			is_mb_throttle_assert = true;
		} else {
			return;
		}
		sel_msg.InF_target = BMC_IPMB;
		sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_SYS_STA;
		sel_msg.sensor_number = SENSOR_NUM_SYSTEM_STATUS;
		sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_FIRMWAREASSERT;
		sel_msg.event_data2 = 0xFF;
		sel_msg.event_data3 = 0xFF;
		if (!common_add_sel_evt_record(&sel_msg)) {
			LOG_ERR("Failed to add MB Throttle sel.");
		}
	}
}

void ISR_SOC_THMALTRIP()
{
	common_addsel_msg_t sel_msg;
	if (1) {
		sel_msg.InF_target = BMC_IPMB;
		sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
		sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_SYS_STA;
		sel_msg.sensor_number = SENSOR_NUM_SYSTEM_STATUS;
		sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_THERMAL_TRIP;
		sel_msg.event_data2 = 0xFF;
		sel_msg.event_data3 = 0xFF;
		if (!common_add_sel_evt_record(&sel_msg)) {
			LOG_ERR("Failed to add SOC Thermal trip sel.");
		}
	}
}

void ISR_SYS_THROTTLE()
{
	/* Same as MB_THROTTLE, glitch of FAST_PROCHOT_N will affect FM_CPU_BIC_PROCHOT_LVT3_N.
	 * Ignore the fake event by checking whether SYS_throttle is asserted before recording the deassertion.
	 */
	static bool is_sys_throttle_assert = false;
	common_addsel_msg_t sel_msg;
	if (1 && (gpio_get(BMC_GPIOL1_SYS_PWRGD) == GPIO_HIGH)) {
		if ((gpio_get(FM_CPU_BIC_PROCHOT_LVT3_N) == GPIO_HIGH) &&
		    (is_sys_throttle_assert == true)) {
			sel_msg.event_type = IPMI_OEM_EVENT_TYPE_DEASSERT;
			is_sys_throttle_assert = false;
		} else if ((gpio_get(FM_CPU_BIC_PROCHOT_LVT3_N) == GPIO_LOW) &&
			   (is_sys_throttle_assert == false)) {
			sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
			is_sys_throttle_assert = true;
		} else {
			return;
		}
		sel_msg.InF_target = BMC_IPMB;
		sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_SYS_STA;
		sel_msg.sensor_number = SENSOR_NUM_SYSTEM_STATUS;
		sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_THROTTLE;
		sel_msg.event_data2 = 0xFF;
		sel_msg.event_data3 = 0xFF;
		if (!common_add_sel_evt_record(&sel_msg)) {
			LOG_ERR("Failed to add System Throttle sel.");
		}
	}
}

void ISR_HSC_OC()
{
	common_addsel_msg_t sel_msg;
	if (1) {
		if (gpio_get(FM_HSC_TIMER) == GPIO_LOW) {
			sel_msg.event_type = IPMI_OEM_EVENT_TYPE_DEASSERT;
		} else {
			sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
		}
		sel_msg.InF_target = BMC_IPMB;
		sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_SYS_STA;
		sel_msg.sensor_number = SENSOR_NUM_SYSTEM_STATUS;
		sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_HSCTIMER;
		sel_msg.event_data2 = 0xFF;
		sel_msg.event_data3 = 0xFF;
		if (!common_add_sel_evt_record(&sel_msg)) {
			LOG_ERR("Failed to add HSC OC sel.");
		}
	}
}

void ISR_UV_DETECT()
{
	common_addsel_msg_t sel_msg;
	if (1) {
		if (gpio_get(IRQ_UV_DETECT_N) == GPIO_HIGH) {
			sel_msg.event_type = IPMI_OEM_EVENT_TYPE_DEASSERT;
		} else {
			sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
		}
		sel_msg.InF_target = BMC_IPMB;
		sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_SYS_STA;
		sel_msg.sensor_number = SENSOR_NUM_SYSTEM_STATUS;
		sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_UV;
		sel_msg.event_data2 = 0xFF;
		sel_msg.event_data3 = 0xFF;
		if (!common_add_sel_evt_record(&sel_msg)) {
			LOG_ERR("Failed to add under voltage sel.");
		}
	}
}

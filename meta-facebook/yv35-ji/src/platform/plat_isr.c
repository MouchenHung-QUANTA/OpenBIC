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
#include "ipmi.h"
#include "libutil.h"
#include "logging/log.h"

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

void ISR_GPIOA0()
{
	isr_dbg_print(FPGA_READY);
}

void ISR_GPIOA3()
{
	isr_dbg_print(INA230_E1S_ALERT_L);
}

void ISR_GPIOA4()
{
	isr_dbg_print(IRQ_UV_DETECT_N);
}

void ISR_GPIOA5()
{
	isr_dbg_print(FPGA_WATCH_DOG_TIMER0_L);
}

void ISR_GPIOA6()
{
	isr_dbg_print(FPGA_WATCH_DOG_TIMER1_L);
}

void ISR_GPIOB0()
{
	isr_dbg_print(FPGA_WATCH_DOG_TIMER2_L);
}

void ISR_GPIOC0()
{
	isr_dbg_print(FM_HSC_TIMER);
}

void ISR_GPIOC1()
{
	isr_dbg_print(IRQ_I2C_IO_LVC_STBY_ALRT_L);
}

void ISR_GPIOC3()
{
	isr_dbg_print(BIC_I2C_0_FPGA_ALERT_L);
}

void ISR_GPIOC4()
{
	isr_dbg_print(BIC_I2C_1_FPGA_ALERT_L);
}

void ISR_GPIOD0()
{
	isr_dbg_print(PWRBTN_L);
}

void ISR_GPIOE0()
{
	isr_dbg_print(RUN_POWER_PG);
}

void ISR_GPIOE2()
{
	isr_dbg_print(SPI_BMC_FPGA_INT_L);
}

void ISR_GPIOE3()
{
	isr_dbg_print(IRQ_HSC_ALERT1_L);
}

void ISR_GPIOE4()
{
	isr_dbg_print(I2C_SENSOR_LVC_ALERT_L);
}

void ISR_GPIOE6()
{
	isr_dbg_print(RUN_POWER_EN);
}

void ISR_GPIOE7()
{
	isr_dbg_print(SPI_HOST_TPM_RST_L);
}

void ISR_GPIOF0()
{
	isr_dbg_print(HSC_TYPE_0);
}

void ISR_GPIOF1()
{
	isr_dbg_print(CARD_TYPE_EXP);
}

void ISR_GPIOF2()
{
	isr_dbg_print(RUN_POWER_FAULT_L);
}

void ISR_GPIOF6()
{
	isr_dbg_print(THERM_BB_OVERT_L);
}

void ISR_GPIOF7()
{
	isr_dbg_print(THERM_BB_WARN_L);
}

void ISR_GPIOG0()
{
	isr_dbg_print(BIC_CPU_JTAG_MUX_SEL);
}

void ISR_GPIOG2()
{
	isr_dbg_print(FPGA_READY);
}

void ISR_GPIOH3()
{
	isr_dbg_print(FAST_PROCHOT_L);
}

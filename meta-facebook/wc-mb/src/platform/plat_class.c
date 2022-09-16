#include "plat_class.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal_gpio.h"
#include "hal_i2c.h"
#include "libutil.h"
#include "plat_gpio.h"
#include "plat_i2c.h"

static uint8_t system_class = SYS_DUAL;
static uint8_t source_class = SRC_MAIN;

uint8_t get_system_class()
{
	return system_class;
}

uint8_t get_source_idx()
{
	return source_class;
}

void set_source_idx(source_class_t idx)
{
	source_class = idx;
}

void init_platform_config()
{
	if (gpio_get(SYS_SKU_ID0) == GPIO_HIGH)
		system_class = SYS_SINGLE;
	else
		system_class = SYS_DUAL;

	printf("SYS_SKU: %s Compute System\n", system_class == SYS_SINGLE ? "Single" : "Dual");
}

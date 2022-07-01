#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snoop.h"
#include "plat_gpio.h"
#include "hal_gpio.h"

void pal_postcode_led_ctl(uint8_t postcode)
{
	printf("******** new postcode 0x%x ********\n", postcode);
	for (int i = 0; i < 8; i++) {
		gpio_set(LED_POSTCODE_0 + i, (postcode & BIT(i)) >> i);
	}
}

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snoop.h"
#include "plat_snoop.h"
#include "plat_gpio.h"
#include "hal_gpio.h"

#define POSTCODE_LED_THREAD_STACK_SIZE 512

k_tid_t postcode_led_ctl_tid;
K_THREAD_STACK_DEFINE(postcode_led_ctl_thread, POSTCODE_LED_THREAD_STACK_SIZE);
struct k_thread postcode_led_thread_handler;

static void postcode_led_ctl()
{
	int cur_postcode_num = 0;
	int pre_postcode_num = 0;
	uint8_t cur_postcode = 0;
	uint8_t offset = 0;

	while (1) {
		cur_postcode_num = snoop_read_num;
		if (cur_postcode_num != pre_postcode_num) {
			if (cur_postcode_num > SNOOP_MAX_LEN)
				offset = (cur_postcode_num % SNOOP_MAX_LEN) - 1;
			else
				offset = cur_postcode_num - 1;
			copy_snoop_read_buffer(offset, 1, &cur_postcode, COPY_SPECIFIC_POSTCODE);

			for (int i = 0; i < 8; i++)
				gpio_set(LED_POSTCODE_0 + i, cur_postcode & BIT(i) >> i);
		}

		pre_postcode_num = cur_postcode_num;
	}
}

void abort_postcode_led_thread()
{
	if (postcode_led_ctl_tid != NULL &&
	    strcmp(k_thread_state_str(postcode_led_ctl_tid), "dead") != 0) {
		k_thread_abort(postcode_led_ctl_tid);
	}

	for (int i = 0; i < 8; i++)
		gpio_set(LED_POSTCODE_0 + i, GPIO_LOW);
}

void init_postcode_led_ctl()
{
	if (postcode_led_ctl_tid != NULL &&
	    strcmp(k_thread_state_str(postcode_led_ctl_tid), "dead") != 0) {
		return;
	}
	postcode_led_ctl_tid =
		k_thread_create(&postcode_led_thread_handler, postcode_led_ctl_thread,
				K_THREAD_STACK_SIZEOF(postcode_led_ctl_thread), postcode_led_ctl,
				NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&postcode_led_thread_handler, "postcode_led_ctl_thread");
}

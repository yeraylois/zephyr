/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <lvgl.h>

#define SLEEP_S 4

int main(void)
{
	const struct device *display_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		printk("Display not ready\n");
		return 0;
	}

	ui_init();

	/* 1. Dashboard */
	ui_show_dashboard(3850, 23, 0, true);
	lv_timer_handler();
	display_blanking_off(display_dev);
	k_sleep(K_SECONDS(SLEEP_S));

	/* 2. Auto test - Button step */
	ui_show_auto_test(0, 9, "Button", TEST_RUNNING,
			  "Press User Button");
	lv_timer_handler();
	k_sleep(K_SECONDS(SLEEP_S));

	/* 3. Auto test - NeoPixel step */
	ui_show_auto_test(2, 9, "NeoPixel", TEST_PASS,
			  "Check RGB glow");
	lv_timer_handler();
	k_sleep(K_SECONDS(SLEEP_S));

	/* 4. Manual menu */
	ui_show_manual_menu(2, 9);
	lv_timer_handler();
	k_sleep(K_SECONDS(SLEEP_S));

	/* 5. Test preview */
	ui_show_test_preview("TFT", "Check display colors");
	lv_timer_handler();
	k_sleep(K_SECONDS(SLEEP_S));

	/* 6. Result - all pass */
	{
		enum test_result results[9] = {
			TEST_PASS, TEST_PASS, TEST_PASS,
			TEST_PASS, TEST_PASS, TEST_PASS,
			TEST_PASS, TEST_PASS, TEST_SKIP
		};
		ui_show_result(true, results, 9);
		lv_timer_handler();
		k_sleep(K_SECONDS(SLEEP_S));
	}

	/* 7. TFT color test */
	ui_show_tft_test();
	lv_timer_handler();
	k_sleep(K_SECONDS(SLEEP_S));

	/* Keep window open */
	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(20));
	}

	return 0;
}

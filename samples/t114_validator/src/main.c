/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"
#include "test.h"
#include "ui.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <lvgl.h>

#define AUTO_STEP_COUNT 9

enum app_state {
	STATE_DASHBOARD,
	STATE_AUTO_TEST,
	STATE_MANUAL_MENU,
	STATE_TEST_PREVIEW,
	STATE_MANUAL_RUN,
	STATE_RESULT,
};

struct auto_step {
	const char *name;
	void (*run)(void *res);
	size_t res_size;
};

static enum app_state state = STATE_DASHBOARD;
static int dashboard_sel = 0;
static int auto_step = 0;
static int manual_sel = 0;
static bool auto_waiting = false;
static bool auto_test_waiting_button = false;
static int auto_advance_counter = 0;
static int bat_mv = 0;
static int temp_c = 0;

static struct { enum test_result status; } r_button;
static struct test_led_result r_led;
static struct test_neopixel_result r_np;
static struct test_adc_result r_adc;
static struct test_temp_result r_temp;
static struct test_ble_result r_ble;
static struct test_lora_result r_lora;
static struct test_qspi_result r_qspi;

static const char *instructions[AUTO_STEP_COUNT] = {
	"Press User Button",
	"Check LED is visible",
	"Check RGB glow",
	"Check display colors",
	"Check voltage reading",
	"Check temp reading",
	"Enable BT on host",
	"Check antenna connected",
	"Check flash access",
};

static const struct auto_step steps[AUTO_STEP_COUNT] = {
	{ "Button", NULL, 0 },
	{ "LED", (void (*)(void *))test_led_run, sizeof(r_led) },
	{ "NeoPixel", (void (*)(void *))test_neopixel_run, sizeof(r_np) },
	{ "TFT", (void (*)(void *))test_tft_run, 0 },
	{ "ADC Battery", (void (*)(void *))test_adc_run, sizeof(r_adc) },
	{ "Temperature", (void (*)(void *))test_temp_run, sizeof(r_temp) },
	{ "BLE Radio", (void (*)(void *))test_ble_run, sizeof(r_ble) },
	{ "LoRa TX", (void (*)(void *))test_lora_run, sizeof(r_lora) },
	{ "QSPI Flash", (void (*)(void *))test_qspi_run, sizeof(r_qspi) },
};

static bool usb_is_connected(void)
{
	const struct device *udc_dev = DEVICE_DT_GET(DT_NODELABEL(usbd));

	if (!device_is_ready(udc_dev)) {
		return false;
	}
	return udc_is_enabled(udc_dev);
}

static void update_dashboard_values(void)
{
	struct test_adc_result adc_res = {0};

	test_adc_run(&adc_res);
	if (adc_res.status == TEST_PASS) {
		bat_mv = adc_res.voltage_mv;
	}

	temp_c = read_temp_simple();

	ui_update_bat_temp(bat_mv, temp_c, usb_is_connected());
}

static int auto_test_timer_cs = 0;

static void run_auto_step(void)
{
	if (auto_step >= AUTO_STEP_COUNT) {
		bool all_pass = true;
		if (r_button.status != TEST_PASS) all_pass = false;
		if (r_led.status != TEST_PASS) all_pass = false;
		if (r_np.status != TEST_PASS) all_pass = false;
		if (r_adc.status != TEST_PASS) all_pass = false;
		if (r_temp.status != TEST_PASS) all_pass = false;
		if (r_ble.status != TEST_PASS) all_pass = false;
		if (r_lora.status != TEST_PASS) all_pass = false;
		if (r_qspi.status != TEST_PASS && r_qspi.status != TEST_SKIP) all_pass = false;

		enum test_result results[AUTO_STEP_COUNT] = {
			r_button.status, r_led.status, r_np.status,
			TEST_PASS, r_adc.status, r_temp.status,
			r_ble.status, r_lora.status, r_qspi.status
		};
		ui_show_result(all_pass, results, AUTO_STEP_COUNT);
		state = STATE_RESULT;
		return;
	}

	const struct auto_step *s = &steps[auto_step];

	/* Button test: wait for actual button press */
	if (auto_step == 0) {
		ui_show_auto_test(0, AUTO_STEP_COUNT, "Button", TEST_RUNNING,
				  instructions[0]);
		auto_test_waiting_button = true;
		auto_waiting = true;
		auto_advance_counter = 0;
		return;
	}

	/* Normal test: run and auto-advance after delay */
	ui_show_auto_test(auto_step, AUTO_STEP_COUNT, s->name, TEST_RUNNING,
			  instructions[auto_step]);
	lv_timer_handler();
	display_blanking_off(DEVICE_DT_GET(DT_CHOSEN(zephyr_display)));

	void *res = NULL;
	if (s->res_size > 0) {
		switch (auto_step) {
		case 1: res = &r_led; break;
		case 2: res = &r_np; break;
		case 4: res = &r_adc; break;
		case 5: res = &r_temp; break;
		case 6: res = &r_ble; break;
		case 7: res = &r_lora; break;
		case 8: res = &r_qspi; break;
		}
		s->run(res);
	} else {
		test_tft_run();
	}

	ui_show_auto_test(auto_step, AUTO_STEP_COUNT, s->name,
			  (s->res_size > 0) ? ((struct test_led_result *)res)->status : TEST_PASS,
			  instructions[auto_step]);
	auto_waiting = true;
	auto_advance_counter = 0;
}

static enum test_result manual_run_selected(void)
{
	enum test_result result = TEST_SKIP;

	switch (manual_sel) {
	case 0:
		test_led_run(&r_led);
		result = r_led.status;
		break;
	case 1:
		test_neopixel_run(&r_np);
		result = r_np.status;
		break;
	case 2:
		test_tft_run();
		result = TEST_PASS;
		break;
	case 3:
		test_adc_run(&r_adc);
		if (r_adc.status == TEST_PASS) bat_mv = r_adc.voltage_mv;
		result = r_adc.status;
		break;
	case 4:
		test_temp_run(&r_temp);
		if (r_temp.status == TEST_PASS) temp_c = r_temp.temp_c;
		result = r_temp.status;
		break;
	case 5:
		test_ble_run(&r_ble);
		result = r_ble.status;
		break;
	case 6:
		test_lora_run(&r_lora);
		result = r_lora.status;
		break;
	case 7:
		test_qspi_run(&r_qspi);
		result = r_qspi.status;
		break;
	}

	ui_show_manual_result(result);
	lv_timer_handler();
	display_blanking_off(DEVICE_DT_GET(DT_CHOSEN(zephyr_display)));
	return result;
}

int main(void)
{
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	button_init();
	ui_init();

	ui_show_dashboard(bat_mv, temp_c, dashboard_sel, usb_is_connected());

	lv_timer_handler();
	display_blanking_off(display_dev);

	update_dashboard_values();

	while (1) {
		button_poll();
		enum button_event evt = button_get_event();

		switch (state) {
		case STATE_DASHBOARD:
			if (evt == BTN_EVT_SHORT) {
				dashboard_sel = !dashboard_sel;
				ui_show_dashboard(bat_mv, temp_c, dashboard_sel, usb_is_connected());
			} else if (evt == BTN_EVT_LONG) {
				if (dashboard_sel == 0) {
					state = STATE_AUTO_TEST;
					auto_step = 0;
					auto_waiting = false;
					auto_test_waiting_button = false;
					auto_advance_counter = 0;
					auto_test_timer_cs = 0;
					r_button.status = TEST_PENDING;
					run_auto_step();
				} else {
					state = STATE_MANUAL_MENU;
					manual_sel = 0;
					ui_show_manual_menu(manual_sel, 9);
				}
			}
			break;

		case STATE_AUTO_TEST:
			if (auto_waiting) {
				/* Update chronometer every 100ms */
				static int tick;
				tick++;
				if (tick % 5 == 0) {
					auto_test_timer_cs++;
					ui_update_timer(auto_test_timer_cs);
				}

				if (auto_test_waiting_button) {
					/* Button test: 500ms grace then accept any press */
					auto_advance_counter++;
					if (auto_advance_counter >= 25) {
						if (evt != BTN_EVT_NONE) {
							r_button.status = TEST_PASS;
							auto_test_waiting_button = false;
							auto_waiting = false;
							auto_advance_counter = 0;
							auto_step++;
							run_auto_step();
						}
					}
				} else {
					/* Auto-advance after 800ms */
					auto_advance_counter++;
					if (auto_advance_counter >= 40) {
						auto_advance_counter = 0;
						auto_waiting = false;
						auto_step++;
							run_auto_step();
					}
				}
			}
			break;

		case STATE_MANUAL_MENU:
			if (evt == BTN_EVT_SHORT) {
				manual_sel = (manual_sel + 1) % 9;
				ui_show_manual_menu(manual_sel, 9);
			} else if (evt == BTN_EVT_LONG) {
				if (manual_sel == 8) {
					/* Back to main */
					state = STATE_DASHBOARD;
					ui_show_dashboard(bat_mv, temp_c, dashboard_sel, usb_is_connected());
			} else {
				/* Show preview */
				state = STATE_TEST_PREVIEW;
				const char *instr = instructions[manual_sel + 1];
				const char *names[] = {
					"LED", "NeoPixel", "TFT", "ADC",
					"Temp", "BLE", "LoRa", "QSPI"
				};
				ui_show_test_preview(names[manual_sel], instr);
			}
			}
			break;

		case STATE_TEST_PREVIEW:
			if (evt == BTN_EVT_SHORT) {
				/* Cancel */
				state = STATE_MANUAL_MENU;
				ui_show_manual_menu(manual_sel, 9);
			} else if (evt == BTN_EVT_LONG) {
				/* Run test */
				state = STATE_MANUAL_RUN;
				manual_run_selected();
			}
			break;

		case STATE_MANUAL_RUN:
			/* Show result for 2s then back to menu */
			{
				static int run_tick = 0;
				run_tick++;
				if (run_tick >= 100) { /* 100 * 20ms = 2000ms */
					run_tick = 0;
					state = STATE_MANUAL_MENU;
					ui_show_manual_menu(manual_sel, 9);
				}
			}
			break;

		case STATE_RESULT:
			if (evt != BTN_EVT_NONE) {
				state = STATE_DASHBOARD;
				ui_show_dashboard(bat_mv, temp_c, dashboard_sel, usb_is_connected());
			}
			break;
		}

		if (state == STATE_DASHBOARD) {
			static int tick;
			tick++;
			if (tick % 20 == 0) {
				update_dashboard_values();
			}
		}

		lv_timer_handler();
		k_sleep(K_MSEC(20));
	}

	return 0;
}

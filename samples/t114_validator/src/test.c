/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"
#include "ui.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(test);

#define LED0_NODE DT_ALIAS(led0)
#define STRIP_NODE DT_ALIAS(led_strip)
#define ADC_NODE DT_NODELABEL(adc)
#define TEMP_NODE DT_NODELABEL(temp)
#define LORA_NODE DT_ALIAS(lora0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *strip_dev;
static const struct adc_channel_cfg adc_cfg = {
	.channel_id = 2,
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.input_positive = NRF_SAADC_AIN2,
};

void test_led_run(struct test_led_result *res)
{
	res->status = TEST_RUNNING;

	if (!device_is_ready(led.port)) {
		res->status = TEST_FAIL;
		return;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	for (int i = 0; i < 3; i++) {
		gpio_pin_set_dt(&led, 1);
		k_sleep(K_MSEC(200));
		gpio_pin_set_dt(&led, 0);
		k_sleep(K_MSEC(200));
	}

	res->status = TEST_PASS;
}

void test_button_run(struct test_button_result *res)
{
	res->status = TEST_PENDING;
}

void test_neopixel_run(struct test_neopixel_result *res)
{
	res->status = TEST_RUNNING;

	strip_dev = DEVICE_DT_GET(STRIP_NODE);
	if (!device_is_ready(strip_dev)) {
		res->status = TEST_FAIL;
		return;
	}

	struct led_rgb colors[] = {
		{ .r = 50, .g = 0, .b = 0 },   /* red dim */
		{ .r = 0, .g = 50, .b = 0 },   /* green dim */
		{ .r = 0, .g = 0, .b = 50 },   /* blue dim */
	};
	struct led_rgb off = { 0 };

	for (int c = 0; c < 3; c++) {
		/* Light both pixels with the same color */
		struct led_rgb px[2] = { colors[c], colors[c] };
		int err = led_strip_update_rgb(strip_dev, px, 2);
		if (err) {
			LOG_ERR("led_strip_update_rgb failed: %d", err);
			res->status = TEST_FAIL;
			return;
		}
		k_sleep(K_MSEC(500));
	}

	struct led_rgb px_off[2] = { off, off };
	int err = led_strip_update_rgb(strip_dev, px_off, 2);
	if (err) {
		LOG_ERR("led_strip_update_rgb (off) failed: %d", err);
		res->status = TEST_FAIL;
		return;
	}

	res->status = TEST_PASS;
}

void test_tft_run(void)
{
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	ui_show_tft_test();
	lv_timer_handler();
	display_blanking_off(display_dev);

	/* Let the user see the test pattern for 2 seconds */
	k_sleep(K_MSEC(2000));
}

void test_adc_run(struct test_adc_result *res)
{
	res->status = TEST_RUNNING;

	const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
	if (!device_is_ready(adc_dev)) {
		res->status = TEST_FAIL;
		return;
	}

	int err = adc_channel_setup(adc_dev, &adc_cfg);
	if (err) {
		res->status = TEST_FAIL;
		return;
	}

	int16_t buf;
	struct adc_sequence seq = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
		.channels = BIT(2),
		.resolution = 12,
	};

	err = adc_read(adc_dev, &seq);
	if (err) {
		res->status = TEST_FAIL;
		return;
	}

	int32_t raw = buf;
	if (raw < 0) {
		raw = 0;
	}

	int32_t vref = adc_ref_internal(adc_dev);
	int32_t mv = (raw * vref) / 4096;
	mv = (mv * 490) / 100;

	res->voltage_mv = mv;
	res->status = TEST_PASS;
}

int read_temp_simple(void)
{
	const struct device *temp_dev = DEVICE_DT_GET(TEMP_NODE);

	if (!device_is_ready(temp_dev)) {
		return 0;
	}

	int err = sensor_sample_fetch(temp_dev);
	if (err) {
		return 0;
	}

	struct sensor_value val;
	err = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (err) {
		return 0;
	}

	return val.val1;
}

void test_temp_run(struct test_temp_result *res)
{
	res->status = TEST_RUNNING;

	const struct device *temp_dev = DEVICE_DT_GET(TEMP_NODE);
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(temp_dev)) {
		res->status = TEST_FAIL;
		return;
	}

	/* Show temperature screen and take live readings for 3 seconds */
	ui_show_temp_test(0);
	lv_timer_handler();
	display_blanking_off(display_dev);

	int total_temp = 0;
	int samples = 0;

	for (int i = 0; i < 6; i++) {
		int err = sensor_sample_fetch(temp_dev);
		if (err) {
			continue;
		}

		struct sensor_value val;
		err = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
		if (err) {
			continue;
		}

		total_temp += val.val1;
		samples++;

		ui_update_temp_test(val.val1);
		lv_timer_handler();
		display_blanking_off(display_dev);

		k_sleep(K_MSEC(500));
	}

	if (samples == 0) {
		res->status = TEST_FAIL;
		return;
	}

	res->temp_c = total_temp / samples;
	res->status = TEST_PASS;
}

void test_ble_run(struct test_ble_result *res)
{
	res->status = TEST_RUNNING;

#ifdef CONFIG_BT
	int err = bt_enable(NULL);
	if (err && err != -EALREADY) {
		res->status = TEST_FAIL;
		return;
	}

	res->status = TEST_PASS;
#else
	res->status = TEST_SKIP;
#endif
}

void test_lora_run(struct test_lora_result *res)
{
	res->status = TEST_RUNNING;

	const struct device *lora_dev = DEVICE_DT_GET(LORA_NODE);
	if (!device_is_ready(lora_dev)) {
		res->status = TEST_FAIL;
		return;
	}

	struct lora_modem_config cfg = {
		.frequency = 868000000,
		.bandwidth = BW_125_KHZ,
		.datarate = SF_7,
		.coding_rate = CR_4_5,
		.preamble_len = 8,
		.tx_power = 14,
		.tx = true,
	};

	int err = lora_config(lora_dev, &cfg);
	if (err) {
		res->status = TEST_FAIL;
		return;
	}

	uint8_t data[] = "T114_TEST";
	err = lora_send(lora_dev, data, sizeof(data));
	if (err) {
		res->status = TEST_FAIL;
		return;
	}

	res->status = TEST_PASS;
}

void test_qspi_run(struct test_qspi_result *res)
{
	res->status = TEST_RUNNING;

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mx25r1635f), okay)
	const struct device *flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r1635f));
	if (!device_is_ready(flash_dev)) {
		res->status = TEST_SKIP;
		return;
	}
	res->status = TEST_PASS;
#else
	res->status = TEST_SKIP;
#endif
}

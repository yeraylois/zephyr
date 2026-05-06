/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_H
#define TEST_H

#include <stdbool.h>
#include <stdint.h>

enum test_result {
    TEST_PENDING,
    TEST_RUNNING,
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
};

struct test_led_result {
    enum test_result status;
};

struct test_button_result {
    enum test_result status;
};

struct test_neopixel_result {
    enum test_result status;
};

struct test_tft_result {
    enum test_result status;
};

struct test_adc_result {
    enum test_result status;
    int voltage_mv;
};

struct test_temp_result {
    enum test_result status;
    int temp_c;
};

struct test_ble_result {
    enum test_result status;
};

struct test_lora_result {
    enum test_result status;
};

struct test_qspi_result {
    enum test_result status;
    uint32_t jedec_id;
};

void test_led_run(struct test_led_result *res);
void test_button_run(struct test_button_result *res);
void test_neopixel_run(struct test_neopixel_result *res);
void test_tft_run(void);
void test_adc_run(struct test_adc_result *res);
int read_temp_simple(void);
void test_temp_run(struct test_temp_result *res);
void test_ble_run(struct test_ble_result *res);
void test_lora_run(struct test_lora_result *res);
void test_qspi_run(struct test_qspi_result *res);

#endif

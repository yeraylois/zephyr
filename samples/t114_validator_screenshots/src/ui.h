/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UI_H
#define UI_H

#include "test.h"
#include <lvgl.h>
#include <stdbool.h>

void ui_init(void);
void ui_show_dashboard(int bat_mv, int temp_c, int selected, bool usb_connected);
void ui_update_bat_temp(int bat_mv, int temp_c, bool usb_connected);

void ui_show_auto_test(int step, int total, const char *name,
                       enum test_result result, const char *instruction);
void ui_update_auto_test_progress(int step, int total);
void ui_update_timer(int total_cs);
void ui_set_instruction(const char *txt);

void ui_show_manual_menu(int selected, int total);
void ui_show_test_preview(const char *name, const char *instruction);
void ui_show_manual_result(enum test_result result);
void ui_show_result(bool all_pass, enum test_result *results, int count);

void ui_show_tft_test(void);

void ui_show_temp_test(int temp_c);
void ui_update_temp_test(int temp_c);

#endif

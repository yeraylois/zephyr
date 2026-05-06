/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui.h"
#include <stdio.h>

#define C_BG          lv_color_black()
#define C_CARD        lv_color_hex(0x1a1a1a)
#define C_CARD_HL     lv_color_hex(0x2a2a2a)
#define C_ACCENT      lv_color_hex(0x39FF14)
#define C_BLUE        lv_color_hex(0x00BFFF)
#define C_ORANGE      lv_color_hex(0xFF8C00)
#define C_RED         lv_color_hex(0xFF4444)
#define C_GREEN       lv_color_hex(0x00FF41)
#define C_TEXT_DIM    lv_color_hex(0x888888)
#define C_TEXT_INFO   lv_color_hex(0xAAAAAA)
#define C_WHITE       lv_color_white()

static lv_obj_t *scr;

/* Dashboard widgets */
static lv_obj_t *lbl_temp;
static lv_obj_t *lbl_title;
static lv_obj_t *bat_cont;
static lv_obj_t *bat_fill;
static lv_obj_t *bat_text;
static lv_obj_t *opt1;
static lv_obj_t *opt2;

static const char *menu_items[] = {
	"LED",
	"NEOPIXEL",
	"TFT",
	"ADC",
	"TEMP",
	"BLE",
	"LORA",
	"QSPI",
	"BACK",
};
#define MENU_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

static int bat_pct_from_mv(int mv)
{
	if (mv >= 4200) {
		return 100;
	}
	if (mv <= 3000) {
		return 0;
	}
	return (mv - 3000) * 100 / 1200;
}

static lv_color_t bat_color(int pct)
{
	if (pct > 60) {
		return C_ACCENT;
	}
	if (pct > 20) {
		return C_ORANGE;
	}
	return C_RED;
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font,
			    lv_color_t col, lv_align_t align, int x, int y,
			    const char *txt)
{
	lv_obj_t *lbl = lv_label_create(parent);
	lv_label_set_text(lbl, txt);
	lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
	lv_obj_set_style_text_color(lbl, col, LV_PART_MAIN);
	lv_obj_align(lbl, align, x, y);
	return lbl;
}

void ui_init(void)
{
	scr = lv_screen_active();
	lv_obj_set_style_bg_color(scr, C_BG, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
}

static void add_fullscreen_bg(void)
{
	lv_obj_t *bg = lv_obj_create(scr);
	lv_obj_remove_style_all(bg);
	lv_obj_set_size(bg, 240, 135);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_style_bg_color(bg, C_BG, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, LV_PART_MAIN);
}

/* Helper: create a retro card */
static lv_obj_t *make_card(lv_obj_t *parent, int x, int y, int w, int h,
			   bool selected, const char *txt)
{
	lv_obj_t *c = lv_obj_create(parent);
	lv_obj_remove_style_all(c);
	lv_obj_set_size(c, w, h);
	lv_obj_set_pos(c, x, y);
	lv_obj_set_style_bg_color(c,
				  selected ? C_CARD_HL : lv_color_hex(0x0a0a0a),
				  LV_PART_MAIN);
	lv_obj_set_style_bg_opa(c, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(c, selected ? 2 : 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(c,
				      selected ? C_ORANGE : lv_color_hex(0x333333),
				      LV_PART_MAIN);
	lv_obj_set_style_radius(c, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);

	lv_obj_t *lbl = lv_label_create(c);
	lv_label_set_text(lbl, txt);
	lv_obj_set_style_text_font(lbl, &lv_font_unscii_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(lbl,
				    selected ? C_WHITE : C_TEXT_DIM,
				    LV_PART_MAIN);
	lv_obj_set_width(lbl, w);
	lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_center(lbl);
	return c;
}

void ui_show_dashboard(int bat_mv, int temp_c, int selected, bool usb_connected)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();
	opt1 = NULL;
	opt2 = NULL;
	bat_cont = NULL;
	bat_fill = NULL;
	bat_text = NULL;
	lbl_temp = NULL;

	/* ---- Header ---- */
	/* Temperature dot */
	lv_obj_t *tmp_dot = lv_obj_create(scr);
	lv_obj_set_size(tmp_dot, 6, 6);
	lv_obj_set_style_bg_color(tmp_dot, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_border_width(tmp_dot, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(tmp_dot, 2, LV_PART_MAIN);
	lv_obj_align(tmp_dot, LV_ALIGN_TOP_LEFT, 8, 8);

	lbl_temp = make_label(scr, &lv_font_unscii_8, C_TEXT_INFO,
			      LV_ALIGN_TOP_LEFT, 18, 8, "-- C");

	/* Title */
	lbl_title = make_label(scr, &lv_font_unscii_16, C_WHITE,
			       LV_ALIGN_TOP_MID, 0, 6, "T114");

	/* Underline */
	lv_obj_t *underline = lv_obj_create(scr);
	lv_obj_remove_style_all(underline);
	lv_obj_set_size(underline, 40, 3);
	lv_obj_set_pos(underline, 100, 24);
	lv_obj_set_style_bg_color(underline, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(underline, LV_OPA_COVER, LV_PART_MAIN);

	/* Battery */
	bat_cont = lv_obj_create(scr);
	lv_obj_set_size(bat_cont, 40, 14);
	lv_obj_align(bat_cont, LV_ALIGN_TOP_RIGHT, -8, 6);
	lv_obj_set_style_bg_color(bat_cont, C_CARD, LV_PART_MAIN);
	lv_obj_set_style_radius(bat_cont, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(bat_cont, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(bat_cont, C_TEXT_DIM, LV_PART_MAIN);
	lv_obj_set_style_pad_all(bat_cont, 0, LV_PART_MAIN);

	bat_fill = lv_obj_create(bat_cont);
	lv_obj_remove_style_all(bat_fill);
	lv_obj_set_size(bat_fill, 1, 12);
	lv_obj_set_pos(bat_fill, 1, 1);
	lv_obj_set_style_bg_color(bat_fill, C_ACCENT, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bat_fill, LV_OPA_COVER, LV_PART_MAIN);

	bat_text = lv_label_create(bat_cont);
	lv_label_set_text(bat_text, "");
	lv_obj_set_style_text_font(bat_text, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(bat_text, C_WHITE, LV_PART_MAIN);
	lv_obj_center(bat_text);

	/* ---- Cards ---- */
	make_card(scr, 8, 34, 110, 70, selected == 0, "AUTO\nTEST");
	make_card(scr, 122, 34, 110, 70, selected == 1, "MANUAL\nMENU");

	/* ---- Footer stats ---- */
	make_label(scr, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_BOTTOM_LEFT, 8, -6, "TEMP");
	make_label(scr, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_BOTTOM_RIGHT, -8, -6, "BAT");

	ui_update_bat_temp(bat_mv, temp_c, usb_connected);
}

void ui_update_bat_temp(int bat_mv, int temp_c, bool usb_connected)
{
	char buf[16];

	if (lbl_temp) {
		snprintf(buf, sizeof(buf), "%d C", temp_c);
		lv_label_set_text(lbl_temp, buf);
	}

	if (bat_cont && bat_fill && bat_text) {
		if (usb_connected) {
			lv_obj_set_size(bat_fill, 38, 12);
			lv_obj_set_style_bg_color(bat_fill, C_BLUE, LV_PART_MAIN);
			lv_label_set_text(bat_text, "USB");
			lv_obj_set_style_text_color(bat_text, lv_color_black(), LV_PART_MAIN);
		} else {
			int pct = bat_pct_from_mv(bat_mv);
			int w = (pct * 38) / 100;
			if (w < 1) {
				w = 1;
			}
			lv_obj_set_size(bat_fill, w, 12);
			lv_obj_set_style_bg_color(bat_fill, bat_color(pct), LV_PART_MAIN);
			snprintf(buf, sizeof(buf), "%d%%", pct);
			lv_label_set_text(bat_text, buf);
			lv_obj_set_style_text_color(bat_text, C_WHITE, LV_PART_MAIN);
		}
	}
}

/* Auto-test screen widgets */
static lv_obj_t *at_window;
static lv_obj_t *at_step_lbl;
static lv_obj_t *at_status_lbl;
static lv_obj_t *at_timer_lbl;
static lv_obj_t *at_instr_lbl;

void ui_show_auto_test(int step, int total, const char *name,
                       enum test_result result, const char *instruction)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();
	at_window = NULL;
	at_step_lbl = NULL;
	at_status_lbl = NULL;
	at_timer_lbl = NULL;
	at_instr_lbl = NULL;

	/* ---- Retro window container ---- */
	at_window = lv_obj_create(scr);
	lv_obj_remove_style_all(at_window);
	lv_obj_set_size(at_window, 224, 88);
	lv_obj_set_pos(at_window, 8, 30);
	lv_obj_set_style_bg_color(at_window, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(at_window, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(at_window, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(at_window, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_radius(at_window, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(at_window, 0, LV_PART_MAIN);

	/* Title bar: orange bg, black text */
	lv_obj_t *title_bar = lv_obj_create(at_window);
	lv_obj_remove_style_all(title_bar);
	lv_obj_set_size(title_bar, 222, 14);
	lv_obj_set_pos(title_bar, 1, 1);
	lv_obj_set_style_bg_color(title_bar, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN);

	lv_obj_t *title_lbl = lv_label_create(title_bar);
	lv_label_set_text(title_lbl, " AUTO TEST");
	lv_obj_set_style_text_font(title_lbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(title_lbl, lv_color_black(), LV_PART_MAIN);
	lv_obj_align(title_lbl, LV_ALIGN_LEFT_MID, 2, 0);

	/* Step counter */
	char sbuf[32];
	snprintf(sbuf, sizeof(sbuf), "%d/%d %s", step + 1, total, name);
	at_step_lbl = lv_label_create(at_window);
	lv_label_set_text(at_step_lbl, sbuf);
	lv_obj_set_style_text_font(at_step_lbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(at_step_lbl, C_WHITE, LV_PART_MAIN);
	lv_obj_set_pos(at_step_lbl, 6, 18);

	/* Status */
	const char *rsym = "WAIT";
	lv_color_t rcol = C_WHITE;

	switch (result) {
	case TEST_PENDING:
		rsym = "WAIT";
		rcol = C_WHITE;
		break;
	case TEST_RUNNING:
		rsym = "RUNNING";
		rcol = lv_color_hex(0xFFFF00);
		break;
	case TEST_PASS:
		rsym = "PASS";
		rcol = C_GREEN;
		break;
	case TEST_FAIL:
		rsym = "FAIL";
		rcol = C_RED;
		break;
	case TEST_SKIP:
		rsym = "SKIP";
		rcol = C_TEXT_DIM;
		break;
	}

	at_status_lbl = lv_label_create(at_window);
	lv_label_set_text(at_status_lbl, rsym);
	lv_obj_set_style_text_font(at_status_lbl, &lv_font_unscii_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(at_status_lbl, rcol, LV_PART_MAIN);
	lv_obj_align(at_status_lbl, LV_ALIGN_TOP_MID, 0, 30);

	/* Timer - smaller Unscii 8 */
	at_timer_lbl = lv_label_create(at_window);
	lv_label_set_text(at_timer_lbl, "00:00:00");
	lv_obj_set_style_text_font(at_timer_lbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(at_timer_lbl, C_ORANGE, LV_PART_MAIN);
	lv_obj_align(at_timer_lbl, LV_ALIGN_TOP_MID, 0, 58);

	/* Instruction */
	at_instr_lbl = lv_label_create(at_window);
	lv_label_set_text(at_instr_lbl, instruction ? instruction : "");
	lv_obj_set_style_text_font(at_instr_lbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(at_instr_lbl, C_TEXT_DIM, LV_PART_MAIN);
	lv_obj_align(at_instr_lbl, LV_ALIGN_BOTTOM_MID, 0, -2);
}

void ui_update_timer(int total_cs)
{
	if (!at_timer_lbl) {
		return;
	}

	int m = total_cs / 6000;
	int s = (total_cs / 100) % 60;
	int cs = total_cs % 100;
	char buf[16];
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d", m, s, cs);
	lv_label_set_text(at_timer_lbl, buf);
}

void ui_set_instruction(const char *txt)
{
	if (at_instr_lbl) {
		lv_label_set_text(at_instr_lbl, txt ? txt : "");
	}
}

void ui_show_manual_menu(int selected, int total)
{
	(void)total;

	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();

	/* ---- Retro window ---- */
	lv_obj_t *win = lv_obj_create(scr);
	lv_obj_remove_style_all(win);
	lv_obj_set_size(win, 224, 102);
	lv_obj_set_pos(win, 8, 26);
	lv_obj_set_style_bg_color(win, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(win, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(win, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(win, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_radius(win, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(win, 0, LV_PART_MAIN);

	/* Title bar */
	lv_obj_t *tbar = lv_obj_create(win);
	lv_obj_remove_style_all(tbar);
	lv_obj_set_size(tbar, 222, 12);
	lv_obj_set_pos(tbar, 1, 1);
	lv_obj_set_style_bg_color(tbar, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(tbar, 0, LV_PART_MAIN);

	lv_obj_t *tlbl = lv_label_create(tbar);
	lv_label_set_text(tlbl, "MANUAL");
	lv_obj_set_style_text_font(tlbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(tlbl, lv_color_black(), LV_PART_MAIN);
	lv_obj_center(tlbl);

	/* Grid 2x4 + full width back - recalculated */
	/* Window: 224x102. Inner: 222x100. Title: 12px at y=1.
	 * Grid area: y=14 to y=99 (85 px).
	 * 5 rows * 12 px = 60 px. 4 gaps * 5 px = 20 px. Total: 80 px.
	 * Start at y=15. End at y=95. Fits in 100 px.
	 *
	 * Horizontal: 222 px. 2 cols * 108 px = 216. 1 gap * 4 px = 4.
	 * Total: 220 px. Margins: (222-220)/2 = 1 px each side.
	 */
	for (int i = 0; i < (int)MENU_COUNT; i++) {
		lv_obj_t *cell = lv_obj_create(win);
		lv_obj_remove_style_all(cell);

		if (i < 8) {
			int col = i % 2;
			int row = i / 2;
			lv_obj_set_size(cell, 108, 12);
			lv_obj_set_pos(cell, 2 + col * 112, 15 + row * 17);
		} else {
			lv_obj_set_size(cell, 220, 12);
			lv_obj_set_pos(cell, 2, 15 + 4 * 17);
		}

		if (i == selected) {
			lv_obj_set_style_bg_color(cell, C_CARD_HL, LV_PART_MAIN);
			lv_obj_set_style_border_width(cell, 2, LV_PART_MAIN);
			lv_obj_set_style_border_color(cell, C_ORANGE, LV_PART_MAIN);
		} else {
			lv_obj_set_style_bg_color(cell, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
			lv_obj_set_style_border_width(cell, 1, LV_PART_MAIN);
			lv_obj_set_style_border_color(cell, lv_color_hex(0x333333), LV_PART_MAIN);
		}
		lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_radius(cell, 0, LV_PART_MAIN);

		lv_obj_t *lbl = lv_label_create(cell);
		lv_label_set_text(lbl, menu_items[i]);
		lv_obj_set_style_text_font(lbl, &lv_font_unscii_8, LV_PART_MAIN);
		if (i == selected) {
			lv_obj_set_style_text_color(lbl, C_WHITE, LV_PART_MAIN);
		} else {
			lv_obj_set_style_text_color(lbl, C_TEXT_DIM, LV_PART_MAIN);
		}
		lv_obj_center(lbl);
	}
}

void ui_show_test_preview(const char *name, const char *instruction)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();

	/* ---- Retro window ---- */
	lv_obj_t *win = lv_obj_create(scr);
	lv_obj_remove_style_all(win);
	lv_obj_set_size(win, 224, 110);
	lv_obj_set_pos(win, 8, 22);
	lv_obj_set_style_bg_color(win, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(win, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(win, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(win, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_radius(win, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(win, 0, LV_PART_MAIN);

	/* Title bar */
	lv_obj_t *tbar = lv_obj_create(win);
	lv_obj_remove_style_all(tbar);
	lv_obj_set_size(tbar, 222, 14);
	lv_obj_set_pos(tbar, 1, 1);
	lv_obj_set_style_bg_color(tbar, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(tbar, 0, LV_PART_MAIN);

	lv_obj_t *tlbl = lv_label_create(tbar);
	lv_label_set_text(tlbl, " TEST PREVIEW");
	lv_obj_set_style_text_font(tlbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(tlbl, lv_color_black(), LV_PART_MAIN);
	lv_obj_align(tlbl, LV_ALIGN_LEFT_MID, 2, 0);

	/* Test name */
	lv_obj_t *name_lbl = lv_label_create(win);
	lv_label_set_text(name_lbl, name);
	lv_obj_set_style_text_font(name_lbl, &lv_font_unscii_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(name_lbl, C_WHITE, LV_PART_MAIN);
	lv_obj_align(name_lbl, LV_ALIGN_TOP_MID, 0, 20);

	/* Instruction */
	lv_obj_t *instr_lbl = lv_label_create(win);
	lv_label_set_text(instr_lbl, instruction);
	lv_obj_set_style_text_font(instr_lbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(instr_lbl, C_TEXT_INFO, LV_PART_MAIN);
	lv_obj_align(instr_lbl, LV_ALIGN_TOP_MID, 0, 42);

	/* Button hints */
	lv_obj_t *hint = lv_label_create(win);
	lv_label_set_text(hint, "LONG: run test\nSHORT: cancel");
	lv_obj_set_style_text_font(hint, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(hint, C_TEXT_DIM, LV_PART_MAIN);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}

void ui_show_manual_result(enum test_result result)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();

	const char *txt = "WAIT";
	lv_color_t col = C_WHITE;

	switch (result) {
	case TEST_PASS:
		txt = "PASS";
		col = C_GREEN;
		break;
	case TEST_FAIL:
		txt = "FAIL";
		col = C_RED;
		break;
	default:
		txt = "DONE";
		col = C_TEXT_DIM;
		break;
	}

	lv_obj_t *lbl = lv_label_create(scr);
	lv_label_set_text(lbl, txt);
	lv_obj_set_style_text_font(lbl, &lv_font_unscii_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(lbl, col, LV_PART_MAIN);
	lv_obj_center(lbl);
}

static const char *test_abbr[] = {
	"BTN", "LED", "NPX", "TFT", "ADC", "TMP", "BLE", "LRA", "QSP"
};

void ui_show_result(bool all_pass, enum test_result *results, int count)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();

	/* ---- Retro window ---- */
	lv_obj_t *win = lv_obj_create(scr);
	lv_obj_remove_style_all(win);
	lv_obj_set_size(win, 224, 102);
	lv_obj_set_pos(win, 8, 26);
	lv_obj_set_style_bg_color(win, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(win, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(win, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(win, all_pass ? C_GREEN : C_RED, LV_PART_MAIN);
	lv_obj_set_style_radius(win, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(win, 0, LV_PART_MAIN);

	/* Title bar */
	lv_obj_t *tbar = lv_obj_create(win);
	lv_obj_remove_style_all(tbar);
	lv_obj_set_size(tbar, 222, 14);
	lv_obj_set_pos(tbar, 1, 1);
	lv_obj_set_style_bg_color(tbar, all_pass ? C_GREEN : C_RED, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(tbar, 0, LV_PART_MAIN);

	lv_obj_t *tlbl = lv_label_create(tbar);
	lv_label_set_text(tlbl, " RESULT");
	lv_obj_set_style_text_font(tlbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(tlbl, lv_color_black(), LV_PART_MAIN);
	lv_obj_align(tlbl, LV_ALIGN_LEFT_MID, 2, 0);

	/* Grid 3x3 - centered vertically */
	for (int i = 0; i < count && i < 9; i++) {
		int col = i % 3;
		int row = i / 3;
		lv_obj_t *cell = lv_obj_create(win);
		lv_obj_remove_style_all(cell);
		lv_obj_set_size(cell, 66, 22);
		lv_obj_set_pos(cell, 6 + col * 74, 18 + row * 24);

		if (results[i] == TEST_PASS) {
			lv_obj_set_style_bg_color(cell, lv_color_hex(0x0a2a0a), LV_PART_MAIN);
		} else if (results[i] == TEST_FAIL) {
			lv_obj_set_style_bg_color(cell, lv_color_hex(0x2a0a0a), LV_PART_MAIN);
		} else {
			lv_obj_set_style_bg_color(cell, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
		}
		lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_radius(cell, 0, LV_PART_MAIN);

		char buf[16];
		const char *sym = (results[i] == TEST_PASS) ? "OK" :
				  (results[i] == TEST_FAIL) ? "XX" : "--";
		snprintf(buf, sizeof(buf), "%s[%s]", test_abbr[i], sym);

		lv_obj_t *lbl = lv_label_create(cell);
		lv_label_set_text(lbl, buf);
		lv_obj_set_style_text_font(lbl, &lv_font_unscii_8, LV_PART_MAIN);
		if (results[i] == TEST_PASS) {
			lv_obj_set_style_text_color(lbl, C_GREEN, LV_PART_MAIN);
		} else if (results[i] == TEST_FAIL) {
			lv_obj_set_style_text_color(lbl, C_RED, LV_PART_MAIN);
		} else {
			lv_obj_set_style_text_color(lbl, C_TEXT_DIM, LV_PART_MAIN);
		}
		lv_obj_center(lbl);
	}

	/* Footer hint */
	make_label(win, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_BOTTOM_MID, 0, -2,
		   "Press button to exit");
}

void ui_show_tft_test(void)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();

	/* ---- Retro window ---- */
	lv_obj_t *win = lv_obj_create(scr);
	lv_obj_remove_style_all(win);
	lv_obj_set_size(win, 224, 118);
	lv_obj_set_pos(win, 8, 8);
	lv_obj_set_style_bg_color(win, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(win, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(win, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(win, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_radius(win, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(win, 0, LV_PART_MAIN);

	/* Title bar: orange bg, black text */
	lv_obj_t *tbar = lv_obj_create(win);
	lv_obj_remove_style_all(tbar);
	lv_obj_set_size(tbar, 222, 14);
	lv_obj_set_pos(tbar, 1, 1);
	lv_obj_set_style_bg_color(tbar, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(tbar, 0, LV_PART_MAIN);

	lv_obj_t *tlbl = lv_label_create(tbar);
	lv_label_set_text(tlbl, " TFT TEST");
	lv_obj_set_style_text_font(tlbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(tlbl, lv_color_black(), LV_PART_MAIN);
	lv_obj_align(tlbl, LV_ALIGN_LEFT_MID, 2, 0);

	/* ---- Color blocks: 3 vertical strips ---- */
	/* Window inner: 222x116. Title: 14px. Area: y=16 to y=115 (99 px).
	 * 3 strips * 70 px = 210. 2 gaps * 2 px = 4. Total: 214.
	 * Margins: (222-214)/2 = 4 px each side.
	 */
	lv_obj_t *red_block = lv_obj_create(win);
	lv_obj_remove_style_all(red_block);
	lv_obj_set_size(red_block, 70, 50);
	lv_obj_set_pos(red_block, 4, 17);
	lv_obj_set_style_bg_color(red_block, lv_color_hex(0xCC0000), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(red_block, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(red_block, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(red_block, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(red_block, lv_color_hex(0xFF3333), LV_PART_MAIN);

	lv_obj_t *green_block = lv_obj_create(win);
	lv_obj_remove_style_all(green_block);
	lv_obj_set_size(green_block, 70, 50);
	lv_obj_set_pos(green_block, 76, 17);
	lv_obj_set_style_bg_color(green_block, lv_color_hex(0x00AA00), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(green_block, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(green_block, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(green_block, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(green_block, lv_color_hex(0x33FF33), LV_PART_MAIN);

	lv_obj_t *blue_block = lv_obj_create(win);
	lv_obj_remove_style_all(blue_block);
	lv_obj_set_size(blue_block, 70, 50);
	lv_obj_set_pos(blue_block, 148, 17);
	lv_obj_set_style_bg_color(blue_block, lv_color_hex(0x0000CC), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(blue_block, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(blue_block, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(blue_block, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(blue_block, lv_color_hex(0x3333FF), LV_PART_MAIN);

	/* Labels inside blocks */
	lv_obj_t *rlbl = lv_label_create(red_block);
	lv_label_set_text(rlbl, "RED");
	lv_obj_set_style_text_font(rlbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(rlbl, C_WHITE, LV_PART_MAIN);
	lv_obj_center(rlbl);

	lv_obj_t *glbl = lv_label_create(green_block);
	lv_label_set_text(glbl, "GRN");
	lv_obj_set_style_text_font(glbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(glbl, C_WHITE, LV_PART_MAIN);
	lv_obj_center(glbl);

	lv_obj_t *blbl = lv_label_create(blue_block);
	lv_label_set_text(blbl, "BLU");
	lv_obj_set_style_text_font(blbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(blbl, C_WHITE, LV_PART_MAIN);
	lv_obj_center(blbl);

	/* Horizontal divider line */
	lv_obj_t *div = lv_obj_create(win);
	lv_obj_remove_style_all(div);
	lv_obj_set_size(div, 216, 2);
	lv_obj_set_pos(div, 4, 70);
	lv_obj_set_style_bg_color(div, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(div, LV_OPA_COVER, LV_PART_MAIN);

	/* Test text lines */
	make_label(win, &lv_font_unscii_16, C_WHITE,
		   LV_ALIGN_TOP_MID, 0, 76, "TFT OK");

	make_label(win, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_TOP_MID, 0, 96, "ST7789V 240x135");

	/* Corner markers to verify alignment */
	lv_obj_t *tl = lv_obj_create(win);
	lv_obj_remove_style_all(tl);
	lv_obj_set_size(tl, 4, 4);
	lv_obj_set_pos(tl, 2, 15);
	lv_obj_set_style_bg_color(tl, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tl, LV_OPA_COVER, LV_PART_MAIN);

	lv_obj_t *tr = lv_obj_create(win);
	lv_obj_remove_style_all(tr);
	lv_obj_set_size(tr, 4, 4);
	lv_obj_set_pos(tr, 216, 15);
	lv_obj_set_style_bg_color(tr, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tr, LV_OPA_COVER, LV_PART_MAIN);

	lv_obj_t *bl = lv_obj_create(win);
	lv_obj_remove_style_all(bl);
	lv_obj_set_size(bl, 4, 4);
	lv_obj_set_pos(bl, 2, 112);
	lv_obj_set_style_bg_color(bl, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bl, LV_OPA_COVER, LV_PART_MAIN);

	lv_obj_t *br = lv_obj_create(win);
	lv_obj_remove_style_all(br);
	lv_obj_set_size(br, 4, 4);
	lv_obj_set_pos(br, 216, 112);
	lv_obj_set_style_bg_color(br, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(br, LV_OPA_COVER, LV_PART_MAIN);
}

/* ---- Temperature test screen ---- */
static lv_obj_t *temp_val_lbl;
static lv_obj_t *temp_bar;
static lv_obj_t *temp_status_lbl;

void ui_show_temp_test(int temp_c)
{
	lv_obj_clean(scr);
	lv_obj_invalidate(scr);
	add_fullscreen_bg();

	/* ---- Retro window ---- */
	lv_obj_t *win = lv_obj_create(scr);
	lv_obj_remove_style_all(win);
	lv_obj_set_size(win, 224, 110);
	lv_obj_set_pos(win, 8, 22);
	lv_obj_set_style_bg_color(win, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(win, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(win, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(win, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_radius(win, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(win, 0, LV_PART_MAIN);

	/* Title bar */
	lv_obj_t *tbar = lv_obj_create(win);
	lv_obj_remove_style_all(tbar);
	lv_obj_set_size(tbar, 222, 14);
	lv_obj_set_pos(tbar, 1, 1);
	lv_obj_set_style_bg_color(tbar, C_ORANGE, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(tbar, 0, LV_PART_MAIN);

	lv_obj_t *tlbl = lv_label_create(tbar);
	lv_label_set_text(tlbl, " TEMP TEST");
	lv_obj_set_style_text_font(tlbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(tlbl, lv_color_black(), LV_PART_MAIN);
	lv_obj_align(tlbl, LV_ALIGN_LEFT_MID, 2, 0);

	/* Large temperature value */
	temp_val_lbl = lv_label_create(win);
	lv_label_set_text_fmt(temp_val_lbl, "%d C", temp_c);
	lv_obj_set_style_text_font(temp_val_lbl, &lv_font_unscii_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(temp_val_lbl, C_WHITE, LV_PART_MAIN);
	lv_obj_align(temp_val_lbl, LV_ALIGN_TOP_MID, 0, 20);

	/* Temperature bar background */
	lv_obj_t *bar_bg = lv_obj_create(win);
	lv_obj_remove_style_all(bar_bg);
	lv_obj_set_size(bar_bg, 200, 12);
	lv_obj_set_pos(bar_bg, 12, 50);
	lv_obj_set_style_bg_color(bar_bg, C_CARD, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bar_bg, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(bar_bg, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(bar_bg, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(bar_bg, C_TEXT_DIM, LV_PART_MAIN);

	/* Temperature bar fill */
	temp_bar = lv_obj_create(win);
	lv_obj_remove_style_all(temp_bar);
	lv_obj_set_size(temp_bar, 1, 10);
	lv_obj_set_pos(temp_bar, 13, 51);
	lv_obj_set_style_bg_color(temp_bar, C_ACCENT, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(temp_bar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(temp_bar, 0, LV_PART_MAIN);

	/* Scale labels */
	make_label(win, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_TOP_LEFT, 12, 66, "-40C");
	make_label(win, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_TOP_MID, 0, 66, "0C");
	make_label(win, &lv_font_unscii_8, C_TEXT_DIM,
		   LV_ALIGN_TOP_RIGHT, -12, 66, "85C");

	/* Status */
	temp_status_lbl = lv_label_create(win);
	lv_label_set_text(temp_status_lbl, "MEASURING...");
	lv_obj_set_style_text_font(temp_status_lbl, &lv_font_unscii_8, LV_PART_MAIN);
	lv_obj_set_style_text_color(temp_status_lbl, C_ORANGE, LV_PART_MAIN);
	lv_obj_align(temp_status_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

	/* Initial update */
	ui_update_temp_test(temp_c);
}

void ui_update_temp_test(int temp_c)
{
	if (!temp_val_lbl || !temp_bar || !temp_status_lbl) {
		return;
	}

	lv_label_set_text_fmt(temp_val_lbl, "%d C", temp_c);

	/* Map -40..85 C to bar width 0..198 px */
	int w = ((temp_c + 40) * 198) / 125;
	if (w < 0) {
		w = 0;
	}
	if (w > 198) {
		w = 198;
	}
	lv_obj_set_size(temp_bar, w, 10);

	/* Color based on range */
	if (temp_c < -20 || temp_c > 75) {
		lv_obj_set_style_bg_color(temp_bar, C_RED, LV_PART_MAIN);
		lv_label_set_text(temp_status_lbl, "CRITICAL");
		lv_obj_set_style_text_color(temp_status_lbl, C_RED, LV_PART_MAIN);
	} else if (temp_c < 0 || temp_c > 50) {
		lv_obj_set_style_bg_color(temp_bar, C_ORANGE, LV_PART_MAIN);
		lv_label_set_text(temp_status_lbl, "WARNING");
		lv_obj_set_style_text_color(temp_status_lbl, C_ORANGE, LV_PART_MAIN);
	} else {
		lv_obj_set_style_bg_color(temp_bar, C_ACCENT, LV_PART_MAIN);
		lv_label_set_text(temp_status_lbl, "NORMAL");
		lv_obj_set_style_text_color(temp_status_lbl, C_ACCENT, LV_PART_MAIN);
	}
}

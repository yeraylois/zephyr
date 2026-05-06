/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#define LONG_PRESS_MS 800

static volatile enum button_event pending_evt = BTN_EVT_NONE;
static struct k_work_delayable long_press_work;
static volatile bool long_fired;

static void long_press_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	long_fired = true;
	pending_evt = BTN_EVT_LONG;
}

static void button_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->sync == 0) {
		return;
	}

	if (evt->value) {
		long_fired = false;
		k_work_reschedule(&long_press_work, K_MSEC(LONG_PRESS_MS));
	} else {
		if (!long_fired) {
			k_work_cancel_delayable(&long_press_work);
			pending_evt = BTN_EVT_SHORT;
		}
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

void button_init(void)
{
	k_work_init_delayable(&long_press_work, long_press_handler);
	long_fired = false;
	pending_evt = BTN_EVT_NONE;
}

enum button_event button_get_event(void)
{
	enum button_event evt = pending_evt;

	pending_evt = BTN_EVT_NONE;
	return evt;
}

void button_poll(void)
{
}

/*
 * Copyright (c) 2026 Yeray Lois Sanchez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

enum button_event {
    BTN_EVT_NONE,
    BTN_EVT_SHORT,
    BTN_EVT_LONG,
};

void button_init(void);
void button_poll(void);
enum button_event button_get_event(void);

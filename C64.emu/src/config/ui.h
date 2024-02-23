#pragma once

#include <stdbool.h>

static int ui_pause_active() { return 0; }
static void ui_pause_enable() {}
static void ui_pause_disable() {}
static bool ui_pause_loop_iteration(void) { return false; }

#pragma once

#include <stdbool.h>

int ui_pause_active();
void ui_pause_enable();
void ui_pause_disable();
bool ui_pause_loop_iteration(void);

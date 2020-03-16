#pragma once

#include "vice-config.h"

void kbd_arch_init(void);

#define KBD_PORT_PREFIX "gtk3"

VICE_API signed long kbd_arch_keyname_to_keynum(char *keyname);
const char *kbd_arch_keynum_to_keyname(signed long keynum);
void kbd_initialize_numpad_joykeys(int *joykeys);

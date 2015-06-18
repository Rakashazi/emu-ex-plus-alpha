#pragma once

extern void kbd_arch_init(void);
extern int kbd_arch_get_host_mapping(void);

// dummy definition
#define KBD_PORT_PREFIX "sdl"

extern signed long kbd_arch_keyname_to_keynum(char *keyname);
extern const char *kbd_arch_keynum_to_keyname(signed long keynum);
extern void kbd_initialize_numpad_joykeys(int *joykeys);

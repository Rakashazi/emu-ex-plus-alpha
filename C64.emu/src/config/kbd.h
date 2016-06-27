#pragma once

extern int c64_kbd_init(void);
extern int c128_kbd_init(void);
extern int vic20_kbd_init(void);
extern int pet_kbd_init(void);
extern int plus4_kbd_init(void);
extern int cbm2_kbd_init(void);

extern int kbd_cmdline_options_init(void);
extern int kbd_resources_init(void);
extern int pet_kbd_cmdline_options_init(void);
extern int pet_kbd_resources_init(void);

extern void kbd_arch_init(void);
extern int kbd_arch_get_host_mapping(void);

// dummy definition
#define KBD_PORT_PREFIX "sdl"

extern signed long kbd_arch_keyname_to_keynum(char *keyname);
extern const char *kbd_arch_keynum_to_keyname(signed long keynum);
extern void kbd_initialize_numpad_joykeys(int *joykeys);

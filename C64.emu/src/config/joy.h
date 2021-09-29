#pragma once

void joystick(void);
int joy_arch_init(void);
static int joystick_init_cmdline_options(void) { return 0; }
static int joystick_arch_init_resources(void) { return 0; }
void joystick_close();

#define JOYDEV_NONE      0
#define JOYDEV_NUMPAD    1
#define JOYDEV_KEYSET1   2
#define JOYDEV_KEYSET2   3
#define JOYDEV_ANALOG_0  4
#define JOYDEV_ANALOG_1  5
#define JOYDEV_ANALOG_2  6
#define JOYDEV_ANALOG_3  7
#define JOYDEV_ANALOG_4  8
#define JOYDEV_ANALOG_5  9
#define JOYDEV_DIGITAL_0 10
#define JOYDEV_DIGITAL_1 11
#define JOYDEV_USB_0     12
#define JOYDEV_USB_1     13

#define JOYSTICK_KEYSET_NUM_KEYS     11 /* 4 directions, 4 diagonals, 3 fire */

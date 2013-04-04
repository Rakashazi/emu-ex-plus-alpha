/*
 * hummeradc.c -- Hummer ADC emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include "c64dtv-resources.h"
#include "cmdline.h"
#include "resources.h"
#include "log.h"
#include "hummeradc.h"
#include "keyboard.h"
#include "translate.h"

static log_t hummeradc_log = LOG_ERR;

#if 0
#define HUMMERADC_DEBUG(args ...) log_message(hummeradc_log, args)
#define HUMMERADC_DEBUG_ENABLED
#endif

/* Hummer ADC port bits */
#define ADC_START_BIT 0x04
#define ADC_CLOCK_BIT 0x02
#define ADC_DIO_BIT 0x01

/* Hummer ADC variables */
BYTE hummeradc_value;
BYTE hummeradc_channel;
BYTE hummeradc_control;
BYTE hummeradc_chanattr;
BYTE hummeradc_chanwakeup;
BYTE hummeradc_prev;

/* Hummer ADC state */
enum {
    ADC_IDLE=0,
    ADC_START,
    ADC_CMD1, ADC_CMD2, ADC_CMD3,

    /* Command 001/010 (Channel attribute/wakeup) states */
    ADC_CHAN7, ADC_CHAN6, ADC_CHAN5, ADC_CHAN4,
    ADC_CHAN3, ADC_CHAN2, ADC_CHAN1, ADC_CHAN0,

    /* Command 011 (Control register) states */
    ADC_CONTROL_PH, ADC_CONTROL_PL, /* Pull-up/down in digital input */
    ADC_CONTROL_RF, ADC_CONTROL_MB, /* Reference voltage source */ /* 13..16 */

    /* Command 100 (ADC conversion) states */
    ADC_CONV_CHAN1, ADC_CONV_CHAN2, ADC_CONV_CHAN3,
    ADC_CONV_ADC1, ADC_CONV_ADC2,
    ADC_CONV_D7, ADC_CONV_D6, ADC_CONV_D5, ADC_CONV_D4,
    ADC_CONV_D3, ADC_CONV_D2, ADC_CONV_D1, ADC_CONV_D0,
    ADC_CONV_PDS,

    /* Command 101 (Digital input reading) states */
    ADC_DINPUT_0,

    /* Command 000/111 (Powerdown 0/1) states */
    ADC_POWERDOWN,

    /* Command 110 (Reserved) states */
    ADC_RESERVED_S
}
hummeradc_state = ADC_IDLE;

/* Hummer ADC command */
enum {
    ADC_POWERDOWN0=0,
    ADC_CHAN_ATTR,
    ADC_CHAN_WAKEUP,
    ADC_CONTROL,
    ADC_CONVERSION,
    ADC_DINPUT,
    ADC_RESERVED,
    ADC_POWERDOWN1,
    ADC_NONE
}
hummeradc_command = ADC_NONE;

inline static int hummeradc_falling_edge(BYTE value)
{
    return ((hummeradc_prev & ADC_CLOCK_BIT) && ((value & ADC_CLOCK_BIT) == 0));
}

inline static int hummeradc_rising_edge(BYTE value)
{
    return (((hummeradc_prev & ADC_CLOCK_BIT) == 0) && (value & ADC_CLOCK_BIT));
}

void hummeradc_store(BYTE value)
{
#ifdef HUMMERADC_DEBUG_ENABLED
    HUMMERADC_DEBUG("write: value %02x, state %i", value, hummeradc_state);
#endif
    if (value & ADC_START_BIT) {
        hummeradc_state = ADC_START;
    }

    switch (hummeradc_state) {
        case ADC_CONV_PDS:
        case ADC_RESERVED_S:
        case ADC_POWERDOWN:
        case ADC_DINPUT_0:
        case ADC_IDLE:
            break;

        case ADC_START:
            if (hummeradc_rising_edge(value)) {
                hummeradc_state = ADC_CMD1;
                hummeradc_value = 0;
            }
            break;

        case ADC_CMD1:
        case ADC_CMD2:
        case ADC_CONV_CHAN1:
        case ADC_CONV_CHAN2:
        case ADC_CONTROL_PH:
        case ADC_CONTROL_PL:
        case ADC_CONTROL_RF:
        case ADC_CHAN7:
        case ADC_CHAN6:
        case ADC_CHAN5:
        case ADC_CHAN4:
        case ADC_CHAN3:
        case ADC_CHAN2:
        case ADC_CHAN1:
            if (hummeradc_falling_edge(value)) {
                hummeradc_value |= (value & ADC_DIO_BIT);
                hummeradc_value <<= 1;
            } else
            if (hummeradc_rising_edge(value)) {
                hummeradc_state++;
            }
            break;

        case ADC_CMD3:
            if (hummeradc_falling_edge(value)) {
                hummeradc_value |= (value & ADC_DIO_BIT);
            } else
            if (hummeradc_rising_edge(value)) {
                hummeradc_command = hummeradc_value;
                switch (hummeradc_command) {
                    case ADC_CHAN_ATTR:
                    case ADC_CHAN_WAKEUP:
                        hummeradc_state = ADC_CHAN7;
                        break;
                    case ADC_CONTROL:
                        hummeradc_state = ADC_CONTROL_PH;
                        break;
                    case ADC_CONVERSION:
                        hummeradc_state = ADC_CONV_CHAN1;
                        break;
                    case ADC_DINPUT:
                        hummeradc_state = ADC_DINPUT_0;
                        break;
                    case ADC_POWERDOWN0:
                    case ADC_POWERDOWN1:
                        hummeradc_state = ADC_POWERDOWN;
                        break;
                    case ADC_RESERVED:
                        hummeradc_state = ADC_IDLE;
                        break;
                    default:
                        log_message(hummeradc_log, "BUG: Unknown command %i.", hummeradc_command);
                        break;
                }
            }
            break;

        case ADC_CHAN0:
            if (hummeradc_falling_edge(value)) {
                hummeradc_value |= (value & ADC_DIO_BIT);
                if (hummeradc_command == ADC_CHAN_ATTR) {
                    hummeradc_chanattr = hummeradc_value;
                } else {
                    hummeradc_chanwakeup = hummeradc_value;
                }
            } else
            if (hummeradc_rising_edge(value)) {
                hummeradc_state = ADC_IDLE;
            }
            break;

        case ADC_CONTROL_MB:
            if (hummeradc_falling_edge(value)) {
                hummeradc_value |= (value & ADC_DIO_BIT);
                hummeradc_control = hummeradc_value;
            } else
            if (hummeradc_rising_edge(value)) {
                hummeradc_state = ADC_IDLE;
            }
            break;

        case ADC_CONV_CHAN3:
            if (hummeradc_falling_edge(value)) {
                hummeradc_value |= (value & ADC_DIO_BIT);
                hummeradc_channel = hummeradc_value;
            } else
            if (hummeradc_rising_edge(value)) {
                hummeradc_state++;
            }
            break;

        case ADC_CONV_ADC2:
            if (hummeradc_falling_edge(value)) {
                /* TODO:
                    - ADC works only on channel 0
                    - "inertia" (hold down left/right for value++/--), handled elsewhere */
                switch (joystick_value[3] & 0x0c) {
                    case 4:
                        hummeradc_value = 0x00;
                        break;
                    case 8:
                        hummeradc_value = 0xff;
                        break;
                    default:
                        hummeradc_value = 0x80;
                        break;
                }
            } else
            if (hummeradc_rising_edge(value)) {
                hummeradc_state = ADC_CONV_D7;
            }
            break;

        default:
            if (hummeradc_rising_edge(value)) {
                hummeradc_state++;
            }
            break;
    }
    hummeradc_prev = value;
    return;
}

BYTE hummeradc_read(void)
{
    BYTE retval;
    retval = (hummeradc_prev & 6);

    switch (hummeradc_state) {
        case ADC_CONV_D7:
            retval |= ((hummeradc_value >> 7) & 1);
            break;
        case ADC_CONV_D6:
            retval |= ((hummeradc_value >> 6) & 1);
            break;
        case ADC_CONV_D5:
            retval |= ((hummeradc_value >> 5) & 1);
            break;
        case ADC_CONV_D4:
            retval |= ((hummeradc_value >> 4) & 1);
            break;
        case ADC_CONV_D3:
            retval |= ((hummeradc_value >> 3) & 1);
            break;
        case ADC_CONV_D2:
            retval |= ((hummeradc_value >> 2) & 1);
            break;
        case ADC_CONV_D1:
            retval |= ((hummeradc_value >> 1) & 1);
            break;
        case ADC_CONV_D0:
            retval |= ((hummeradc_value) & 1);
            break;

        case ADC_POWERDOWN:
            retval |= (hummeradc_command & 1);
            break;

        case ADC_CONV_ADC1: /* TODO */
        case ADC_CONV_ADC2: /* TODO */
        case ADC_CONV_PDS: /* TODO */
        case ADC_RESERVED_S: /* TODO */
        case ADC_DINPUT_0: /* TODO */
        default:
            retval = hummeradc_prev;
            break;
    }
#ifdef HUMMERADC_DEBUG_ENABLED
    HUMMERADC_DEBUG(" read: value %02x, state %i", retval, hummeradc_state);
#endif
    return retval;
}

/* ------------------------------------------------------------------------- */

void hummeradc_init(void)
{
    if (hummeradc_log == LOG_ERR) {
        hummeradc_log = log_open("HUMMERADC");
    }
    hummeradc_reset();
    return;
}

void hummeradc_shutdown(void)
{
    return;
}

void hummeradc_reset(void)
{
    hummeradc_state = ADC_IDLE;
    hummeradc_command = ADC_NONE;
    hummeradc_value = 0;
    hummeradc_channel = 0;
    hummeradc_control = 0;
    hummeradc_prev = 0;
    hummeradc_chanattr = 0;
    hummeradc_chanwakeup = 0;
    return;
}

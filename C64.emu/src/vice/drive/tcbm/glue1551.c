/*
 * glue1551.c - 1551 glue logic.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "alarm.h"
#include "drive-writeprotect.h"
#include "drive.h"
#include "drivetypes.h"
#include "glue1551.h"
#include "interrupt.h"
#include "lib.h"
#include "rotation.h"
#include "types.h"
#include "drive-sound.h"

/*-----------------------------------------------------------------------*/

#define GLUE1551_ALARM_TICKS_ON   50
#define GLUE1551_ALARM_TICKS_OFF  19950

struct glue1551_s {
    alarm_t *timer_alarm;
    int irq_line;
    unsigned int int_num;
};
typedef struct glue1551_s glue1551_t;

static glue1551_t glue1551[DRIVE_NUM];

/*-----------------------------------------------------------------------*/

static void glue_pport_update(drive_context_t *drv)
{
    static BYTE old_output = 0;
    BYTE output, input;

    output = (drv->drive->drive_ram[1] & drv->drive->drive_ram[0])
             | ~(drv->drive->drive_ram[0]);

    /* Stepper motor.  */
    if (((old_output ^ output) & 0x3) && (output & 0x4)) {
        drive_move_head(((output - drv->drive->current_half_track + 3) & 3) - 1, drv->drive);
    }

    /* Motor on/off.  */
    if ((old_output ^ output) & 0x04) {
        drive_sound_update((output & 4) ? DRIVE_SOUND_MOTOR_ON : DRIVE_SOUND_MOTOR_OFF, drv->mynumber);
        drv->drive->byte_ready_active = (output & 0x04) ? 0x06 : 0;
        if (drv->drive->byte_ready_active == 0x06) {
            rotation_begins(drv->drive);
        }
    }

    /* Drive active LED.  */
    drv->drive->led_status = (output & 8) ? 0 : 1;

    if (drv->drive->led_status) {
        drv->drive->led_active_ticks += *(drv->clk_ptr)
                                        - drv->drive->led_last_change_clk;
    }
    drv->drive->led_last_change_clk = *(drv->clk_ptr);

    if ((old_output ^ output) & 0x60) {
        rotation_speed_zone_set((output >> 5) & 0x3, drv->mynumber);
    }

    rotation_rotate_disk(drv->drive);

    input = drive_writeprotect_sense(drv->drive)
            | (drv->drive->byte_ready_level ? 0x80 : 0);

    drv->drive->drive_ram[1] = output & (input | ~0x90);

    old_output = output;
}

BYTE glue1551_port0_read(drive_context_t *drv)
{
    glue_pport_update(drv);
    return drv->drive->drive_ram[0];
}

BYTE glue1551_port1_read(drive_context_t *drv)
{
    glue_pport_update(drv);
    return drv->drive->drive_ram[1];
}

void glue1551_port0_store(drive_context_t *drv, BYTE value)
{
    drv->drive->drive_ram[0] = value;
    glue_pport_update(drv);
}

void glue1551_port1_store(drive_context_t *drv, BYTE value)
{
    drv->drive->drive_ram[1] = value;
    glue_pport_update(drv);
}

/*-----------------------------------------------------------------------*/

static void glue1551_timer(CLOCK offset, void *data)
{
    drive_context_t *drv = (drive_context_t *)data;

    if (glue1551[drv->mynumber].irq_line == 0) {
        alarm_set(glue1551[drv->mynumber].timer_alarm, *(drv->clk_ptr)
                  + GLUE1551_ALARM_TICKS_ON - offset);
        interrupt_set_irq(drv->cpu->int_status,
                          glue1551[drv->mynumber].int_num,
                          IK_IRQ, *(drv->clk_ptr));
    } else {
        alarm_set(glue1551[drv->mynumber].timer_alarm, *(drv->clk_ptr)
                  + GLUE1551_ALARM_TICKS_OFF - offset);
        interrupt_set_irq(drv->cpu->int_status,
                          glue1551[drv->mynumber].int_num,
                          0, *(drv->clk_ptr));
    }
    glue1551[drv->mynumber].irq_line ^= 1;
}

void glue1551_init(drive_context_t *drv)
{
    char *buffer;

    buffer = lib_msprintf("GLUE1551D%i", drv->mynumber);

    glue1551[drv->mynumber].timer_alarm = alarm_new(drv->cpu->alarm_context,
                                                    buffer, glue1551_timer,
                                                    drv);
    glue1551[drv->mynumber].int_num = interrupt_cpu_status_int_new(
        drv->cpu->int_status, buffer);
    lib_free(buffer);
}

void glue1551_reset(drive_context_t *drv)
{
    alarm_unset(glue1551[drv->mynumber].timer_alarm);
    alarm_set(glue1551[drv->mynumber].timer_alarm,
              *(drv->clk_ptr) + GLUE1551_ALARM_TICKS_OFF);
    glue1551[drv->mynumber].irq_line = 0;

    drv->drive->led_status = 1;
    drive_update_ui_status();
}

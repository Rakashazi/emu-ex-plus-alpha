/*
 * c1541-stubs.c - dummies for unneeded/unused functions
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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
#include "archdep_defs.h"
#include <stdbool.h>
#include <inttypes.h>

#include "attach.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "drive.h"
#include "imagecontents.h"
#include "kbd.h"
#include "machine.h"
#include "machine-bus.h"
#include "machine-drive.h"
#include "main.h"
#include "mainlock.h"
#include "network.h"
#include "serial.h"
#include "tape.h"
#include "vdrive.h"
#include "vice-event.h"
#include "vsync.h"
#include "uiapi.h"

/*
   FIXME: these really shouldnt be needed here and are a sign of bad modular
          design elsewhere
 */

/* called from resources_set_defaults() */
void cartridge_detach_image(int type)
{
}

/* called from resources_set_defaults() */
void cartridge_unset_default(void)
{
}

int cmdline_register_options(const cmdline_option_t *c)
{
    return 0;
}

int network_connected(void)
{
    return 0;
}

int network_get_mode(void)
{
#if 0
    return NETWORK_IDLE;
#else
    return 0;
#endif
}

void network_event_record(unsigned int type, void *data, unsigned int size)
{

}

void event_record_in_list(event_list_state_t *list, unsigned int type,
                          void *data, unsigned int size)
{

}

void ui_error(const char *format, ...)
{

}

void main_exit(void)
{

}

#ifdef USE_VICE_THREAD
bool mainlock_is_vice_thread(void)
{

    return false;
}

void mainlock_initiate_shutdown(void)
{
}
#endif

#if 0
void enable_text(void)
{
}

void disable_text(void)
{
}
#endif

int machine_bus_device_attach(unsigned int unit, const char *name,
                                     int (*getf)(struct vdrive_s *,
                                                 uint8_t *, unsigned int),
                                     int (*putf)(struct vdrive_s *, uint8_t,
                                                 unsigned int),
                                     int (*openf)(struct vdrive_s *,
                                                  const uint8_t *, unsigned int,
                                                  unsigned int,
                                                  struct cbmdos_cmd_parse_s *),
                                     int (*closef)(struct vdrive_s *,
                                                   unsigned int),
                                     void (*flushf)(struct vdrive_s *,
                                                    unsigned int),
                                     void (*listenf)(struct vdrive_s *,
                                                     unsigned int))
{
    return 0;
}

snapshot_module_t *snapshot_module_create(snapshot_t *s, const char *name, uint8_t major_version, uint8_t minor_version)
{
    return NULL;
}

snapshot_module_t *snapshot_module_open(snapshot_t *s, const char *name, uint8_t *major_version_return, uint8_t *minor_version_return)
{
    return NULL;
}

int snapshot_module_close(snapshot_module_t *m)
{
    return 0;
}

int snapshot_module_read_dword_into_int(snapshot_module_t *m, int *value_return)
{
    return 0;
}

int snapshot_module_read_dword_into_uint(snapshot_module_t *m, unsigned int *value_return)
{
    return 0;
}


int snapshot_module_write_byte_array(snapshot_module_t *m,
                                     const uint8_t *data,
                                     unsigned int num)
{
    return 0;
}

int snapshot_module_read_byte_array(snapshot_module_t *m,
                                    uint8_t *b_return,
                                    unsigned int num)
{
    return 0;
}

int snapshot_module_write_word(snapshot_module_t *m, uint16_t data)
{
    return 0;
}

int snapshot_module_read_word_into_int(snapshot_module_t *m,
                                       int *value_return)
{
    return 0;
}

int snapshot_version_is_smaller(uint8_t major_version,
                                uint8_t minor_version,
                                uint8_t major_version_required,
                                uint8_t minor_version_required)
{
    return 0;
}

int snapshot_version_is_equal(uint8_t major_version,
                              uint8_t minor_version,
                              uint8_t major_version_required,
                              uint8_t minor_version_required)
{
    return 0;
}

int snapshot_version_is_bigger(uint8_t major_version,
                               uint8_t minor_version,
                               uint8_t major_version_required,
                               uint8_t minor_version_required)
{
    return 0;
}

void snapshot_set_error(int error)
{
    /* NOP */
}

int snapshot_get_error(void)
{
    return 0;
}




#if 0
void ui_error_string(const char *text)
{
}
#endif

void vsync_suspend_speed_eval(void)
{
}

struct image_contents_s *machine_diskcontents_bus_read(unsigned int unit)
{
    return diskcontents_iec_read(unit);
}

int machine_bus_lib_directory(unsigned int unit, const char *pattern, uint8_t **buf)
{
    return serial_iec_lib_directory(unit, pattern, buf);
}

int machine_bus_lib_read_sector(unsigned int unit, unsigned int track, unsigned int sector, uint8_t *buf)
{
    return serial_iec_lib_read_sector(unit, track, sector, buf);
}

int machine_bus_lib_write_sector(unsigned int unit, unsigned int track, unsigned int sector, uint8_t *buf)
{
    return serial_iec_lib_write_sector(unit, track, sector, buf);
}

unsigned int machine_bus_device_type_get(unsigned int unit)
{
    return serial_device_type_get(unit);
}

int machine_drive_rom_check_loaded(unsigned int type)
{
    return 0;
}

void machine_drive_flush(void)
{
}

/** \brief  Machine name
 */
const char machine_name[] = "C1541";

/** \brief  Machine class
 */
int machine_class = 0;

const char *machine_get_name(void)
{
    return machine_name;
}

uint8_t machine_tape_behaviour(void)
{
    return TAPE_BEHAVIOUR_NORMAL;
}

#if defined(USE_SDLUI) || defined(USE_SDLUI2)
char *kbd_get_menu_keyname(void)
{
    return NULL;
}
#endif

int crt_getid(const char *filename)
{
    return -1;
}


void ui_hotkeys_init(void)
{
}

/*
 * serial.h - Serial device implementation.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_SERIAL_H
#define VICE_SERIAL_H

#include "types.h"

#define SERIAL_MAXDEVICES 16

#define SERIAL_DEVICE_NONE 0
#define SERIAL_DEVICE_FS   1 /* filesystem */
#define SERIAL_DEVICE_REAL 2 /* real IEC device (opencbm) */
#define SERIAL_DEVICE_RAW  3 /* raw device */
#define SERIAL_DEVICE_VIRT 4 /* non-tde drive/image */

struct cbmdos_cmd_parse_s;
struct disk_image_s;
struct trap_s;
struct vdrive_s;
struct snapshot_s;

typedef struct serial_s {
    int inuse;
    int isopen[16]; /* isopen flag for each secondary address */
    struct disk_image_s *image; /* pointer to the disk image data  */
    char *name; /* name of the device */
    int (*getf)(struct vdrive_s *, uint8_t *, unsigned int);
    int (*putf)(struct vdrive_s *, uint8_t, unsigned int);
    int (*openf)(struct vdrive_s *, const uint8_t *, unsigned int, unsigned int,
                 struct cbmdos_cmd_parse_s *cmd_parse_ext);
    int (*closef)(struct vdrive_s *, unsigned int);
    void (*flushf)(struct vdrive_s *, unsigned int);
    void (*listenf)(struct vdrive_s *, unsigned int);
    uint8_t nextbyte[16]; /* next byte to send, per sec. addr. */
    char nextok[16]; /* flag if nextbyte is valid */

    int nextst[16];
    unsigned int device;

    /* The PET hardware emulation can be interrupted while
       transferring a byte. Thus we also have to save the byte
       and status last sent, to be able to send it again. */
    uint8_t lastbyte[16];
    char lastok[16];
    int lastst[16];
} serial_t;

#define ISOPEN_CLOSED           0
#define ISOPEN_AWAITING_NAME    1
#define ISOPEN_OPEN             2

int serial_init(const struct trap_s *trap_list);
int serial_resources_init(void);
int serial_cmdline_options_init(void);
void serial_shutdown(void);
int serial_install_traps(void);
int serial_remove_traps(void);

void serial_trap_init(uint16_t tmpin);
int serial_trap_attention(void);
int serial_trap_send(void);
int serial_trap_receive(void);
int serial_trap_ready(void);
void serial_traps_reset(void);
void serial_trap_eof_callback_set(void (*func)(void));
void serial_trap_attention_callback_set(void (*func)(void));
void serial_trap_truedrive_set(unsigned int unit, unsigned int flag);

int serial_realdevice_enable(void);
void serial_realdevice_disable(void);

int serial_iec_lib_directory(unsigned int unit, const char *pattern, uint8_t **buf);
int serial_iec_lib_read_sector(unsigned int unit, unsigned int track, unsigned int sector, uint8_t *buf);
int serial_iec_lib_write_sector(unsigned int unit, unsigned int track, unsigned int sector, uint8_t *buf);

serial_t *serial_device_get(unsigned int unit);
unsigned int serial_device_type_get(unsigned int unit);
void serial_device_type_set(unsigned int type, unsigned int unit);

void serial_iec_device_set_machine_parameter(long cycles_per_sec);
void serial_iec_device_exec(CLOCK clk_value);

void serial_iec_bus_init(void);

void fsdrive_snapshot_prepare(void);
int fsdrive_snapshot_write_module(struct snapshot_s *s);
int fsdrive_snapshot_read_module(struct snapshot_s *s);

#endif

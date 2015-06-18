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

typedef struct serial_s {
    int inuse;
    int isopen[16]; /* isopen flag for each secondary address */
    struct disk_image_s *image; /* pointer to the disk image data  */
    char *name; /* name of the device */
    int (*getf)(struct vdrive_s *, BYTE *, unsigned int);
    int (*putf)(struct vdrive_s *, BYTE, unsigned int);
    int (*openf)(struct vdrive_s *, const BYTE *, unsigned int, unsigned int,
                 struct cbmdos_cmd_parse_s *cmd_parse_ext);
    int (*closef)(struct vdrive_s *, unsigned int);
    void (*flushf)(struct vdrive_s *, unsigned int);
    void (*listenf)(struct vdrive_s *, unsigned int);
    BYTE nextbyte[16]; /* next byte to send, per sec. addr. */
    char nextok[16]; /* flag if nextbyte is valid */

    int nextst[16];
    unsigned int device;

    /* The PET hardware emulation can be interrupted while
       transferring a byte. Thus we also have to save the byte
       and status last sent, to be able to send it again. */
    BYTE lastbyte[16];
    char lastok[16];
    int lastst[16];
} serial_t;

extern int serial_init(const struct trap_s *trap_list);
extern int serial_resources_init(void);
extern int serial_cmdline_options_init(void);
extern void serial_shutdown(void);
extern int serial_install_traps(void);
extern int serial_remove_traps(void);

extern void serial_trap_init(WORD tmpin);
extern int serial_trap_attention(void);
extern int serial_trap_send(void);
extern int serial_trap_receive(void);
extern int serial_trap_ready(void);
extern void serial_traps_reset(void);
extern void serial_trap_eof_callback_set(void (*func)(void));
extern void serial_trap_attention_callback_set(void (*func)(void));
extern void serial_trap_truedrive_set(unsigned int flag);

extern int serial_realdevice_enable(void);
extern void serial_realdevice_disable(void);

extern int serial_iec_lib_directory(unsigned int unit, const char *pattern,
                                    BYTE **buf);
extern int serial_iec_lib_read_sector(unsigned int unit, unsigned int track,
                                      unsigned int sector, BYTE *buf);
extern int serial_iec_lib_write_sector(unsigned int unit, unsigned int track,
                                       unsigned int sector, BYTE *buf);

extern serial_t *serial_device_get(unsigned int unit);
extern unsigned int serial_device_type_get(unsigned int unit);
extern void serial_device_type_set(unsigned int type, unsigned int unit);

extern void serial_iec_device_set_machine_parameter(long cycles_per_sec);
extern void serial_iec_device_exec(CLOCK clk_value);

extern void serial_iec_bus_init(void);

#endif

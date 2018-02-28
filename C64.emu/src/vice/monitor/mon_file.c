/*
 * mon_file.c - The VICE built-in monitor file functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "attach.h"
#include "autostart.h"
#include "cartridge.h"
#include "charset.h"
#include "machine.h"
#include "mem.h"
#include "montypes.h"
#include "mon_file.h"
#include "mon_util.h"
#include "tape.h"
#include "uimon.h"
#include "vdrive-iec.h"
#include "vdrive.h"


#define ADDR_LIMIT(x) ((WORD)(addr_mask(x)))

#define curbank (mon_interfaces[mem]->current_bank)

static FILE *fp;
static vdrive_t *vdrive;
/* we require an EOF buffer to support CBM EOFs. */
static int mon_file_read_eof[4][16];

static int mon_file_open(const char *filename, unsigned int secondary,
                         int device)
{
    char pname[17];
    const char *s;
    int i;

    switch (device) {
        case 0:
            if (secondary == 0) {
                fp = fopen(filename, MODE_READ);
            } else {
                fp = fopen(filename, MODE_WRITE);
            }
            if (fp == NULL) {
                return -1;
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            vdrive = file_system_get_vdrive((unsigned int)device);
            if (vdrive == NULL) {
                return -1;
            }
            /* convert filename to petscii */
            s = filename;
            for (i = 0; (i < 16) && (*s); ++i) {
                pname[i] = charset_p_topetcii(*s);
                ++s;
            }
            pname[i] = 0;

            if (vdrive_iec_open(vdrive, (const BYTE *)pname,
                                (int)strlen(pname), secondary, NULL) != SERIAL_OK) {
                return -1;
            }

            /* initialize EOF buffer. */
            mon_file_read_eof[device - 8][secondary] = 0;
            break;
        default:
            return -1;
    }
    return 0;
}

static int mon_file_read(BYTE *data, unsigned int secondary, int device)
{
    switch (device) {
        case 0:
            if (fread((char *)data, 1, 1, fp) < 1) {
                return -1;
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            /* Return EOF if we hit a CBM EOF on the last read. */
            if (mon_file_read_eof[device - 8][secondary]) {
                *data = 0xc7;
                return -1;
            }
            /* Set next EOF based on CBM EOF. */
            mon_file_read_eof[device - 8][secondary] =
                vdrive_iec_read(vdrive, data, secondary);
            break;
    }
    return 0;
}

static int mon_file_write(BYTE data, unsigned int secondary, int device)
{
    switch (device) {
        case 0:
            if (fwrite((char *)&data, 1, 1, fp) < 1) {
                return -1;
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            if (vdrive_iec_write(vdrive, data, secondary) != SERIAL_OK) {
                return -1;
            }
            break;
    }
    return 0;
}

static int mon_file_close(unsigned int secondary, int device)
{
    switch (device) {
        case 0:
            if (fclose(fp) != 0) {
                return -1;
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            if (vdrive_iec_close(vdrive, secondary) != SERIAL_OK) {
                return -1;
            }
            break;
    }
    return 0;
}


void mon_file_load(const char *filename, int device, MON_ADDR start_addr,
                   bool is_bload)
{
    WORD adr, load_addr = 0, basic_addr;
    BYTE b1 = 0, b2 = 0;
    int ch = 0;
    MEMSPACE mem;
    int origbank = 0;

    if (mon_file_open(filename, 0, device) < 0) {
        mon_out("Cannot open %s.\n", filename);
        return;
    }

    /* if loading a .prg file, read/skip the start address */
    if (is_bload == FALSE) {
        mon_file_read(&b1, 0, device);
        mon_file_read(&b2, 0, device);
        load_addr = (BYTE)b1 | ((BYTE)b2 << 8);
    }

    mem_get_basic_text(&basic_addr, NULL); /* get BASIC start */
    mon_evaluate_default_addr(&start_addr); /* get target addr given in monitor */

    if (!mon_is_valid_addr(start_addr)) {   /* No Load address given */
        if (is_bload == TRUE) {
            /* when loading plain binary, load addr is required */
            mon_out("No LOAD address given.\n");
            mon_file_close(0, device);
            return;
        }

        if (load_addr == basic_addr) {   /* Load to BASIC start */
            adr = basic_addr;
            mem = e_comp_space;
        } else {
            start_addr = new_addr(e_default_space, load_addr);
            mon_evaluate_default_addr(&start_addr);
            adr = addr_location(start_addr);
            mem = addr_memspace(start_addr);
        }
    } else {
        adr = addr_location(start_addr);
        mem = addr_memspace(start_addr);
    }

    mon_out("Loading %s", filename);
    mon_out(" from %04X\n", adr);

    if (machine_class == VICE_MACHINE_C64DTV) {
        origbank = curbank;
    }

    do {
        BYTE load_byte;

        if (mon_file_read(&load_byte, 0, device) < 0) {
            break;
        }
        mon_set_mem_val(mem, ADDR_LIMIT(adr + ch), load_byte);

        /* Hack to be able to read large .prgs for x64dtv */
        if ((machine_class == VICE_MACHINE_C64DTV) &&
            (ADDR_LIMIT(adr + ch) == 0xffff) &&
            ((curbank >= mem_bank_from_name("ram00")) && (curbank <= mem_bank_from_name("ram1f")))) {
            curbank++;
            if (curbank > mem_bank_from_name("ram1f")) {
                curbank = mem_bank_from_name("ram00");
            }
            mon_out("Crossing 64k boundary.\n");
        }
        ch++;
    }
    while (1);

    if (machine_class == VICE_MACHINE_C64DTV) {
        curbank = origbank;
    }

    mon_out("to %04X (%x bytes)\n", ADDR_LIMIT(adr + ch), ch);

    /* set end of load addresses like kernal load if
     * 1. loading .prg file
     * 2. loading to BASIC start
     * 3. loading to computer bank/memory
     */
    if ((is_bload == FALSE) && (load_addr == basic_addr) && (mem == e_comp_space)) {
        mem_set_basic_text(adr, (WORD)(adr + ch));
    }

    mon_file_close(0, device);
}

void mon_file_save(const char *filename, int device, MON_ADDR start_addr,
                   MON_ADDR end_addr, bool is_bsave)
{
    WORD adr, end;
    long len;
    int ch = 0;
    MEMSPACE mem;

    len = mon_evaluate_address_range(&start_addr, &end_addr, TRUE, -1);

    if (len < 0) {
        mon_out("Invalid range.\n");
        return;
    }

    mem = addr_memspace(start_addr);
    adr = addr_location(start_addr);
    end = addr_location(end_addr);

    if (end < adr) {
        mon_out("Start address must be below end address.\n");
        return;
    }

    if (mon_file_open(filename, 1, device) < 0) {
        mon_out("Cannot open %s.\n", filename);
        return;
    }

    printf("Saving file `%s'...\n", filename);

    if (is_bsave == FALSE) {
        if (mon_file_write((BYTE)(adr & 0xff), 1, device) < 0
            || mon_file_write((BYTE)((adr >> 8) & 0xff), 1, device) < 0) {
            mon_out("Saving for `%s' failed.\n", filename);
            mon_file_close(1, device);
            return;
        }
    }

    do {
        unsigned char save_byte;

        save_byte = mon_get_mem_val(mem, (WORD)(adr + ch));
        if (mon_file_write(save_byte, 1, device) < 0) {
            mon_out("Saving for `%s' failed.\n", filename);
            break;
        }
        ch++;
    } while ((adr + ch) <= end);

    mon_file_close(1, device);
}

/* Where is the implementation?  */
void mon_file_verify(const char *filename, int device, MON_ADDR start_addr)
{
    mon_evaluate_default_addr(&start_addr);

    mon_out("Verify file %s at address $%04x\n",
            filename, addr_location(start_addr));
}


void mon_attach(const char *filename, int device)
{
    switch (device) {
        case 1:
            if (machine_class == VICE_MACHINE_C64DTV) {
                mon_out("Unimplemented.\n");
            } else if (tape_image_attach(device, filename)) {
                mon_out("Failed.\n");
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            if (file_system_attach_disk(device, filename)) {
                mon_out("Failed.\n");
            }
            break;
        case 32:
            if (mon_cart_cmd.cartridge_attach_image != NULL) {
                if ((mon_cart_cmd.cartridge_attach_image)(CARTRIDGE_CRT, filename)) {
                    mon_out("Failed.\n");
                }
            } else {
                mon_out("Unsupported.\n");
            }
            break;
        default:
            mon_out("Unknown device %i.\n", device);
            break;
    }
}

void mon_detach(int device)
{
    switch (device) {
        case 1:
            if (machine_class == VICE_MACHINE_C64DTV) {
                mon_out("Unimplemented.\n");
            } else {
                tape_image_detach(device);
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            file_system_detach_disk(device);
            break;
        case 32:
            if (mon_cart_cmd.cartridge_detach_image != NULL) {
                (mon_cart_cmd.cartridge_detach_image)(-1); /* FIXME: param should be cart id, -1 detaches all */
            } else {
                mon_out("Unsupported.\n");
            }
            break;
        default:
            mon_out("Unknown device %i.\n", device);
            break;
    }
}

void mon_autostart(const char *image_name,
                   int file_index,
                   int run)
{
    mon_out("auto%s %s #%d\n", run ? "starting" : "loading",
            image_name, file_index);
    autostart_autodetect_opt_prgname(image_name, file_index,
                                     run ? AUTOSTART_MODE_RUN : AUTOSTART_MODE_LOAD);

    /* leave monitor but return after autostart */
    autostart_trigger_monitor(1);
    exit_mon = 1;
}

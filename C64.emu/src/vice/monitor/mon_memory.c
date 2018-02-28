/*
 * mon_memory.c - The VICE built-in monitor memory functions.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "console.h"
#include "lib.h"
#include "montypes.h"
#include "mon_memory.h"
#include "mon_util.h"
#include "types.h"


#define ADDR_LIMIT(x) ((WORD)(addr_mask(x)))

void mon_memory_move(MON_ADDR start_addr, MON_ADDR end_addr, MON_ADDR dest)
{
    unsigned int i, dst;
    int len;
    WORD start;
    MEMSPACE src_mem, dest_mem;
    BYTE *buf;

    len = mon_evaluate_address_range(&start_addr, &end_addr, TRUE, -1);
    if (len <= 0) {
        mon_out("Invalid range.\n");
        return;
    }
    src_mem = addr_memspace(start_addr);
    start = addr_location(start_addr);

    mon_evaluate_default_addr(&dest);
    dst = addr_location(dest);
    dest_mem = addr_memspace(dest);

    buf = lib_malloc(sizeof(BYTE) * len);

    for (i = 0; (int)i < len; i++) {
        buf[i] = mon_get_mem_val(src_mem, (WORD)ADDR_LIMIT(start + i));
    }

    for (i = 0; (int)i < len; i++) {
        mon_set_mem_val(dest_mem, (WORD)ADDR_LIMIT(dst + i), buf[i]);
    }

    lib_free(buf);
}

void mon_memory_compare(MON_ADDR start_addr, MON_ADDR end_addr, MON_ADDR dest)
{
    WORD start;
    MEMSPACE src_mem, dest_mem;
    BYTE byte1, byte2;
    unsigned int i, dst;
    int len;

    len = mon_evaluate_address_range(&start_addr, &end_addr, TRUE, -1);
    if (len < 0) {
        mon_out("Invalid range.\n");
        return;
    }
    src_mem = addr_memspace(start_addr);
    start = addr_location(start_addr);

    mon_evaluate_default_addr(&dest);
    dst = addr_location(dest);
    dest_mem = addr_memspace(dest);

    for (i = 0; (int)i < len; i++) {
        byte1 = mon_get_mem_val(src_mem, (WORD)ADDR_LIMIT(start + i));
        byte2 = mon_get_mem_val(dest_mem, (WORD)ADDR_LIMIT(dst + i));

        if (byte1 != byte2) {
            mon_out("$%04x $%04x: %02x %02x\n",
                    ADDR_LIMIT(start + i), ADDR_LIMIT(dst + i), byte1, byte2);
        }
    }
}

void mon_memory_fill(MON_ADDR start_addr, MON_ADDR end_addr,
                     unsigned char *data)
{
    WORD start;
    MEMSPACE dest_mem;
    unsigned int i, mon_index;
    int len;

    len = mon_evaluate_address_range(&start_addr, &end_addr, FALSE,
                                     (WORD)data_buf_len);
    if (len < 0) {
        mon_out("Invalid range.\n");
        return;
    }
    start = addr_location(start_addr);

    if (!mon_is_valid_addr(start_addr)) {
        mon_out("Invalid start address\n");
        return;
    }

    dest_mem = addr_memspace(start_addr);

    i = 0;
    mon_index = 0;
    while ((int)i < len) {
        mon_set_mem_val(dest_mem, (WORD)ADDR_LIMIT(start + i),
                        data_buf[mon_index++]);
        if (mon_index >= data_buf_len) {
            mon_index = 0;
        }
        i++;
    }

    mon_clear_buffer();
}

void mon_memory_hunt(MON_ADDR start_addr, MON_ADDR end_addr,
                     unsigned char *data)
{
    BYTE *buf;
    WORD start, next_read;
    MEMSPACE mem;
    unsigned int i;
    int len;

    len = mon_evaluate_address_range(&start_addr, &end_addr, TRUE, -1);
    if (len < 0 || len < (int)(data_buf_len)) {
        mon_out("Invalid range.\n");
        return;
    }
    mem = addr_memspace(start_addr);
    start = addr_location(start_addr);

    buf = lib_malloc(sizeof(BYTE) * data_buf_len);

    /* Fill buffer */
    for (i = 0; i < data_buf_len; i++) {
        buf[i] = mon_get_mem_val(mem, (WORD)ADDR_LIMIT(start + i));
    }

    /* Do compares */
    next_read = start + (WORD)data_buf_len;

    for (i = 0; i <= (len - data_buf_len); i++, next_read++) {
        int not_found = 0;
        unsigned int j;
        for (j = 0; j < data_buf_len; j++) {
            if ((buf[j] & data_mask_buf[j]) != data_buf[j]) {
                not_found = 1;
                break;
            }
        }
        if (!not_found) {
            mon_out("%04x\n", ADDR_LIMIT(start + i));
        }

        if (data_buf_len > 1) {
            memmove(&(buf[0]), &(buf[1]), data_buf_len - 1);
        }
        buf[data_buf_len - 1] = mon_get_mem_val(mem, next_read);
    }

    mon_clear_buffer();
    lib_free(buf);
}

static const int radix_chars_per_byte[] = {
    2, /* default = hex */
    2, /* hexadecimal */
    3, /* decimal */
    3, /* octal */
    8, /* binary */
};


static void memory_to_string(char *buf, MEMSPACE mem, WORD addr,
                             unsigned int len, bool petscii)
{
    unsigned int i;
    BYTE val;

    for (i = 0; i < len; i++) {
        val = mon_get_mem_val(mem, addr);

        if (petscii) {
            buf[i] = charset_p_toascii(val, 0);
        }

        buf[i] = isprint(val) ? val : '.';

        addr++;
    }
}

static void set_addr_location(MON_ADDR *a, unsigned l)
{
    *a = new_addr(addr_memspace(*a), addr_mask(l));
}

void mon_memory_display(int radix_type, MON_ADDR start_addr, MON_ADDR end_addr, mon_display_format_t format)
{
    unsigned int i, cnt = 0, len, max_width, real_width;
    WORD addr = 0;
    char printables[50];
    char prefix;
    MEMSPACE mem;
    WORD display_number;
    BYTE v;

    prefix = (format == DF_PETSCII) ? '>' : '*';

    if (radix_type) {
        if (radix_type != e_hexadecimal && radix_type != e_decimal && radix_type != e_octal) {
            max_width = (console_log->console_xres - 12)
                        / (radix_chars_per_byte[radix_type] + 2);
        } else {
            max_width = (4 * (console_log->console_xres - 12))
                        / (4 * (radix_chars_per_byte[radix_type] + 2) + 1);
        }

        max_width &= ~3;

        display_number = max_width * ((console_log->console_yres - 6) / 2);
    } else {
        max_width = 40;
        display_number = 128;
    }

    len = mon_evaluate_address_range(&start_addr, &end_addr, FALSE,
                                     display_number);
    mem = addr_memspace(start_addr);
    addr = addr_location(start_addr);

    while (cnt < len) {
        mon_out("%c%s:%04x ", prefix, mon_memspace_string[mem], addr);
        for (i = 0, real_width = 0; i < max_width; i++) {
            v = mon_get_mem_val(mem, (WORD)ADDR_LIMIT(addr + i));

            switch (radix_type) {
                case 0: /* special case == petscii text */
                    if (format == DF_PETSCII) {
                        mon_out("%c", charset_p_toascii(v, 1));
                    } else {
                        mon_out("%c", charset_p_toascii(
                                    charset_screencode_to_petcii(v), 1));
                    }
                    real_width++;
                    cnt++;
                    break;
                case e_decimal:
                    memset(printables, 0, 50);
                    if (!(cnt % 4)) {
                        mon_out(" ");
                    }
                    if (cnt < len) {
                        mon_out("%03d ", v);
                        real_width++;
                        cnt++;
                    } else {
                        mon_out("    ");
                    }
                    break;
                case e_hexadecimal:
                    memset(printables, 0, 50);
                    if (!(cnt % 4)) {
                        mon_out(" ");
                    }
                    if (cnt < len) {
                        mon_out("%02x ", v);
                        real_width++;
                    } else {
                        mon_out("   ");
                    }
                    cnt++;
                    break;
                case e_octal:
                    memset(printables, 0, 50);
                    if (!(cnt % 4)) {
                        mon_out(" ");
                    }
                    if (cnt < len) {
                        mon_out("%03o ", v);
                        real_width++;
                        cnt++;
                    } else {
                        mon_out("    ");
                    }
                    break;
                case e_binary:
                    memset(printables, 0, 50);
                    if (cnt < len) {
                        mon_print_bin(v, '1', '0');
                        mon_out(" ");
                        real_width++;
                        cnt++;
                    } else {
                        mon_out("         ");
                    }
                    break;
                default:
                    return;
            }
        }

        if (radix_type != 0) {
            memory_to_string(printables, mem, addr, real_width, FALSE);
            mon_out("  %s", printables);
        }
        mon_out("\n");
        addr = ADDR_LIMIT(addr + real_width);
        if (mon_stop_output != 0) {
            break;
        }
    }

    set_addr_location(&(dot_addr[mem]), addr);
}

/* display binary data (sprites/chars) */
void mon_memory_display_data(MON_ADDR start_addr, MON_ADDR end_addr,
                             unsigned int x, unsigned int y)
{
    unsigned i, j, len, cnt = 0;
    WORD addr = 0;
    MEMSPACE mem;

    len = mon_evaluate_address_range(&start_addr, &end_addr, FALSE,
                                     (WORD)((x * y) / 8));
    mem = addr_memspace(start_addr);
    addr = addr_location(start_addr);

    while (cnt < len) {
        for (i = 0; i < y; i++) {
            mon_out(">%s:%04x ", mon_memspace_string[mem], addr);
            for (j = 0; j < (x / 8); j++) {
                mon_print_bin(mon_get_mem_val(mem,
                                              (WORD)(ADDR_LIMIT(addr + j))), '.', '*');
                cnt++;
            }
            mon_out("\n");
            addr = ADDR_LIMIT(addr + (x / 8));
            if (mon_stop_output != 0) {
                break;
            }
        }

        mon_out("\n");
        if (mon_stop_output != 0) {
            break;
        }
    }

    if ((x == 24) && (y == 21)) {
        addr++; /* continue at next even address when showing sprites */
    }
    set_addr_location(&(dot_addr[mem]), addr);
}

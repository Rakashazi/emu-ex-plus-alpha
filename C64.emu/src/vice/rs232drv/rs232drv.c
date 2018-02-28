/*
 * rs232drv.c - Common RS232 driver handling.
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

#include "archdep.h"
#include "cmdline.h"
#include "lib.h"
#include "resources.h"
#include "rs232.h"
#include "rs232drv.h"
#include "translate.h"
#include "types.h"
#include "util.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)

char *rs232_devfile[RS232_NUM_DEVICES] = { NULL };

static int set_devfile(const char *val, void *param)
{
    util_string_set(&rs232_devfile[vice_ptr_to_int(param)], val);
    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_string_t resources_string[] = {
    { "RsDevice1", ARCHDEP_RS232_DEV1, RES_EVENT_NO, NULL,
      &rs232_devfile[0], set_devfile, (void *)0 },
    { "RsDevice2", ARCHDEP_RS232_DEV2, RES_EVENT_NO, NULL,
      &rs232_devfile[1], set_devfile, (void *)1 },
    { "RsDevice3", ARCHDEP_RS232_DEV3, RES_EVENT_NO, NULL,
      &rs232_devfile[2], set_devfile, (void *)2 },
    { "RsDevice4", ARCHDEP_RS232_DEV4, RES_EVENT_NO, NULL,
      &rs232_devfile[3], set_devfile, (void *)3 },
    RESOURCE_STRING_LIST_END
};

#if RS232_NUM_DEVICES != 4
# error Please fix the count of resources_string[] and cmdline_options[]!
#endif

int rs232drv_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return rs232_resources_init();
}

void rs232drv_resources_shutdown(void)
{
    int i;
    for (i = 0; i < RS232_NUM_DEVICES; i++) {
        lib_free(rs232_devfile[i]);
    }

    rs232_resources_shutdown();
}

static const cmdline_option_t cmdline_options[] = {
    { "-rsdev1", SET_RESOURCE, 1,
      NULL, NULL, "RsDevice1", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_RS232_1_NAME,
      NULL, NULL },
    { "-rsdev2", SET_RESOURCE, 1,
      NULL, NULL, "RsDevice2", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_RS232_2_NAME,
      NULL, NULL },
    { "-rsdev3", SET_RESOURCE, 1,
      NULL, NULL, "RsDevice3", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_RS232_3_NAME,
      NULL, NULL },
    { "-rsdev4", SET_RESOURCE, 1,
      NULL, NULL, "RsDevice4", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_RS232_4_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

int rs232drv_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return rs232_cmdline_options_init();
}

void rs232drv_init(void)
{
    rs232_init();
}

void rs232drv_reset(void)
{
    rs232_reset();
}

int rs232drv_open(int device)
{
    return rs232_open(device);
}

void rs232drv_close(int fd)
{
    rs232_close(fd);
}

int rs232drv_putc(int fd, BYTE b)
{
    return rs232_putc(fd, b);
}

int rs232drv_getc(int fd, BYTE *b)
{
    return rs232_getc(fd, b);
}

int rs232drv_set_status(int fd, enum rs232handshake_out status)
{
    return rs232_set_status(fd, status);
}

enum rs232handshake_in rs232drv_get_status(int fd)
{
    return rs232_get_status(fd);
}

void rs232drv_set_bps(int fd, unsigned int bps)
{
    rs232_set_bps(fd, bps);
}

#else

void rs232drv_init(void)
{
}

void rs232drv_reset(void)
{
}

int rs232drv_open(int device)
{
    return -1;
}

void rs232drv_close(int fd)
{
}

int rs232drv_putc(int fd, BYTE b)
{
    return -1;
}

int rs232drv_getc(int fd, BYTE *b)
{
    return -1;
}

int rs232drv_set_status(int fd, enum rs232handshake_out status)
{
    return -1;
}

enum rs232handshake_in rs232drv_get_status(int fd)
{
    return 0;
}

void rs232drv_set_bps(int fd, unsigned int bps)
{
}

int rs232drv_resources_init(void)
{
    return 0;
}

void rs232drv_resources_shutdown(void)
{
}

int rs232drv_cmdline_options_init(void)
{
    return 0;
}

#endif

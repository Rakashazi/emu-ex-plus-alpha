/*
 * opencbmlib.h - Interface to access the opencbm library.
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

#ifndef VICE_OPENCBMLIB_H
#define VICE_OPENCBMLIB_H

#include "vice.h"
#include "opencbm.h"

typedef int (CBMAPIDECL *cbm_driver_open_t)(CBM_FILE *f, int port);
typedef void (CBMAPIDECL *cbm_driver_close_t)(CBM_FILE f);
typedef const char *(CBMAPIDECL *cbm_get_driver_name_t)(int port);
typedef int (CBMAPIDECL *cbm_listen_t)(CBM_FILE f, unsigned char dev,
                                       unsigned char secadr);
typedef int (CBMAPIDECL *cbm_talk_t)(CBM_FILE f, unsigned char dev,
                                     unsigned char secadr);
typedef int (CBMAPIDECL *cbm_open_t)(CBM_FILE f, unsigned char dev, unsigned char secadr,
                                     const void *fname, size_t len);
typedef int (CBMAPIDECL *cbm_close_t)(CBM_FILE f, unsigned char dev,
                                      unsigned char secadr);
typedef int (CBMAPIDECL *cbm_raw_read_t)(CBM_FILE f, void *buf, size_t size);
typedef int (CBMAPIDECL *cbm_raw_write_t)(CBM_FILE f, const void *buf,
                                          size_t size);
typedef int (CBMAPIDECL *cbm_unlisten_t)(CBM_FILE f);
typedef int (CBMAPIDECL *cbm_untalk_t)(CBM_FILE f);
typedef int (CBMAPIDECL *cbm_get_eoi_t)(CBM_FILE f);
typedef int (CBMAPIDECL *cbm_reset_t)(CBM_FILE f);

struct opencbmlib_s {
    cbm_driver_open_t p_cbm_driver_open;
    cbm_driver_close_t p_cbm_driver_close;
    cbm_get_driver_name_t p_cbm_get_driver_name;
    cbm_listen_t p_cbm_listen;
    cbm_talk_t p_cbm_talk;
    cbm_open_t p_cbm_open;
    cbm_close_t p_cbm_close;
    cbm_raw_read_t p_cbm_raw_read;
    cbm_raw_write_t p_cbm_raw_write;
    cbm_unlisten_t p_cbm_unlisten;
    cbm_untalk_t p_cbm_untalk;
    cbm_get_eoi_t p_cbm_get_eoi;
    cbm_reset_t p_cbm_reset;
};

typedef struct opencbmlib_s opencbmlib_t;

extern int opencbmlib_open(opencbmlib_t *opencbmlib);
extern void opencbmlib_close(void);
extern unsigned int opencbmlib_is_available(void);

#endif


/*
 * snapshot.h - Implementation of machine snapshot files.
 *
 * Written by
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

#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "types.h"

#define SNAPSHOT_MACHINE_NAME_LEN       16
#define SNAPSHOT_MODULE_NAME_LEN        16

typedef struct snapshot_module_s snapshot_module_t;
typedef struct snapshot_s snapshot_t;

extern int snapshot_module_write_byte(snapshot_module_t *m, BYTE data);
extern int snapshot_module_write_word(snapshot_module_t *m, WORD data);
extern int snapshot_module_write_dword(snapshot_module_t *m, DWORD data);
extern int snapshot_module_write_double(snapshot_module_t *m, double db);
extern int snapshot_module_write_padded_string(snapshot_module_t *m,
                                               const char *s, BYTE pad_char,
                                               int len);
extern int snapshot_module_write_byte_array(snapshot_module_t *m, const BYTE *data,
                                            unsigned int num);
extern int snapshot_module_write_word_array(snapshot_module_t *m, const WORD *data,
                                            unsigned int num);
extern int snapshot_module_write_dword_array(snapshot_module_t *m, const DWORD *data,
                                             unsigned int num);
extern int snapshot_module_write_string(snapshot_module_t *m, const char *s);

extern int snapshot_module_read_byte(snapshot_module_t *m, BYTE *b_return);
extern int snapshot_module_read_word(snapshot_module_t *m, WORD *w_return);
extern int snapshot_module_read_dword(snapshot_module_t *m, DWORD *dw_return);
extern int snapshot_module_read_double(snapshot_module_t *m, double *db_return);
extern int snapshot_module_read_byte_array(snapshot_module_t *m,
                                           BYTE *b_return, unsigned int num);
extern int snapshot_module_read_word_array(snapshot_module_t *m,
                                           WORD *w_return, unsigned int num);
extern int snapshot_module_read_dword_array(snapshot_module_t *m,
                                            DWORD *dw_return,
                                            unsigned int num);
extern int snapshot_module_read_string(snapshot_module_t *m, char **s);
extern int snapshot_module_read_byte_into_int(snapshot_module_t *m,
                                              int *value_return);
extern int snapshot_module_read_word_into_int(snapshot_module_t *m,
                                              int *value_return);
extern int snapshot_module_read_dword_into_ulong(snapshot_module_t *m,
                                                 unsigned long *value_return);
extern int snapshot_module_read_dword_into_int(snapshot_module_t *m,
                                               int *value_return);
extern int snapshot_module_read_dword_into_uint(snapshot_module_t *m,
                                                unsigned int *value_return);

#define SMW_B       snapshot_module_write_byte
#define SMW_W       snapshot_module_write_word
#define SMW_DW      snapshot_module_write_dword
#define SMW_DB      snapshot_module_write_double
#define SMW_PSTR    snapshot_module_write_padded_string
#define SMW_BA      snapshot_module_write_byte_array
#define SMW_WA      snapshot_module_write_word_array
#define SMW_DWA     snapshot_module_write_dword_array
#define SMW_STR     snapshot_module_write_string
#define SMR_B       snapshot_module_read_byte
#define SMR_W       snapshot_module_read_word
#define SMR_DW      snapshot_module_read_dword
#define SMR_DB      snapshot_module_read_double
#define SMR_BA      snapshot_module_read_byte_array
#define SMR_WA      snapshot_module_read_word_array
#define SMR_DWA     snapshot_module_read_dword_array
#define SMR_STR     snapshot_module_read_string
#define SMR_B_INT   snapshot_module_read_byte_into_int
#define SMR_W_INT   snapshot_module_read_word_into_int
#define SMR_DW_UL   snapshot_module_read_dword_into_ulong
#define SMR_DW_INT  snapshot_module_read_dword_into_int
#define SMR_DW_UINT snapshot_module_read_dword_into_uint

extern snapshot_module_t *snapshot_module_create(snapshot_t *s,
                                                 const char *name,
                                                 BYTE major_version,
                                                 BYTE minor_version);
extern snapshot_module_t *snapshot_module_open(snapshot_t *s,
                                               const char *name,
                                               BYTE *major_version_return,
                                               BYTE *minor_version_return);
extern int snapshot_module_close(snapshot_module_t *m);

extern snapshot_t *snapshot_create(const char *filename,
                                   BYTE major_version, BYTE minor_version,
                                   const char *snapshot_machine_name);
extern snapshot_t *snapshot_open(const char *filename,
                                 BYTE *major_version_return,
                                 BYTE *minor_version_return,
                                 const char *snapshot_machine_name);
extern int snapshot_close(snapshot_t *s);

#endif

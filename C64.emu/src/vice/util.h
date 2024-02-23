/*
 * util.h - Miscellaneous utility functions.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_UTIL_H
#define VICE_UTIL_H

#include "vice.h"

#include <stdarg.h>
#include <stdio.h>

#include "types.h"

#define UTIL_FILE_LOAD_RAW          0
#define UTIL_FILE_LOAD_SKIP_ADDRESS 1

#define util_arraysize(_x) (sizeof(_x) / sizeof(_x[0]))

char *util_concat(const char *s1, ...);
char *util_strjoin(const char **list, const char *sep);
char **util_strsplit(const char *string, const char *delimiter, int max_tokens);
void util_addline(char **list, const char *line);
void util_addline_free(char **list, char *line);
uint8_t *util_bufcat(uint8_t *buf, int *buf_size, size_t *max_buf_size, const uint8_t *src, int src_size);
void util_remove_spaces(char *s);
void util_add_extension(char **name, const char *extension);
char *util_add_extension_const(const char *filename, const char *extension);
void util_add_extension_maxpath(char *name, const char *extension, unsigned int maxpath);
char *util_get_extension(const char *filename);

int util_file_exists(const char *name);
int util_file_load(const char *name, uint8_t *dest, size_t size, unsigned int load_flag);
int util_file_load_string(FILE *fd, char **dest);
int util_file_save(const char *name, uint8_t *src, int size);

int util_get_line(char *buf, int bufsize, FILE *f);
void util_fname_split(const char *path, char **directory_return, char **name_return);

char *util_subst(const char *s, const char *string, const char *replacement);
int util_string_set(char **str, const char *new_value);
int util_check_null_string(const char *string);

int util_check_filename_access(const char *filename);

int util_fpread(FILE *fd, void *buf, size_t num, long offset);
int util_fpwrite(FILE *fd, const void *buf, size_t num, long offset);

void util_dword_to_be_buf(uint8_t *buf, uint32_t data);
void util_dword_to_le_buf(uint8_t *buf, uint32_t data);
uint32_t util_le_buf_to_dword(uint8_t *buf);
uint32_t util_be_buf_to_dword(uint8_t *buf);

void util_int_to_be_buf4(uint8_t *buf, int data);
void util_int_to_le_buf4(uint8_t *buf, int data);
int util_le_buf4_to_int(uint8_t *buf);
int util_be_buf4_to_int(uint8_t *buf);

void util_word_to_be_buf(uint8_t *buf, uint16_t data);
void util_word_to_le_buf(uint8_t *buf, uint16_t data);
uint16_t util_le_buf_to_word(uint8_t *buf);
uint16_t util_be_buf_to_word(uint8_t *buf);

char *util_find_prev_line(const char *text, const char *pos);
char *util_find_next_line(const char *pos);

char util_tolower(char c);
char util_toupper(char c);
const char *util_skip_whitespace(const char *s);
const char *util_skip_whitespace_trailing(const char *s);

char *util_gen_hex_address_list(int start, int stop, int step);

char *util_join_paths(const char *path, ...);

int util_strcasecmp(const char *s1, const char *s2);
int util_strncasecmp(const char *s1, const char *s2, size_t n);


#endif

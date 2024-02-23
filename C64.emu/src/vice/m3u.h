/** \file   m3u.h
 * \brief   Extended M3U playlist handling - header
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

#ifndef VICE_M3U_H
#define VICE_M3U_H

#include <stddef.h>
#include <stdbool.h>


/* m3u directives */
typedef enum {
    M3U_ERROR = -1, /**< for error reporting, not part of m3u */
    M3U_EXTM3U,     /**< extended M3U header "\#EXTM3U" */
    M3U_EXTINF,     /**< track info, key=value pairs */
    M3U_PLAYLIST,   /**< playlist title */
    M3U_EXTALB,     /**< album title */
    M3U_EXTART      /**< album artist */
} m3u_ext_id_t;



bool m3u_open(const char *path,
              bool (*entry_callback)(const char *arg, size_t len),
              bool (*directive_callback)(m3u_ext_id_t id, const char *arg, size_t len));
bool m3u_parse(void);
void m3u_close(void);
bool m3u_create(const char *path);
bool m3u_set_playlist_title(const char *title);
bool m3u_append_entry(const char *entry);
bool m3u_append_comment(const char *comment);
bool m3u_append_newline(void);
long m3u_linenum(void);
const char *m3u_directive_str(m3u_ext_id_t id);

#endif

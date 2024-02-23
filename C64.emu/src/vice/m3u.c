/** \file   m3u.c
 * \brief   Extended M3U playlist handling
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Simple API for reading and writing extended M3U playlist files, tailored
 * towards use in VSID.
 *
 * There is no formal specification of the file format, so information was
 * used from various places on the internet, among which:
 * https://en.wikipedia.org/wiki/M3U
 *
 * Only a few directives of the extended m3u format are supported. Inline
 * comments are **not** supported (can't find any reference about whether or
 * not they should be).
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "m3u.h"

/** \brief  Array length helper
 *
 * \param   arr array
 */
#define ARRAY_LEN(arr)  (sizeof arr / sizeof arr[0])

/** \brief  Initial size of buffer for reading the playlist */
#define INITIAL_BUFSIZE 80

/** \brief  Token indicating comment or directive */
#define M3U_COMMENT '#'

/** \brief  Extended M3U header
 *
 * This header must be the first line in an m3u 1.1 file.
 *
 * \note    A newline is added automagically when calling m3u_create()
 */
#define M3U_HEADER  "#EXTM3U"


/** \brief  M3U directive data */
typedef struct m3u_ext_s {
    m3u_ext_id_t id;    /**< numeric ID */
    const char *text;   /**< text in upper case */
    const char *desc;   /**< short description */
} m3u_ext_t;


/** \brief  Path to playlist file */
static char *playlist_path = NULL;

/** \brief  File pointer for the parser */
static FILE *playlist_fp = NULL;

/** \brief  Current line number in the playlist */
static long playlist_linenum = 0;

/** \brief  Title has been set via m3u_set_playlist_title()
 *
 * Avoid writing title multiple times.
 */
static bool have_playlist_title = false;

/** \brief  Buffer used for reading lines from the playlist
 *
 * Dynamically allocated and resized when required, freed with m3u_close()
 */
static char *playlist_buf = NULL;

/** \brief  Size of the buffer */
static size_t playlist_bufsize = 0;

/** \brief  Callback for normal entries
 *
 * The first argument is the entry as a string, the second the length of the
 * entry excluding the terminating nul character.
 */
static bool (*entry_cb)(const char *, size_t) = NULL;

/** \brief  Callback for directives
 *
 * The first argument the directive type, the second argument is the data
 * following the entry and the third argument is the length of the data
 * excluding the terminating nul character.
 */
static bool (*directive_cb)(int, const char *, size_t) = NULL;

/** \brief  Current directory for relative entries
 *
 * M3UEXT allows using directory-only entries and relative paths, so we keep
 * track of that here.
 *
 * Initially the directory is set by m3u_open() to the directory of the playlist
 * file being parsed. When a non-file entry is found the contents are either
 * appended to the current dir (if relative) or the current directory is set
 * to the entry (if absolute).
 */
static char *playlist_current_dir = NULL;


/** \brief  List of extended m3u directives */
static const m3u_ext_t extensions[] = {
    { M3U_EXTM3U,   "EXTM3U",   "file header" },
    { M3U_EXTINF,   "EXTINF",   "track information" },
    { M3U_PLAYLIST, "PLAYLIST", "playlist title" },
    { M3U_EXTALB,   "EXTALB",   "album title" },
    { M3U_EXTART,   "EXTART",   "album artist" },
};


/** \brief  Parse string for m3u directive
 *
 * Parse string \a s for a valid directive (excluding the starting '#') and
 * return the directive ID, optionally setting \a endptr to the first
 * non-directive character (whitspace, colon or eol).
 *
 * \param[in]   s       string to parse
 * \param[out]  endptr  pointer to character after the directive in \a s
 *                      (optional, pass `NULL` to ignore)
 *
 * \return  directive ID or #M3U_ERROR (-1) when no valid directive found
 */
static m3u_ext_id_t get_directive_id(const char *s, const char **endptr)
{
    const char *p = s;
    size_t i = 0;

    /* locate either eol, whitespace or colon */
    while (*p != '\0' && *p != ':' && !isspace((unsigned char)*p)) {
        p++;
    }

    /* check valid directives */
    for (i = 0; i < ARRAY_LEN(extensions); i++) {
        if (util_strncasecmp(extensions[i].text, s, strlen(extensions[i].text)) == 0) {
            if (*p == ':') {
                p++;    /* skip past colon */
            }
            if (endptr != NULL) {
                *endptr = p;
            }
            return extensions[i].id;
        }
    }
    if (endptr != NULL) {
        *endptr = NULL;
    }
    return M3U_ERROR;
}

/** \brief  Strip trailing whitespace from buffer
 *
 * Strip trailing whitespace starting at \a pos - 1 in the buffer.
 *
 * \return  position in the string of the terminating nul character
 */
static ssize_t strip_trailing(ssize_t pos)
{
    pos--;
    while (pos >= 0 && isspace((unsigned char)(playlist_buf[pos]))) {
        pos--;
    }
    playlist_buf[pos + 1] = '\0';
    return pos + 1;
}

/** \brief  Free `playlist_current_dir` and set to `NULL`
 */
static void free_current_dir(void)
{
    if (playlist_current_dir != NULL) {
        lib_free(playlist_current_dir);
        playlist_current_dir = NULL;
    }
}

/** \brief  Join current directory with \a path
 *
 * \param[in]   path    pathname
 *
 * \return  heap-allocated string, free with lib_free() after use
 */
static char *prepend_current_dir(const char *path)
{
    if (playlist_current_dir != NULL) {
        return util_join_paths(playlist_current_dir, path, NULL);
    } else {
        return lib_strdup(path);
    }
}

/** \brief  Update current directory for relative entries
 *
 * \param[in]   path    new directory (may be relative)
 */
static void update_current_dir(const char *path)
{
#if 0
    printf("m3u: %s() called with %s\n", __func__, path);
#endif
    if (archdep_path_is_relative(path)) {
        /* append path to current directory */
        char *new_dir;

        new_dir = util_join_paths(playlist_current_dir, path, NULL);
        free_current_dir();
        playlist_current_dir = new_dir;

    } else {
        free_current_dir();
        playlist_current_dir = lib_strdup(path);
    }
#if 0
    printf("m3u: new working directory: %s\n", playlist_current_dir);
#endif
}

/** \brief  Read a line of text from the playlist
 *
 * \param[out]  len length of line read (excluding trailing whitespace)
 *
 * \note    trailing whitespace is stripped
 *
 * \return  `false` on EOF
 */
static bool read_line(size_t *len)
{
    ssize_t pos = 0;

    while (true) {
        int ch = fgetc(playlist_fp);

        if (pos == playlist_bufsize) {
            playlist_bufsize *= 2;
            playlist_buf = lib_realloc(playlist_buf, playlist_bufsize);
        }

        if (ch == EOF) {
            /* EOF, terminate string in buffer and exit */
            playlist_buf[pos] = '\0';
            *len = (size_t)strip_trailing(pos);
            return false;
        }

        if (ch == 0x0a) {
            /* LF */
            playlist_buf[pos] = '\0';
            if (pos > 0 && playlist_buf[pos - 1] == 0x0d) {
                /* strip CR */
                playlist_buf[pos - 1] = '\0';
                pos--;
            }

            /* strip trailing whitespace */
            *len = (size_t)strip_trailing(pos);
            return true;
        }

        playlist_buf[pos++] = ch;
    }
}


/** \brief  Open m3u file for parsing
 *
 * Try to open \a path for reading and set up the parser.
 *
 * The \a entry_callback argument is required and will be called by m3u_parse()
 * whenever a normal entry is encountered in \a path. Its first argument is
 * the entry text and the second argument is the lenght of the entry excluding
 * the terminating nul character.
 *
 * The \a directive_callback argument is optional and can be used to handle
 * extended M3U directives. The first argument is the directive ID (\see
 * #m3u_ext_id_t), the second argument is the text following the directive
 * and the third argument is the length of the text excluding the terminating
 * nul character.
 *
 * To stop the parser the callbacks must return `false`.
 *
 * \param[in]   path                path to m3u file
 * \param[in]   entry_callback      callback for normal entries
 * \param[in]   directive_callback  callback for extended m3u directives
 *                                  (optional, pass `NULL` to ignore)
 *
 * \return  `true` on success
 */
bool m3u_open(const char *path,
              bool (*entry_callback)(const char *arg, size_t len),
              bool (*directive_callback)(m3u_ext_id_t id, const char *arg, size_t len))
{
    if (path == NULL || *path == '\0') {
        log_error(LOG_ERR, "m3u: path argument cannot be NULL or empty.");
        return false;
    }
    if (entry_callback == NULL) {
        log_error(LOG_ERR, "m3u: entry_callback entry cannot be NULL.");
        return false;
    }
    playlist_path = lib_strdup(path);
    entry_cb = entry_callback;
    directive_cb = directive_callback;
    playlist_linenum = 1;

    /* try to open file for reading */
    playlist_fp = fopen(path, "rb");
    if (playlist_fp == NULL) {
        log_error(LOG_ERR,
                  "m3u: failed to open %s for reading: errno %d: %s.",
                  path, errno, strerror(errno));
        return false;
    }

    /* use directory of playlist file as the "working dir" for relative
     * paths in the file */
    free_current_dir();
    util_fname_split(path, &playlist_current_dir, NULL);

    /* allocate buffer for reading file content */
    playlist_buf = lib_malloc(INITIAL_BUFSIZE);
    playlist_bufsize = INITIAL_BUFSIZE;
    return true;
}


/** \brief  Check if we have an open file
 *
 * Test if we have an open file and report an error if not.
 *
 * \return  `true` when a file is open
 */
static bool have_output_file(void)
{
    if (playlist_fp == NULL) {
        log_error(LOG_ERR, "m3u: no output file.");
        return false;
    }
    return true;
}


/** \brief  Append string to current playlist file
 *
 * \param[in]   s   string to write
 *
 * \note    Obviously requires calling m3u_create() first
 *
 * \return  `true` on success
 */
static bool m3u_append_string(const char *s)
{
    if (!have_output_file()) {
        return false;
    }
    if (fputs(s, playlist_fp) < 0) {
        log_error(LOG_ERR,
                  "m3u: failed to write to %s: errno %d: %s.",
                  playlist_path, errno, strerror(errno));
        return false;
    }
    return m3u_append_newline();
}


/** \brief  Create new M3U playlist file
 *
 * Open \a path for writing and write the M3U header.
 *
 * \param[in]   path    filename and path
 *
 * \return  `true` on success
 */
bool m3u_create(const char *path)
{
    /* open file for writing */
    playlist_fp = fopen(path, "w");
    if (playlist_fp == NULL) {
        log_error(LOG_ERR,
                  "m3u: failed to open %s for writing: errno %d: %s.",
                  path, errno, strerror(errno));
        return false;
    }

    /* copy path */
    if (playlist_path != NULL) {
        lib_free(playlist_path);
    }
    playlist_path = lib_strdup(path);

    /* write header and initialize line number */
    if (!m3u_append_string(M3U_HEADER)) {
        return false;
    }
    playlist_linenum = 2;
    have_playlist_title = false;
    return true;
}


/** \brief  Set playlist title
 *
 * \param[in]   title   playlist title
 *
 * \return  `true` on success
 *
 * \note    Normally one would call this after m3u_create() and before adding
 *          any entries.
 */
bool m3u_set_playlist_title(const char *title)
{
    if (!have_output_file()) {
        return false;
    }
    if (have_playlist_title) {
        log_error(LOG_ERR, "m3u: playlist title has already been set.");
        return false;
    }
    if (fprintf(playlist_fp, "#PLAYLIST: %s\n", title) < 0) {
        log_error(LOG_ERR,
                  "m3u: failed to write playlist title: errno %d: %s.",
                  errno, strerror(errno));
        return false;
    }
    have_playlist_title = true;
    return true;
}


/** \brief  Append normal entry to playlist file
 *
 * Append path to SID file to playlist file.
 *
 * \param[in]   entry   path to SID file
 *
 * \return  `true` on success
 *
 * \todo    Support relative paths
 */
bool m3u_append_entry(const char *entry)
{
    if (!have_output_file()) {
        return false;
    }
    return m3u_append_string(util_skip_whitespace(entry));
}


/** \brief  Append comment to playlist file
 *
 * \param[in]   comment comment text
 *
 * \note    The caller should not use the '\#' comment token, the function will
 *          add it.
 *
 * \return  `true` on success
 */
bool m3u_append_comment(const char *comment)
{
    if (!have_output_file()) {
        return false;
    }
    /* strip comment token, we add that ourselves */
    if (*comment == M3U_COMMENT) {
        comment++;
    }
    if (fprintf(playlist_fp, "# %s\n", comment) < 0) {
        log_error(LOG_ERR,
                  "m3u: failed to write comment: errno %d: %s.",
                  errno, strerror(errno));
        return false;
    }
    return true;
}


/** \brief  Append newline to playlist file
 *
 * \return  `true` on success
 */
bool m3u_append_newline(void)
{
    if (fputc('\n', playlist_fp) < 0) {
        log_error(LOG_ERR,
                  "m3u: failed to write to %s: errno %d: %s.",
                  playlist_path, errno, strerror(errno));
        return false;
    }
    return true;
}


/** \brief  Close m3u file and clean up resources
 *
 * Cleans up resources and resets internal state for subsequent reading or
 * writing of m3u files.
 */
void m3u_close(void)
{
    if (playlist_fp != NULL) {
        fclose(playlist_fp);
        playlist_fp = NULL;
    }
    if (playlist_buf != NULL) {
        lib_free(playlist_buf);
        playlist_buf = NULL;
    }
    if (playlist_path != NULL) {
        lib_free(playlist_path);
        playlist_path = NULL;
    }
    free_current_dir();
    playlist_linenum = 0;
    have_playlist_title = false;
}


/** \brief  Parse the playlist
 *
 * Parse playlist file triggering the callbacks registered with m3u_open().
 *
 * \return  `true` on success
 */
bool m3u_parse(void)
{
    size_t len;

    if (playlist_fp == NULL) {
        log_error(LOG_ERR, "m3u: no input file.");
        return false;
    }

    while (read_line(&len)) {
        const char *s;
#if 0
        printf("line[%3ld](%3zu) '%s'\n", playlist_linenum, len, playlist_buf);
#endif
        s = util_skip_whitespace(playlist_buf);
        if (*s == M3U_COMMENT) {
            const char *endptr;
            m3u_ext_id_t id;

            /* try to find valid directive */
            id = get_directive_id(s + 1, &endptr);
            if (id >= 0) {
                if (!directive_cb(id, endptr, 0)) {
                    /* error */
                    return false;
                }
            }
        } else if (*s != '\0') {
            /* entry, check dir or file */
            unsigned int isdir = 0;
            char *temp;

            if (archdep_path_is_relative(s)) {
                temp = prepend_current_dir(s);
            } else {
                temp = lib_strdup(s);
            }

            /* entry, check if directory or file */
            if (archdep_stat(temp, NULL, &isdir) < 0) {
                /* error, report but ignore for now */
                log_error(LOG_ERR, "m3u: stat() call failed on %s", temp);
                lib_free(temp);
                continue;
            }
            if (isdir) {
                /* handle directory */
                update_current_dir(temp);
            } else {
                /* it's a file entry: */
                char normalized[ARCHDEP_PATH_MAX];

                /* normalize first, otherwise SLDB/STIL lookups might fail */
                archdep_real_path(temp, normalized);
                if (!entry_cb(normalized, strlen(normalized))) {
                    /* some error, stop */
                    lib_free(temp);
                    return false;
                }
            }
            lib_free(temp);
        }

        playlist_linenum++;
    }
    return true;
}


/** \brief  Get current line number of playlist being processed
 *
 * \return  line number
 */
long m3u_linenum(void)
{
    return playlist_linenum;
}


/** \brief  Get directive string for ID
 *
 * \param[in]   id  directive ID
 *
 * \see #m3u_ext_id_t
 */
const char *m3u_directive_str(m3u_ext_id_t id)
{
    if (id < 0 || id >= (m3u_ext_id_t)ARRAY_LEN(extensions)) {
        return NULL;
    }
    return extensions[id].text;
}

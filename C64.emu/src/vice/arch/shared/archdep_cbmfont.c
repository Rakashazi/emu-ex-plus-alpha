/** \file   archdep_cbmfont.c
 * \brief   CBM font handling
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
#include <stddef.h>
#include <stdbool.h>
#include "archdep_defs.h"

#include "archdep_boot_path.h"
#include "archdep_stat.h"
#include "lib.h"
#include "log.h"
#include "sysfile.h"

#include "archdep_cbmfont.h"

#ifdef USE_GTK3UI

/** \brief  Filename of the TrueType CBM font used for directory display
 */
#define VICE_CBM_FONT_TTF "C64_Pro_Mono-STYLE.ttf"


/** \fn  int archdep_register_cbmfont(void)
 * \brief    Try to register the CBM font with the OS
 *
 * This tries to use fontconfig on Unix, GDI on windows and CoreText on macOS.
 *
 * \return    bool as int
 */

# ifdef MACOS_COMPILE

#  include <CoreText/CTFontManager.h>

int archdep_register_cbmfont(void)
{
    char *fontPath;
    CFStringRef fontPathStringRef;
    CFURLRef fontUrl;
    CFArrayRef fontUrls;
    CFArrayRef errors;

    if (sysfile_locate(VICE_CBM_FONT_TTF, "common", &fontPath) < 0) {
        log_error(LOG_ERR, "failed to find resource data '%s'.",
                VICE_CBM_FONT_TTF);
        return 0;
    }

    fontPathStringRef = CFStringCreateWithCString(NULL, fontPath, kCFStringEncodingUTF8);
    fontUrl = CFURLCreateWithFileSystemPath(NULL, fontPathStringRef, kCFURLPOSIXPathStyle, false);
    fontUrls = CFArrayCreate(NULL, (const void **)&fontUrl, 1, NULL);

    CFRelease(fontPathStringRef);

    if(!CTFontManagerRegisterFontsForURLs(fontUrls, kCTFontManagerScopeProcess, &errors))
    {
        log_error(LOG_ERR, "Failed to register font for file: %s", fontPath);
        CFRelease(fontUrls);
        CFRelease(fontUrl);
        lib_free(fontPath);
        return 0;
    }

    CFRelease(fontUrls);
    CFRelease(fontUrl);
    lib_free(fontPath);
    return 1;
}

# elif defined(UNIX_COMPILE)

#  ifdef HAVE_FONTCONFIG

#   include <fontconfig/fontconfig.h>

int archdep_register_cbmfont(void)
{
    FcConfig *fc_config;
    int result;
    char *path;

    if (!FcInit()) {
        return 0;
    }

    fc_config = FcConfigGetCurrent();
    if (sysfile_locate(VICE_CBM_FONT_TTF, "common", &path) < 0) {
        log_error(LOG_ERR, "failed to find resource data '%s'.",
                VICE_CBM_FONT_TTF);
        return 0;
    }
    result = FcConfigAppFontAddFile(fc_config, (FcChar8 *)path) ? 1 : 0;
    lib_free(path);
    return result;
}

#  else     /* HAVE_FONTCONFIG */

int archdep_register_cbmfont(void)
{
    log_error(LOG_ERR, "no fontconfig support, sorry.");
    return 0;
}

#  endif

# else   /* UNIX_COMPILE */


/*
 * Windows part of the API
 */

#  ifdef WINDOWS_COMPILE

/** Flag indictating the font was successfully registered and thus must be
 *  unregistered.
 */
static bool font_registered = false;

/* Make sure AddFontResourceEx prototype is used in wingdi.h */
#   ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0500
#   else
#    if (_WIN32_WINNT < 0x0500)
#     undef _WIN32_WINNT
#     define _WIN32_WINNT 0x0500
#    endif
#   endif

#   include <windows.h>
#   include <pango/pango.h>

int archdep_register_cbmfont(void)
{
    char *path;
    int result;

    log_message(LOG_DEFAULT,
                "%s(): Registering CBM font using Pango %s",
                __func__, pango_version_string());

    if (sysfile_locate(VICE_CBM_FONT_TTF, "common", &path) < 0) {
        log_error(LOG_ERR, "failed to find resource data '%s'.",
                VICE_CBM_FONT_TTF);
        return 0;
    }

    /* Work around the fact that Pango, starting with 1.50.12, has switched to
       (only) using DirectWrite for enumarating fonts, and DirectWrite doesn't
       find fonts added with AddFontResourceEx().
       see https://gitlab.gnome.org/GNOME/pango/-/issues/720

       1.50.12 is actually broken in this respect, but the GDI way of font
       enumeration (with AddFontResource[A|W]) will be added back in 1.50.13.
     */
    if (pango_version() < PANGO_VERSION_ENCODE(1, 50, 12)) {
        log_message(LOG_DEFAULT,
                    "%s(): Using AddFontResourceEx()",
                    __func__);
        result = AddFontResourceEx(path, FR_PRIVATE, 0);
    } else {
        /* non-private version, if VICE crashes the font will remain on the
           host system until the system is rebooted */
        log_message(LOG_DEFAULT,
                    "%s(): Using AddFontResourceA()",
                    __func__);
        result = AddFontResourceA(path);
    }
    lib_free(path);
    if (result > 0) {
        font_registered = true;
        log_message(LOG_DEFAULT,
                    "%s(): According to Windows, the CBM font was succesfully"
                    " registered.",
                    __func__);
        return 1;
    }
    log_warning(LOG_DEFAULT,
                "%s(): According to Windows, registering the font failed",
                __func__);
    return 0;
}

#  else

int archdep_register_cbmfont(void)
{
    /* OS not supported */
   return 0;
}

#  endif

# endif

/** \brief  Unregister the CBM font
 *
 * Unregisters the font on Windows, NOP on any other arch.
 */
void archdep_unregister_cbmfont(void)
{
# ifdef WINDOWS_COMPILE
    if (font_registered) {
        char *path;

        if (sysfile_locate(VICE_CBM_FONT_TTF, "common", &path) < 0) {
            log_error(LOG_ERR, "failed to find resource data '%s'.",
                    VICE_CBM_FONT_TTF);
            return;
        }

        if (pango_version() < PANGO_VERSION_ENCODE(1, 50, 12)) {
            log_message(LOG_DEFAULT,
                        "%s(): Unregistering CBM font with RemoveFontResourceExA()",
                        __func__);
            RemoveFontResourceExA(path, FR_PRIVATE, 0);
        } else {
            /* If we unregister the font with RemoveFontResourceA() the font
             * will somehow remain unregistered and registering the font with
             * AddFontResourceA() on subsequent runs will NOT register it again,
             * although the function reports success.
             * So if we call this RemoveFontResourceA() the user will either
             * have to reboot Windows every time before starting VICE, or
             * manually install the font.
             * Although rebooting a lot is perfectly normal in daily Windows use,
             * I find to be less enjoyable ;)
             * So we leave the font around for the session, after a reboot (heh)
             * the font will be gone again and VICE will register it again.
             */
            log_warning(LOG_DEFAULT,
                        "%s(): Pango version >= 1.5.12: skipping unregistering"
                        " font with RemoveFontResourceA()",
                        __func__);
#if 0
            RemoveFontResourceA(path);
#endif
        }
        lib_free(path);
    }
# endif
}
# else  /* !USE_GTK3UI */

/* Non-Gtk UIs */
int archdep_register_cbmfont(void)
{
    return 0;   /* error */
}

void archdep_unregister_cbmfont(void)
{
    /* NOP */
}

#endif  /* !USE_GTK3UI */

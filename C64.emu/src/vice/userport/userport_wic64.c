/*
 * userport_wic64.c - Userport WiC64 wifi interface emulation.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
 *  fixed & complemented by pottendo <pottendo@gmx.net>
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

/* #define DEBUG_WIC64 */

/* - WiC64 (C64/C128)

   PET     VIC20  C64/C128   |  I/O
   --------------------------------
   C                 (PB0)   |  I/O   databits from/to C64
   D                 (PB1)   |  I/O
   E                 (PB2)   |  I/O
   F                 (PB3)   |  I/O
   H                 (PB4)   |  I/O
   J                 (PB5)   |  I/O
   K                 (PB6)   |  I/O
   L                 (PB7)   |  I/O
   READ(1) PA6(2)  8 (PC2)   |  O     C64 triggers PC2 IRQ whenever data is read or write
   CB2     CB2     M (PA2)   |  O     Low=device sends data High=C64 sends data (powerup=high)
   CA1     CB1     B (FLAG2) |  I     device asserts high->low transition when databyte sent to c64 is ready (triggers irq)

   (1) tape #1 read
   (2) also connected to tape "sense" (PLAY, F.FWD or REW pressed)

   enable the device and start https://www.wic64.de/wp-content/uploads/2021/10/wic64start.zip

   for more info see https://www.wic64.de
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "alarm.h"
#include "cmdline.h"
#include "maincpu.h"
#include "resources.h"
#include "joyport.h"
#include "joystick.h"
#include "snapshot.h"
#include "userport.h"
#include "userport_wic64.h"
#include "machine.h"
#include "uiapi.h"
#include "lib.h"
#include "util.h"
#include "charset.h"

#ifdef HAVE_LIBCURL

#define VICEWIC64VERSION PACKAGE"-"VERSION
#define WIC64_VERSION_MAJOR 2
#define WIC64_VERSION_MINOR 0
#define WIC64_VERSION_PATCH 0
#define WIC64_VERSION_DEVEL 0
#define WIC64_SHORT_VERSION "2.0.0"
#define WIC64_VERSION_STRING "2.0.0 ("PACKAGE"; "VERSION")"
#define HTTP_AGENT_REVISED "WiC64/"WIC64_SHORT_VERSION" ("PACKAGE"; "VERSION")"
#define HTTP_AGENT_LEGACY "ESP32HTTPClient"
static char *http_user_agent = HTTP_AGENT_REVISED;

#include "log.h"
static log_t wic64_loghandle = LOG_ERR;
static int userport_wic64_enabled = 0;

/* Some prototypes are needed */
static uint8_t userport_wic64_read_pbx(uint8_t orig);
static void userport_wic64_store_pbx(uint8_t val, int pulse);
static void userport_wic64_store_pa2(uint8_t value);
static int userport_wic64_write_snapshot_module(snapshot_t *s);
static int userport_wic64_read_snapshot_module(snapshot_t *s);
static int userport_wic64_enable(int value);
static void userport_wic64_reset(void);

static int wic64_set_default_server(const char *val, void *p);
static int wic64_set_macaddress(const char *val, void *p);
static int wic64_set_ipaddress(const char *val, void *p);
static int wic64_set_sectoken(const char *val, void *p);
static int wic64_set_timezone(int val, void *param);
static int wic64_set_logenabled(int val, void *param);
static int wic64_set_loglevel(int val, void *param);
static int wic64_set_resetuser(int val, void *param);
static int wic64_set_hexdumplines(int val, void *param);
static int wic64_set_colorize_log(int val, void *param);
static int wic64_cmdl_reset(const char *val, void *param);
static void wic64_log(const char *col, const char *fmt, ...);
static void _wic64_log(const char *col, const int lv, const char *fmt, ...);
static void handshake_flag2(void);
static void send_binary_reply(const uint8_t *reply, size_t len);
static void send_reply_revised(const uint8_t rcode, const char *msg, const uint8_t *payload, size_t len, const char *legacy_msg);
static void userport_wic64_reset(void);
static void wic64_reset_user_helper(void);
static void wic64_set_status(const char *status);
static void prep_wic64_str(void);
static void do_connect(uint8_t *url);
static void cmd_tcp_write(void);
static void cmd_timeout(int arm);

static userport_device_t userport_wic64_device = {
    "Userport WiC64",                     /* device name */
    JOYSTICK_ADAPTER_ID_NONE,             /* this is NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_WIFI,            /* device is a WIFI adapter */
    userport_wic64_enable,                /* enable function */
    userport_wic64_read_pbx,              /* read pb0-pb7 function */
    userport_wic64_store_pbx,             /* store pb0-pb7 function */
    NULL,                                 /* NO read pa2 pin function */
    userport_wic64_store_pa2,             /* store pa2 pin function */
    NULL,                                 /* NO read pa3 pin function */
    NULL,                                 /* NO store pa3 pin function */
    /* HACK: We put a 0 into the struct here, although pin 8 of the userport
       (which is PC2 on the C64) is actually used. This is needed so the device
       can be registered in xvic (where the pin is driven by PA6). */
    0,                                    /* pc pin IS needed */
    NULL,                                 /* NO store sp1 pin function */
    NULL,                                 /* NO read sp1 pin function */
    NULL,                                 /* NO store sp2 pin function */
    NULL,                                 /* NO read sp2 pin function */
    userport_wic64_reset,                 /* reset function */
    NULL,                                 /* NO powerup function */
    userport_wic64_write_snapshot_module, /* snapshot write function */
    userport_wic64_read_snapshot_module   /* snapshot read function */
};

static struct alarm_s *http_get_alarm = NULL;
static struct alarm_s *http_post_alarm = NULL;
static struct alarm_s *http_post_endalarm = NULL;
static struct alarm_s *tcp_get_alarm = NULL;
static struct alarm_s *tcp_send_alarm = NULL;
static struct alarm_s *cmd_timeout_alarm = NULL;
static char sec_token[32];
static int sec_init = 0;
static const char *TOKEN_NAME = "sectokenname";
static char *default_server_hostname = NULL;

static char *wic64_mac_address = NULL; /* c-string std. notation e.g 0a:02:0b:04:05:0c */
static char *wic64_internal_ip = NULL; /* c-string std. notation e.g. 192.168.1.10 */
static unsigned char wic64_external_ip[4] = { 0, 0, 0, 0 }; /* just a dummy, report not implemented to user cmd 0x13 */
static uint8_t wic64_timezone[2] = { 0, 0};
static uint16_t wic64_udp_port = 0;
static uint16_t wic64_tcp_port = 0;
static uint8_t wic64_timeout = 2;
static int force_timeout = 0;
static char *wic64_sec_token = NULL;
static int current_tz = 2;
static int wic64_logenabled = 0;
static int wic64_loglevel = 0;
static int wic64_resetuser = 0;
static int wic64_hexdumplines = 0;
static int wic64_colorize_log = 0;
static char wic64_protocol = 'U'; /* invalid, so we see in trace even the legacy */
static int big_load = 0;
static char wic64_last_status[40]; /* according spec 40 bytes, hold status string. incl. \0 */
static char *post_data = NULL;
static size_t post_data_rcvd;
static size_t post_data_new;
static size_t post_data_size;
static char *post_url = NULL;
static int post_error;
static int cheatlen = 0;

static const resource_string_t wic64_resources[] =
{
    { "WIC64DefaultServer", "http://x.wic64.net/", (resource_event_relevant_t)0, NULL,
      &default_server_hostname, wic64_set_default_server, NULL },
    { "WIC64MACAddress", "DEADBE", (resource_event_relevant_t)0, NULL,
      (char **) &wic64_mac_address, wic64_set_macaddress, NULL },
    { "WIC64IPAddress", "AAAA", (resource_event_relevant_t)0, NULL,
      (char **) &wic64_internal_ip, wic64_set_ipaddress, NULL },
    { "WIC64SecToken", "0123456789ab", (resource_event_relevant_t)0, NULL,
      (char **) &wic64_sec_token, wic64_set_sectoken, NULL },
    RESOURCE_STRING_LIST_END,
};

static const resource_int_t wic64_resources_int[] = {
    { "WIC64Timezone", 2, RES_EVENT_NO, NULL,
      &current_tz, wic64_set_timezone, NULL },
    { "WIC64Logenabled", 0, RES_EVENT_NO, NULL,
      &wic64_logenabled, wic64_set_logenabled, NULL },
    { "WIC64LogLevel", 0, RES_EVENT_NO, NULL,
      &wic64_loglevel, wic64_set_loglevel, NULL },
    { "WIC64Resetuser", 0, RES_EVENT_NO, NULL,
      &wic64_resetuser, wic64_set_resetuser, NULL },
    { "WIC64HexdumpLines", 8, RES_EVENT_NO, NULL,
      &wic64_hexdumplines, wic64_set_hexdumplines, NULL },
    { "WIC64ColorizeLog", 0, RES_EVENT_NO, NULL,
      &wic64_colorize_log, wic64_set_colorize_log, NULL },
    RESOURCE_INT_LIST_END
};

static tzones_t timezones[] = {
    { 0, "Greenwich Mean Time", 0, 0, 1},
    { 1, "Greenwich Mean Time", 0, 0, 1},
    { 2, "European Central Time", 1, 0, 1 },
    { 3, "Eastern European Time", 2, 0, 1 },
    { 4, "Arabic Egypt Time", 2, 0, 1 },
    { 5, "Arabic Egypt Time", 2, 0, 1 },
    { 6, "Arabic Egypt Time", 2, 0, 1 },
    { 7, "Near East Time", 4, 0, 1 },
    { 8, "India Standard Time", 5, 30, 0 },
    { 9, "Dont Know Time", 6, 0, 0 },
    { 10, "Dont Know Time", 7, 0, 0 },
    { 11, "China Standard Time", 8, 0, 0 },
    { 12, "Korean Standard Time", 9, 0, 0 },
    { 13, "Japan Standard Time", 9, 0, 0},
    { 14, "Australia Central Time", 9, 30, 0 },
    { 15, "Australia Eastern Time", 10, 0, 0 },
    { 16, "Dont Know Time", 11, 0,0 },
    { 17, "New Zealand Standart Time", 12, 0, 0 },
    { 18, "Midway Islands Time", -11, 0, 0 },
    { 19, "Hawaii Standard Time", -10, 0, 0 },
    { 20, "Alaska Standard Time", -8, 0, 0 },
    { 21, "Pacific Standard Time", -7, 0, 0 },
    { 22, "Phoenix Standard Time", -7, 0, 0 },
    { 23, "Mountain Standard Time", -6, 0, 0 },
    { 24, "Central Standard Time", -5, 0, 0 },
    { 25, "Eastern Standard Time", -4, 0, 0 },
    { 26, "Indiana Eastern Standard Time", -5, 0, 0 },
    { 27, "Puerto Rico Virg. Island Time", -4, 0, 0 },
    { 28, "Canada Newfoundland Time", -2, -30, 0 },
    { 29, "Dont Know Time", -2, 0, 0 },
    { 30, "Dont Know Time", -1, 0, 0 },
    { 31, "Dont Know Time", 0, 0, 0 },
};

static const cmdline_option_t cmdline_options[] =
{
    { "-wic64server", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "WIC64DefaultServer", NULL,
      "<URL>", "Specify default server URL" },
    { "-wic64timezone", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "WIC64Timezone", NULL,
      "<0..31>", "Specify default timezone index, e.g. 2: European Central Time" },
    { "-wic64trace", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "WIC64Logenabled", (void *)1,
      NULL, "Enable WiC64 tracing" },
    { "+wic64trace", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "WIC64Logenabled", (void *)0,
      NULL, "Disable WiC64 tracing" },
    { "-wic64tracelevel", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "WIC64LogLevel", NULL,
      "<0..3>", "Set WiC64 tracing level (0: off, 1: cmd-level, >2: debug-level), implicitly turns on/off WiC64 tracing" },
    { "-wic64reset", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      wic64_cmdl_reset, (void *)2, NULL, NULL,
      NULL, "Reset WiC64 to factory defaults" },
    { "-wic64hexdumplines", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "WIC64HexdumpLines", NULL,
      "<value>", "Limit WiC64 hexdump lines (0: unlimited)" },
    { "-wic64colorizetrace", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "WIC64colorizelog", (void *)1,
      NULL, "Enable WiC64 colorized trace on terminal" },
    { "+wic64colorizetrace", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "WIC64colorizelog", (void *)0,
      NULL, "Disable WiC64 colorized trace on terminal" },
    CMDLINE_LIST_END
};

/* WiC64 commands as defined */
#define WIC64_CMD_GET_VERSION_STRING  0x00
#define WIC64_CMD_GET_VERSION_NUMBERS 0x26

#define WIC64_CMD_SCAN_WIFI_NETWORKS       0x0c
#define WIC64_CMD_IS_CONFIGURED            0x2f
#define WIC64_CMD_IS_CONNECTED             0x2c
#define WIC64_CMD_CONNECT_WITH_SSID_STRING 0x02
#define WIC64_CMD_CONNECT_WITH_SSID_INDEX  0x0d

#define WIC64_CMD_GET_MAC  0x14
#define WIC64_CMD_GET_SSID 0x10
#define WIC64_CMD_GET_RSSI 0x11
#define WIC64_CMD_GET_IP   0x06

#define WIC64_CMD_HTTP_GET         0x01
#define WIC64_CMD_HTTP_GET_ENCODED 0x0f
#define WIC64_CMD_HTTP_POST_URL    0x28
#define WIC64_CMD_HTTP_POST_DATA   0x2b

#define WIC64_CMD_TCP_OPEN  0x21
#define WIC64_CMD_TCP_AVAILABLE 0x30
#define WIC64_CMD_TCP_READ  0x22
#define WIC64_CMD_TCP_WRITE 0x23
#define WIC64_CMD_TCP_CLOSE 0x2e

#define WIC64_CMD_GET_SERVER 0x12
#define WIC64_CMD_SET_SERVER 0x08

#define WIC64_CMD_GET_TIMEZONE   0x17
#define WIC64_CMD_SET_TIMEZONE   0x16
#define WIC64_CMD_GET_LOCAL_TIME 0x15

#define WIC64_CMD_UPDATE_FIRMWARE 0x27

#define WIC64_CMD_REBOOT 0x29
#define WIC64_CMD_GET_STATUS_MESSAGE 0x2a
#define WIC64_CMD_SET_TIMEOUT 0x2d
#define WIC64_CMD_IS_HARDWARE 0x31

#define WIC64_CMD_FORCE_TIMEOUT 0xfc
#define WIC64_CMD_FORCE_ERROR 0xfd
#define WIC64_CMD_ECHO 0xfe

// Deprecated commands
#define WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_03 0x03
#define WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_04 0x04
#define WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_05 0x05
#define WIC64_CMD_DEPRECATED_FIRMWARE_UPDATE_REQUIRED_18 0x18
#define WIC64_CMD_DEPRECATED_GET_STATS_07 0x07
#define WIC64_CMD_DEPRECATED_LOG_TO_SERIAL_CONSOLE_09 0x09
#define WIC64_CMD_DEPRECATED_GET_UPD_0A 0x0a
#define WIC64_CMD_DEPRECATED_SEND_UPD_0B 0x0b
#define WIC64_CMD_DEPRECATED_SET_UPD_PORT_0E 0x0e
#define WIC64_CMD_DEPRECATED_GET_UPD_DUPLICATE_1E 0x1e
#define WIC64_CMD_DEPRECATED_SEND_UPD_DUPLICATE_1F 0x1f
#define WIC64_CMD_DEPRECATED_GET_EXTERNAL_IP_13 0x13
#define WIC64_CMD_DEPRECATED_GET_PREFERENCES_19 0x19
#define WIC64_CMD_DEPRECATED_SET_PREFERENCES_1A 0x1a
#define WIC64_CMD_DEPRECATED_SET_TCP_PORT_20 0x20
#define WIC64_CMD_DEPRECATED_BIG_LOADER_25 0x25
#define WIC64_CMD_DEPRECATED_FACTORY_RESET_63 0x63
#define WIC64_CMD_DEPRECATED_LEGACY_HTTP_POST_24 0x24

#define WIC64_CMD_NONE 0xff

#define WIC64_PROT_LEGACY 'W'
#define WIC64_PROT_REVISED 'R'
#define WIC64_PROT_EXTENDED 'E'
#define INPUT_EXP_PROT 0
#define INPUT_EXP_CMD 1
#define INPUT_EXP_LL 2
#define INPUT_EXP_LH 3
#define INPUT_EXP_HL 4
#define INPUT_EXP_HH 5
#define INPUT_EXP_ARGS 6

#define CONS_COL_NO ""
#define CONS_COL_RED "\x1B[31m"
#define CONS_COL_GREEN "\x1B[32m"
#define CONS_COL_BLUE "\x1B[36m"
#define CONS_COL_OFF "\033[0m\t\t"

static const char *cmd2string[256];
/* WiC64 return codes */
static uint8_t SUCCESS          = 0;
static uint8_t INTERNAL_ERROR   = 1;
static uint8_t CLIENT_ERROR     = 2;
static uint8_t CONNECTION_ERROR = 3;
static uint8_t NETWORK_ERROR    = 4;
static uint8_t SERVER_ERROR     = 5;

#define HTTPREPLY_MAXLEN ((unsigned)(16 * 1024 * 1024)) /* 16MB needed for potential large images to flash via bigloader */
static size_t httpbufferptr = 0;
static uint8_t *httpbuffer = NULL;
static char *replybuffer = NULL;

#define COMMANDBUFFER_MAXLEN    0x100010
#define URL_MAXLEN              2000
static char *encoded_helper = NULL;

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifndef WINDOWS_COMPILE
#include <unistd.h>
#endif
#include <curl/curl.h>

#define MAX_PARALLEL 1 /* number of simultaneous transfers */
#define NUM_URLS 10
static int still_alive = 0;
static CURLM *cm;                      /* used for http(s) */
static CURL *curl;                     /* used for telnet */
static uint8_t curl_buf[240];          /* this slows down by smaller chunks sent to C64, improves BBSs  */
static uint8_t *curl_send_buf = NULL;
static uint16_t curl_send_len;

/* ------------------------------------------------------------------------- */
static int userport_wic64_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_wic64_enabled == val) {
        return 0;
    }
    if (wic64_loghandle == LOG_ERR) {
        wic64_loghandle = log_open("WiC64");
    }

    userport_wic64_enabled = val;
    if (val) {
        httpbuffer = lib_malloc(HTTPREPLY_MAXLEN);
        _wic64_log(CONS_COL_NO, 2, "%s: httpreplybuffer allocated 0x%xkB", __FUNCTION__,
                   HTTPREPLY_MAXLEN / 1024);

        replybuffer = lib_malloc(HTTPREPLY_MAXLEN + 16);
        _wic64_log(CONS_COL_NO, 2, "%s: replybuffer allocated 0x%xkB", __FUNCTION__,
                   (HTTPREPLY_MAXLEN / 1024) + 16);

        encoded_helper = lib_malloc(COMMANDBUFFER_MAXLEN);
        _wic64_log(CONS_COL_NO, 2, "%s: encoded_helper allocated 0x%xkB", __FUNCTION__,
                   COMMANDBUFFER_MAXLEN / 1024);

        curl_send_buf = lib_malloc(COMMANDBUFFER_MAXLEN);
        _wic64_log(CONS_COL_NO, 2, "%s: curl_send_buf allocated 0x%xkB", __FUNCTION__,
                   COMMANDBUFFER_MAXLEN / 1014);

        wic64_set_status("enabled");
        log_message(wic64_loghandle, "WiC64 enabled");

        prep_wic64_str();

    } else {
        lib_free(httpbuffer);
        httpbuffer = NULL;
        lib_free(replybuffer);
        replybuffer = NULL;
        lib_free(encoded_helper);
        encoded_helper = NULL;
        lib_free(curl_send_buf);
        curl_send_buf = NULL;
        _wic64_log(CONS_COL_NO, 2, "%s: httpreplybuffer/replybuffer/encoded_helper/curl_send_buf freed",
                   __FUNCTION__);

        if (post_data) {
            lib_free(post_data);
            post_data = NULL;
        }

        if (curl) {
            /* connection closed */
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            curl = NULL;
        }
        if (post_url) {
            lib_free(post_url);
            post_url = NULL;
        }
        if (http_get_alarm) {
            alarm_destroy(http_get_alarm);
            http_get_alarm = NULL;
        }
        if (http_post_alarm) {
            alarm_destroy(http_post_alarm);
            http_post_alarm = NULL;
        }
        if (http_post_endalarm) {
            alarm_destroy(http_post_endalarm);
            http_post_endalarm = NULL;
        }
        if (tcp_get_alarm) {
            alarm_destroy(tcp_get_alarm);
            tcp_get_alarm = NULL;
        }
        if (tcp_send_alarm) {
            alarm_destroy(tcp_send_alarm);
            tcp_send_alarm = NULL;
        }
        if (cmd_timeout_alarm) {
            alarm_destroy(cmd_timeout_alarm);
            cmd_timeout_alarm = NULL;
        }
        wic64_set_status("disabled");
        log_message(wic64_loghandle, "WiC64 disabled");
    }

    return 0;
}

static int wic64_set_default_server(const char *val, void *v)
{
    util_string_set(&default_server_hostname, val);
    return 0;
}

static int wic64_set_macaddress(const char *val, void *v)
{
    util_string_set((char **)&wic64_mac_address, val);
    return 0;
}

static int wic64_set_ipaddress(const char *val, void *v)
{
    util_string_set((char **)&wic64_internal_ip, val);
    return 0;
}

static int wic64_set_sectoken(const char *val, void *v)
{
    util_string_set((char **)&wic64_sec_token, val);
    return 0;
}

static int wic64_set_timezone(int val, void *param)
{
    current_tz = val;
    return 0;
}

static int wic64_set_logenabled(int val, void *param)
{
    wic64_logenabled = val;
    return 0;
}

static int wic64_set_loglevel(int val, void *param)
{
    if (val > WIC64_MAXTRACELEVEL) {
      val = WIC64_MAXTRACELEVEL;
    }
    if (val < 0) {
      val = 0;
    }
    wic64_loglevel = val;
    if (wic64_loglevel == 0) {
        wic64_log(CONS_COL_NO, "setting log level to %d", wic64_loglevel);
        wic64_logenabled = 0;
        return 0;
    }
    wic64_logenabled = 1;
    wic64_log(CONS_COL_NO, "setting log level to %d", wic64_loglevel);
    return 0;
}

static int wic64_set_resetuser(int val, void *param)
{
    if (val > 32767) {
      val = 32767;
    }
    if (val < 0) {
      val = 0;
    }
    wic64_resetuser = val;
    return 0;
}

static int wic64_set_hexdumplines(int val, void *param)
{
    wic64_hexdumplines = val;
    return 0;
}

static int wic64_set_colorize_log(int val, void *param)
{
    wic64_colorize_log = val;
    return 0;
}

static int wic64_cmdl_reset(const char *val, void *param)
{
    if (param == (void *)2) {
        /* cmdline option to reset user */
        log_message(wic64_loghandle, "cmdline option: factory reset");
        int save = wic64_resetuser;
        wic64_resetuser = 1;
        userport_wic64_factory_reset();
        wic64_resetuser = save;
    }
    return 0;
}

int userport_wic64_resources_init(void)
{
    if (resources_register_string(wic64_resources) < 0) {
        return -1;
    }
    if (resources_register_int(wic64_resources_int) < 0) {
        return -1;
    }
    userport_wic64_reset();
    return userport_device_register(USERPORT_DEVICE_WIC64, &userport_wic64_device);
}

/** \brief  Free memory used by WIC64 resources
 */
void userport_wic64_resources_shutdown(void)
{
    wic64_log(CONS_COL_NO, "%s: shutting down wic64", __FUNCTION__);
    lib_free(default_server_hostname);
    lib_free(wic64_mac_address);
    lib_free(wic64_internal_ip);
    lib_free(wic64_sec_token);
    if (httpbuffer) {
        lib_free(httpbuffer);
    }
}

int userport_wic64_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/
/** \brief  Get list of timezones
 *
 * \param[out]  num_zones  number of elements in the list
 *
 * \return  list of timezones, without sentinel value to indicate end-of-list
 */
const tzones_t *userport_wic64_get_timezones(size_t *num_zones)
{
    *num_zones = sizeof timezones / sizeof timezones[0];
    return timezones;
}

static void wic64_reset_user_helper(void)
{
    char tmp[32];
    snprintf(tmp, 32, "08:d1:f9:%02x:%02x:%02x",
             lib_unsigned_rand(0, 15),
             lib_unsigned_rand(0, 15),
             lib_unsigned_rand(0, 15));
    resources_set_string("WIC64MACAddress", tmp);
    resources_set_string("WIC64SecToken", "0123456789ab");
}

void userport_wic64_factory_reset(void)
{
    int tz;
    int reset_user;
    char *defserver;

    resources_get_default_value("WIC64Timezone", (void *)&tz);
    resources_get_default_value("WIC64DefaultServer", (void *)&defserver);

    resources_set_int("WIC64Timezone", tz);
    resources_set_string("WIC64DefaultServer", defserver);

    resources_get_int("WIC64Resetuser", &reset_user);
    if (reset_user) {
        wic64_reset_user_helper();
    }
}

void wic64_set_status(const char *status)
{
    strncpy(wic64_last_status, status, sizeof wic64_last_status);
    wic64_last_status[sizeof wic64_last_status - 1u] = '\0';
}

/** \brief  log message to console
 *
 * \param[in]  typical printf format string
 */
static void wic64_log(const char *col, const char *fmt, ...)
{
    char t[256];
    va_list args;
    const char *col_before = "";
    const char *col_after = "";

    if (!wic64_logenabled) {
        return;
    }
    if (col && wic64_colorize_log) {
        col_before = col;
        col_after = CONS_COL_OFF;
    }
    va_start(args, fmt);
    vsnprintf(t, 256, fmt, args);
    log_message(wic64_loghandle, "%s%s%s", col_before, t, col_after);
}

/** \brief  debug log message to console
 *
 * \param[in]  typical printf format string
 */
static void _wic64_log(const char *col, const int lv, const char *fmt, ...)
{
    char t[256];
    va_list args;

    if (wic64_loglevel < lv)
        return;
    va_start(args, fmt);
    vsnprintf(t, 256, fmt, args);
    wic64_log(col, "%s", t);
}

/** \brief  formatted hexdump, lines limited by value of "WIC64HexdumpLines"
 *
 * \param[in]  buf, len
 */
static void hexdump(const char *col, const char *buf, int len)
{
    int i;
    int idx = 0;
    int lines = 0;
    char linestr[256];

    while (len > 0) {
        if (big_load) {
            snprintf(linestr, 256, "%08x: ", (unsigned) idx);
        } else {
            snprintf(linestr, 256, "%04x: ", (unsigned) idx);
        }

        if (wic64_hexdumplines && lines++ >= wic64_hexdumplines) {
            strcat(linestr, "...");
            wic64_log(col, "%s", linestr);
            break;
        }
        for (i = 0; i < 16; i++) {
            if (i < len) {
                char t[4];
                snprintf(t, 4, "%02x ", (uint8_t) buf[idx + i]);
                strcat(linestr, t);
            } else {
                strcat(linestr, "   ");
            }
        }
        strcat(linestr, "|");
        for (i = 0; i < 16; i++) {
            if (i < len) {
                char t[2];
                char c;
                c = isprint((unsigned char)buf[idx + i]) ? buf[idx + i] : '.';
                snprintf(t, 2, "%c", c);
                strcat(linestr, t);
            } else {
                strcat(linestr, " ");
            }
        }
        strcat(linestr, "|");
        wic64_log(col, "%s", linestr);
        idx += 16;
        len -= 16;
    }
}

static void prep_wic64_str(void)
{
    cmd2string[WIC64_CMD_GET_VERSION_STRING] = "WIC64_CMD_GET_VERSION_STRING";
    cmd2string[WIC64_CMD_GET_VERSION_NUMBERS] = "WIC64_CMD_GET_VERSION_NUMBERS";

    cmd2string[WIC64_CMD_SCAN_WIFI_NETWORKS] = "WIC64_CMD_SCAN_WIFI_NETWORKS";
    cmd2string[WIC64_CMD_IS_CONNECTED] = "WIC64_CMD_IS_CONNECTED";
    cmd2string[WIC64_CMD_IS_CONFIGURED] = "WIC64_CMD_IS_CONFIGURED";
    cmd2string[WIC64_CMD_CONNECT_WITH_SSID_STRING] = "WIC64_CMD_CONNECT_WITH_SSID_STRING";
    cmd2string[WIC64_CMD_CONNECT_WITH_SSID_INDEX] = "WIC64_CMD_CONNECT_WITH_SSID_INDEX";

    cmd2string[WIC64_CMD_GET_MAC] = "WIC64_CMD_GET_MAC";
    cmd2string[WIC64_CMD_GET_SSID] = "WIC64_CMD_GET_SSID";
    cmd2string[WIC64_CMD_GET_RSSI] = "WIC64_CMD_GET_RSSI";
    cmd2string[WIC64_CMD_GET_IP] = "WIC64_CMD_GET_IP";

    cmd2string[WIC64_CMD_HTTP_GET] = "WIC64_CMD_HTTP_GET";
    cmd2string[WIC64_CMD_HTTP_GET_ENCODED] = "WIC64_CMD_HTTP_GET_ENCODED";
    cmd2string[WIC64_CMD_HTTP_POST_URL] = "WIC64_CMD_HTTP_POST_URL";
    cmd2string[WIC64_CMD_HTTP_POST_DATA] = "WIC64_CMD_HTTP_POST_DATA";

    cmd2string[WIC64_CMD_TCP_OPEN] = "WIC64_CMD_TCP_OPEN";
    cmd2string[WIC64_CMD_TCP_AVAILABLE] = "WIC64_CMD_TCP_AVAILABLE";
    cmd2string[WIC64_CMD_TCP_READ] = "WIC64_CMD_TCP_READ";
    cmd2string[WIC64_CMD_TCP_WRITE] = "WIC64_CMD_TCP_WRITE";
    cmd2string[WIC64_CMD_TCP_CLOSE] = "WIC64_CMD_TCP_CLOSE";

    cmd2string[WIC64_CMD_GET_SERVER] = "WIC64_CMD_GET_SERVER";
    cmd2string[WIC64_CMD_SET_SERVER] = "WIC64_CMD_SET_SERVER";

    cmd2string[WIC64_CMD_GET_TIMEZONE] = "WIC64_CMD_GET_TIMEZONE";
    cmd2string[WIC64_CMD_SET_TIMEZONE] = "WIC64_CMD_SET_TIMEZONE";
    cmd2string[WIC64_CMD_GET_LOCAL_TIME] = "WIC64_CMD_GET_LOCAL_TIME";

    cmd2string[WIC64_CMD_UPDATE_FIRMWARE] = "WIC64_CMD_UPDATE_FIRMWARE";
    cmd2string[WIC64_CMD_REBOOT] = "WIC64_CMD_REBOOT";

    cmd2string[WIC64_CMD_GET_STATUS_MESSAGE] = "WIC64_CMD_GET_STATUS_MESSAGE";
    cmd2string[WIC64_CMD_SET_TIMEOUT] = "WIC64_CMD_SET_TIMEOUT";
    cmd2string[WIC64_CMD_IS_HARDWARE] = "WIC64_CMD_IS_HARDWARE";
    cmd2string[WIC64_CMD_ECHO] = "WIC64_CMD_ECHO";
    cmd2string[WIC64_CMD_FORCE_TIMEOUT]= "WIC64_CMD_FORCE_TIMEOUT";
    cmd2string[WIC64_CMD_FORCE_ERROR] = "WIC64_CMD_FORCE_ERROR";

    cmd2string[WIC64_CMD_NONE] = "unknown";
    /* deprecated commands */
    cmd2string[WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_03] = "WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_03";
    cmd2string[WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_04] = "WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_04";
    cmd2string[WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_05] = "WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_05";
    cmd2string[WIC64_CMD_DEPRECATED_FIRMWARE_UPDATE_REQUIRED_18] = "WIC64_CMD_DEPRECATED_FIRMWARE_UPDATE_REQUIRED_18";
    cmd2string[WIC64_CMD_DEPRECATED_GET_STATS_07] = "WIC64_CMD_DEPRECATED_GET_STATS_07";
    cmd2string[WIC64_CMD_DEPRECATED_LOG_TO_SERIAL_CONSOLE_09] = "WIC64_CMD_DEPRECATED_LOG_TO_SERIAL_CONSOLE_09";
    cmd2string[WIC64_CMD_DEPRECATED_GET_UPD_0A] = "WIC64_CMD_DEPRECATED_GET_UPD_0A";
    cmd2string[WIC64_CMD_DEPRECATED_SEND_UPD_0B] = "WIC64_CMD_DEPRECATED_SEND_UPD_0B";
    cmd2string[WIC64_CMD_DEPRECATED_SET_UPD_PORT_0E] = "WIC64_CMD_DEPRECATED_SET_UPD_PORT_0E";
    cmd2string[WIC64_CMD_DEPRECATED_GET_UPD_DUPLICATE_1E] = "WIC64_CMD_DEPRECATED_GET_UPD_DUPLICATE_1E";
    cmd2string[WIC64_CMD_DEPRECATED_SEND_UPD_DUPLICATE_1F] = "WIC64_CMD_DEPRECATED_SEND_UPD_DUPLICATE_1F";
    cmd2string[WIC64_CMD_DEPRECATED_GET_EXTERNAL_IP_13] = "WIC64_CMD_DEPRECATED_GET_EXTERNAL_IP_13";
    cmd2string[WIC64_CMD_DEPRECATED_GET_PREFERENCES_19] = "WIC64_CMD_DEPRECATED_GET_PREFERENCES_19";
    cmd2string[WIC64_CMD_DEPRECATED_SET_PREFERENCES_1A] = "WIC64_CMD_DEPRECATED_SET_PREFERENCES_1A";
    cmd2string[WIC64_CMD_DEPRECATED_SET_TCP_PORT_20] = "WIC64_CMD_DEPRECATED_SET_TCP_PORT_20";
    cmd2string[WIC64_CMD_DEPRECATED_BIG_LOADER_25] = "WIC64_CMD_DEPRECATED_BIG_LOADER_25";
    cmd2string[WIC64_CMD_DEPRECATED_FACTORY_RESET_63] = "WIC64_CMD_DEPRECATED_FACTORY_RESET_63";
    cmd2string[WIC64_CMD_DEPRECATED_LEGACY_HTTP_POST_24] = "WIC64_CMD_DEPRECATED_LEGACY_HTTP_POST_24";
}

/** \brief  formatted hexdump, lines limited by value of "WIC64HexdumpLines" for debug level
 *
 * \param[in]  lv, buf, len
 */
static void _hexdump(const char *col, const int lv, const char *buf, int len)
{
    if (wic64_loglevel < lv) {
        return;
    }
    hexdump(col, buf, len);
}

static size_t write_cb(char *data, size_t n, size_t l, void *userp)
{
    size_t tmp = httpbufferptr + n * l;

    if (tmp >= HTTPREPLY_MAXLEN) {
        wic64_log(CONS_COL_NO, "libcurl reply too long, dropping %"PRI_SIZE_T" bytes.\n",
                  tmp - HTTPREPLY_MAXLEN);
        return CURLE_WRITE_ERROR;
    }
    memcpy(&httpbuffer[httpbufferptr], data, n * l);
    httpbufferptr += (n * l);
    return n*l;
}

static void add_transfer(CURLM *cmulti, char *url)
{
    CURL *eh = curl_easy_init();
    if (wic64_loglevel > 1) {
        curl_easy_setopt(eh, CURLOPT_VERBOSE, 1L);
    } else {
        curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
    }
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(eh, CURLOPT_URL, url);
    curl_easy_setopt(eh, CURLOPT_PRIVATE, url);
    /* work around bug 1964 (https://sourceforge.net/p/vice-emu/bugs/1964/) */
#ifdef CURLSSLOPT_NATIVE_CA
    curl_easy_setopt(eh, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
    /* set USERAGENT: otherwise the server won't return data, e.g. wicradio */
    if (wic64_protocol == WIC64_PROT_LEGACY) {
        http_user_agent = HTTP_AGENT_LEGACY;
    } else {
        http_user_agent = HTTP_AGENT_REVISED;
    }
    curl_easy_setopt(eh, CURLOPT_USERAGENT, http_user_agent);
    curl_multi_add_handle(cmulti, eh);
}

static void update_prefs(uint8_t *buffer, size_t len)
{
    /* manage preferences in memory only for now */
    wic64_log(CONS_COL_NO, "%s: requested", __FUNCTION__);
    hexdump(CONS_COL_NO, (char *)buffer, (int)len);
    char *t;
    char *p;
    char *pref = NULL;
    char *val = "";
    char *ret = "";

    if (len > 0) {
        p = (char *)buffer + 1; /* skip \001 */
        t = strchr(p, '\001');
        if ((t != NULL) && ((t - p) < 31)) {
            *t = '\0';
            pref = p;
        }
        p = t + 1; /* skip \0 */
        t = strchr(p, '\001');
        if ((t != NULL) && ((t - p) < 31)) {
            *t = '\0';
            val = p;
        }
        ret = t + 1; /* hope string is terminated */
        wic64_log(CONS_COL_NO, "WiC64: user-pref '%s' = '%s', ret = '%s'", pref, val, ret);
    } else {
        return;
    }

    if (sec_init &&
        (strcmp(pref, sec_token) == 0)) {
        resources_set_string("WIC64SecToken", val);
        wic64_log(CONS_COL_NO, "%s: session id = %s", __FUNCTION__, val);
    }
    if (strcmp(pref, TOKEN_NAME) == 0) {
        strncpy(sec_token, val, 31);
        wic64_log(CONS_COL_NO, "%s: token = %s", __FUNCTION__, sec_token);
        if (strcmp(sec_token, "KIGMYPLA2021") != 0) {
            wic64_log(CONS_COL_NO, "WiC64: sectoken changed: '%s' - resource won't match", val);
        }
        sec_init = 1;
    }
    send_reply_revised(SUCCESS, "Success", (uint8_t *)ret, strlen(ret), NULL);
}

static void http_get_alarm_handler(CLOCK offset, void *data)
{
    CURLMsg *msg;
    CURLMcode r;
    int msgs_left = -1;
    long response = -1;
    char *url = "<unknown>";

    r = curl_multi_perform(cm, &still_alive);
    if (r != CURLM_OK) {
        wic64_log(CONS_COL_NO, "%s: curl_multi_perform failed: %s", __FUNCTION__, curl_multi_strerror(r));
        msg = curl_multi_info_read(cm, &msgs_left);
        if (msg) {
            _wic64_log(CONS_COL_RED, 2, "%s: msg: %u, %s", __FUNCTION__,
                       msg->data.result, curl_easy_strerror(msg->data.result));
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
            _wic64_log(CONS_COL_RED, 2, "%s, R: %u - %s <%s>", __FUNCTION__,
                       msg->data.result, curl_easy_strerror(msg->data.result), url);
        }
        send_reply_revised(NETWORK_ERROR, "Failed to read HTTP response", NULL, 0, "!0"); /* maybe wrong here */
        goto out;
    }
    if (still_alive) {
        /* http request not yet finished */
        return;
    }
    msg = curl_multi_info_read(cm, &msgs_left);
    if (msg) {
        CURLcode res;
        res = curl_easy_getinfo(msg->easy_handle,
                                CURLINFO_RESPONSE_CODE,
                                &response);
        if (res != CURLE_OK) {
            wic64_log(CONS_COL_RED, "%s: curl_easy_getinfo(...&response failed: %s", __FUNCTION__,
                      curl_easy_strerror(res));
            send_reply_revised(NETWORK_ERROR, "Failed to read HTTP response", NULL, 0, "!0");
            goto out;
        }
        res = curl_easy_getinfo(msg->easy_handle, CURLINFO_EFFECTIVE_URL, &url);
        if (res != CURLE_OK) {
            /* ignore problem, URL is only for debugging */
            wic64_log(CONS_COL_RED, "%s: curl_easy_getinfo(...&URL failed: %s", __FUNCTION__,
                      curl_easy_strerror(res));
            url = "<unknown>";
        }
    }
    if (response == 201) {
        /* prefs update requested, handles replies */
        update_prefs(httpbuffer, httpbufferptr);
        goto out;
    }

    if (response == 200) {
        wic64_log(CONS_COL_NO, "%s: got %lu bytes, URL: '%s', http code = %ld", __FUNCTION__,
                  httpbufferptr, url, response);
        if (wic64_protocol == WIC64_PROT_LEGACY) {
            char *t;
            /* check weird .prg -> cheat length */
            t = strrchr(url, '.');
            if ((t != NULL) && (strcmp(t, ".prg") == 0)){
                cheatlen = -2;
                wic64_log(CONS_COL_GREEN, "prg URL -> reducing reported len by 2");
            }
        }
        send_reply_revised(SUCCESS, "Success", httpbuffer, httpbufferptr, NULL); /* raw send, supporting big_load */
        cheatlen = 0;
    } else if (response >= 400) {
        char t[32];
        wic64_log(CONS_COL_RED, "URL '%s' returned %lu bytes (http code: %ld)", url, httpbufferptr, response);
        snprintf(t, 31, "http response: %ld", response);
        send_reply_revised(SERVER_ERROR, t, NULL, 0, "!0");      /* raw send supporting big_load */
    } else {
        /* firmeare handles codes: 301, 302, 307, 308 - check if needed with libcurl */
        char m[64];
        snprintf(m, 64, "Unhandled http response %ld", response);
        wic64_log(CONS_COL_RED, m);
        send_reply_revised(INTERNAL_ERROR, m, NULL, 0, "!0");
    }

  out:
    curl_multi_cleanup(cm);
    curl_global_cleanup();
    alarm_unset(http_get_alarm);
    memset(httpbuffer, 0, httpbufferptr);
    big_load = 0;
}

static void do_http_get(char *url)
{
    cm = curl_multi_init();
    if (!cm) {
        send_reply_revised(CONNECTION_ERROR, "Can't send HTTP request", NULL, 0, "!0");
        return;
    }

    /* Limit the amount of simultaneous connections curl should allow: */
    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);

    still_alive = 1;
    httpbufferptr = 0;
    add_transfer(cm, url);

    if (http_get_alarm == NULL) {
        http_get_alarm = alarm_new(maincpu_alarm_context, "HTTPGetAlarm",
                                   http_get_alarm_handler, NULL);
    }
    alarm_unset(http_get_alarm);
    alarm_set(http_get_alarm, maincpu_clk + (312 * 65));
}

/* ---------------------------------------------------------------------*/

#define FLAG2_ACTIVE    0
#define FLAG2_INACTIVE  1
#define FLAG2_TOGGLE_DELAY 3    /* delay in cycles to toggle flag2 */

static uint8_t input_state = 0, input_command = WIC64_CMD_NONE;
static uint8_t wic64_inputmode = 1;
static uint32_t input_length = 0, commandptr = 0;
static uint8_t commandbuffer[COMMANDBUFFER_MAXLEN];

static uint32_t replyptr = 0, reply_length = 0;
static uint8_t reply_port_value = 0;
static struct alarm_s *flag2_alarm = NULL;

static void flag2_alarm_handler(CLOCK offset, void *data)
{
    set_userport_flag(FLAG2_INACTIVE);
    alarm_unset(flag2_alarm);
}

/* a handshake is triggered after:
   - each byte the esp received from the c64 (inputmode = true)
   - each byte put on the userport for the c64 to fetch (inputmode = false, transferdata = true)
   - a command, to signal the c64 there is a reply
*/
static void handshake_flag2(void)
{
    if (flag2_alarm == NULL) {
        flag2_alarm = alarm_new(maincpu_alarm_context, "FLAG2Alarm", flag2_alarm_handler, NULL);
    }
    alarm_unset(flag2_alarm);
    alarm_set(flag2_alarm, maincpu_clk + FLAG2_TOGGLE_DELAY);

    set_userport_flag(FLAG2_ACTIVE);
    /* set_userport_flag(FLAG2_INACTIVE); */
}

static void reply_next_byte(void)
{
    if (replyptr < reply_length) {
        reply_port_value = replybuffer[replyptr];
        /* _wic64_log(CONS_COL_NO, 2, "reply_next_byte: %3u/%3u - %02x'%c'", replyptr, reply_length, reply_port_value, isprint((unsigned char)reply_port_value)?reply_port_value:'.')); */
        replyptr++;
        if (replyptr == reply_length) {
            replyptr = reply_length = 0;
        }
    } else {
        replyptr = reply_length = 0;
    }
}

static void send_binary_reply(const uint8_t *reply, size_t len)
{
    int offs;

    /* highbyte first! */
    if (big_load) {
        offs = 4;
        replybuffer[3] = len & 0xff;
        replybuffer[2] = (len >> 8) & 0xff;
        replybuffer[1] = (len >> 16) & 0xff;
        replybuffer[0] = (len >> 24) & 0xff;

    } else {
        offs = 2;
        replybuffer[1] = (len + cheatlen) & 0xff;
        replybuffer[0] = ((len + cheatlen) >> 8) & 0xff;
    }

    memcpy((char*)replybuffer + offs, reply, len);
    reply_length = (uint32_t)(len + offs);
    replyptr = 0;
    if (len > 0) {
        wic64_log(CONS_COL_GREEN, "sends %d/0x%x bytes...", len, len);
        hexdump(CONS_COL_GREEN, replybuffer, reply_length);
    } else {
        wic64_log(CONS_COL_GREEN, "handshake flag2");
    }
    handshake_flag2();
}

static void cmd_timeout_alarm_handler(CLOCK offset, void *data)
{
    wic64_log(CONS_COL_RED, "timed out - '%s' command", cmd2string[input_command]);
    replyptr = reply_length = force_timeout = 0;
    input_state = INPUT_EXP_PROT;
    commandptr = 0;
    alarm_unset(cmd_timeout_alarm);
}

static void cmd_timeout(int arm)
{
    if (wic64_protocol == WIC64_PROT_LEGACY) {
        return;                 /* legacy won't support timeouts */
    }
    if (cmd_timeout_alarm == NULL) {
        cmd_timeout_alarm = alarm_new(maincpu_alarm_context, "CMDTimoutAlarm",
                                      cmd_timeout_alarm_handler, NULL);
    }
    alarm_unset(cmd_timeout_alarm);
    if (arm) {
        alarm_set(cmd_timeout_alarm, maincpu_clk + wic64_timeout * machine_get_cycles_per_second());
    }
}

static void send_reply_revised(const uint8_t rcode, const char *msg, const uint8_t *payload, size_t len, const char *legacy_msg)
{
    int offs = 0;

    wic64_set_status(msg);

    if (wic64_protocol != WIC64_PROT_LEGACY) {
        const char *col;

        if (rcode == SUCCESS) {
            col = CONS_COL_GREEN;
        } else {
            col = CONS_COL_RED;
        }
        wic64_log(col, "replies %s", msg);
        replybuffer[0] = rcode;
        replybuffer[1] = len & 0xff; /* little endian */
        replybuffer[2] = (len >> 8) & 0xff;
        if (wic64_protocol == WIC64_PROT_EXTENDED) {
            offs = 2;
            replybuffer[3] = (len >> 16) & 0xff;
            replybuffer[4] = (len >> 24) & 0xff;
        }
        memcpy((char *) &replybuffer[3 + offs], (const char *)payload, len);
        _wic64_log(col, 2, "sends header...");
        _hexdump(col, 2, replybuffer, 3 + offs);
        if (len > 0) {
            wic64_log(col, "sends payload %d/0x%x bytes...", len, len);
            hexdump(col, &replybuffer[3 + offs], (int)len);
        }
        reply_length = (uint32_t)(len + 3 + offs);

        cmd_timeout(1);         /* arm alarm handler */
        handshake_flag2();
    } else {
        /* legacy protocol */
        if (legacy_msg && payload) {
            wic64_log(CONS_COL_RED,
                      "protocol error: can't send both payload and legacy message: '%s' discarded.",
                      legacy_msg);
            return;
        }
        if (legacy_msg) {
            /* legacy_msg becomes the payload */
            payload = (uint8_t *)legacy_msg;
            len = strlen(legacy_msg);
        }
        /* always send a response, even if payload == NULL and len == 0,
           so that a response header is always send, even if it's [$00, $00] */
        send_binary_reply(payload, len);
    }
}

/* ----------- WiC64 commands ----------- */
static void cmd_get_version(int variant)
{
    static uint8_t version[4] = {
        WIC64_VERSION_MAJOR,
        WIC64_VERSION_MINOR,
        WIC64_VERSION_PATCH,
        WIC64_VERSION_DEVEL,
    };
    if (variant == WIC64_CMD_GET_VERSION_STRING) {
        send_reply_revised(SUCCESS, "Success",
                           (uint8_t *)WIC64_VERSION_STRING,
                           strlen(WIC64_VERSION_STRING) + 1, NULL);
    } else {
        send_reply_revised(SUCCESS, "Success", version, 4, NULL);
    }
}

static int _encode(char **p, int len)
{
    int enc_it = 0;
    int i;
    static char hextab[16] = "0123456789abcdef";

    for (i = 0; i < len; i++) {
        encoded_helper[enc_it++] = hextab[((**p) >> 4) & 0xf];
        encoded_helper[enc_it++] = hextab[(**p) & 0xf];
        (*p)++;
    }
    encoded_helper[enc_it] = '\0';
    return enc_it;
}

/* encode binary after escape '$<' */
static void cmd_http_get_encoded(void)
{
    char *p;
    char *endmarker;
    char *cptr;
    char *tptr;
    char temppath[COMMANDBUFFER_MAXLEN];
    int len;
    int l;

    hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr);

    /* if encode is enabled, there might be binary data after <$, which is
       then encoded as a stream of hex digits */
    cptr = (char*)commandbuffer;
    tptr = temppath;
    endmarker = cptr + commandptr;
    len = 0;
    while ((len < COMMANDBUFFER_MAXLEN) &&
           ((p = strstr(cptr, "<$")) != NULL) &&
           (cptr < endmarker)) {
        l = (int)(p - cptr);
        len += l;
        _wic64_log(CONS_COL_NO, 2, "%s: escape sequence found, offset %d", __FUNCTION__, len);
        /* copy string before <$ */
        memcpy(tptr, cptr, (size_t)l);
        tptr += l;
        l = p[2];
        l += p[3] << 8;
        p += 4; /* skip escape sequence and len */
        l = _encode(&p, l);
        memcpy(tptr, encoded_helper, l);
        len += l;
        cptr = p;
        tptr += l;
    }
    l = 0;
    if (cptr < endmarker) {
        /* copy remaining commandbuffer */
        l = (int)(endmarker - cptr);
        memcpy(temppath + len, cptr, (size_t)l);
    }
    commandptr = len + l;
    memcpy(commandbuffer, temppath, commandptr);
    commandbuffer[commandptr] = '\0'; /* URL must be a valid string */
}

static int http_expand_url(char *final_url)
{
    char *p;
    char temppath[COMMANDBUFFER_MAXLEN];
    int i;
    char *cur = final_url;

    hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr); /* commands may contain '0' */

    if (commandptr == 0) {
        send_reply_revised(CLIENT_ERROR, "URL not specified", NULL, 0, "!E");
    }

    /* see below, noprintables in pid=.. need to be overruled, otherwise libcurl complains */
    p = strstr((const char *)commandbuffer, "&pid=");
    if (p != NULL) {
        if (!isprint((unsigned char)*(p+5))) {
            wic64_log(CONS_COL_NO, "%s: patching &pid=X.", __FUNCTION__);
            *(p + 5) = 'X';
        }
        if (!isprint((unsigned char)*(p+6))) {
            wic64_log(CONS_COL_NO, "%s: patching &pid=.Y", __FUNCTION__);
            *(p + 6) = 'Y';
        }
    }

    /* sanity check if URL is OK in principle */
    for (i = 0; i < commandptr; i++) {
        if (!isprint(commandbuffer[i])) {
            wic64_log(CONS_COL_RED, "bad char '0x%02x' detected in URL at offet %d, %s",
                      commandbuffer[i], i, commandbuffer);
            send_reply_revised(CLIENT_ERROR, "Malformed URL", NULL, 0, "!0");
            return -1;
        }
    }

    /* if url begins with !, replace by default server */
    if (commandbuffer[0] == '!') {
        const char *sv;
        resources_get_string("WIC64DefaultServer", &sv);
        wic64_log(CONS_COL_NO, "URL starts with !, default server is: %s", sv);
        p = temppath;
        /* add the default server address */
        strcpy(p, sv);
        p += strlen(sv);
        /* copy command buffer */
        memcpy(p, commandbuffer + 1, COMMANDBUFFER_MAXLEN - strlen(sv));
        /* copy back to commandbuffer buffer */
        memcpy(commandbuffer, temppath, COMMANDBUFFER_MAXLEN);
    }

    /* detect protocol and split path/hostname */
    p = (char*)commandbuffer;
    if (!strncmp(p, "http://", 7)) {
        strcpy(cur, "http://");
        cur += 7;
        p += 7;
    } else {
        if (!strncmp(p, "https://", 8)) {
            strcpy(cur, "https://");
            cur += 8;
            p += 8;
        } else {
            wic64_log(CONS_COL_RED, "malformed URL: %s", commandbuffer);
            send_reply_revised(CLIENT_ERROR, "Malformed URL", NULL, 0, "!0");
            return -1;
        }
    }

    p = strtok(p, "/");
    strcpy(cur, p);
    cur += strlen(p);
    *cur = '/'; cur++;
    *cur = '\0';
    p = (char*)commandbuffer;
    p += strlen(final_url);
    memcpy(cur, p, URL_MAXLEN - (p - (char*)commandbuffer));

    /* replace "%mac" by our MAC */
    p = strstr(cur, "%mac");
    if (p != NULL) {
        char macstring[64]; /* MAC + session_id */
        const char *sess;
        resources_get_string("WIC64SecToken", &sess);
        /* copy string before %mac */
        strncpy(temppath, cur, p - cur);
        temppath[p - cur] = 0;
        /* add the MAC address */
        sprintf(macstring, "%c%c%c%c%c%c%c%c%c%c%c%c%s",
                wic64_mac_address[0], wic64_mac_address[1],
                wic64_mac_address[3], wic64_mac_address[4],
                wic64_mac_address[6], wic64_mac_address[7],
                wic64_mac_address[9], wic64_mac_address[10],
                wic64_mac_address[12], wic64_mac_address[13],
                wic64_mac_address[15], wic64_mac_address[16],
                sess);
        strcat(temppath, macstring);
        strcat(temppath, p + 4);
        /* copy back to path buffer */
        strcpy(cur, temppath);
    }
    /* replace "%ser" by the default server */
    p = strstr(cur, "%ser");
    if (p != NULL) {
        const char *sv;
        resources_get_string("WIC64DefaultServer", &sv);
        /* copy string before %ser */
        strncpy(temppath, cur, p - cur);
        temppath[p - cur] = 0;
        /* add the default server address */
        strcat(temppath, sv);
        /* copy string after %ser */
        strcat(temppath, p + 4);
        /* copy back to path buffer */
        strcpy(cur, temppath);
        _wic64_log(CONS_COL_NO, 2, "temppath:%s", temppath);
    }
    /* now strip trailing whitespaces of path */
    p = cur + strlen(cur) - 1;
    while (isspace((unsigned char)*p)) {
        *p = '\0';
        p--;
    }
    /* remove trailing nonprintables - otherwise libcurl rejects the URL
       probably a bug on the app side, fixes at least artillery duel */
    while (!isprint((unsigned char)*p)) {
        *p = '\0';
        p--;
    }
    _wic64_log(CONS_COL_NO, 2, "%s: URL = '%s'", __FUNCTION__, final_url);

    if (strlen(final_url) > URL_MAXLEN) {
        send_reply_revised(CLIENT_ERROR, "URL too long (max 2000 bytes)", NULL, 0, "!E");
        return -1;
    }
    return 0;
}

/* http get */
static void cmd_http_get(void)
{
    char url[URL_MAXLEN];

    if (http_expand_url(url) < 0) {
        return;
    }
    do_http_get(url);
}

static void http_post_endalarm_handler(CLOCK offset, void *data)
{
    _wic64_log(CONS_COL_NO, 2, "http post endalarm triggered...");
    send_reply_revised(SUCCESS, "Success", (uint8_t *)post_data, post_data_rcvd, NULL);
    alarm_unset(http_post_endalarm);
    alarm_unset(http_post_alarm);
    lib_free(post_data);
    post_data = NULL;
    post_error = post_data_rcvd = post_data_new = 0;
    _wic64_log(CONS_COL_NO, 2, "http post done");
}

static void http_post_alarm_handler(CLOCK offset, void *data)
{
    _wic64_log(CONS_COL_NO, 2, "%s: post_data_rcvd = %d, expected = %d",
               __FUNCTION__, post_data_rcvd, post_data_size);

    if (post_error) {
        send_reply_revised(SERVER_ERROR, "Server error",
                           (uint8_t *)post_data,
                           post_data_rcvd, NULL);
        goto out;
    }
    if (post_data_rcvd >= post_data_size) { /* post_data_size is size_t so unsigned */
        send_reply_revised(SUCCESS, "Success", (uint8_t *)post_data, post_data_rcvd, NULL);
        goto out;
    }
    if (post_data_size == -1) {
        /* reply header didn't tell us the size, so read up to 64kB */
        if (post_data_rcvd > 0xffff) {
            send_reply_revised(SUCCESS, "Success", (uint8_t *)post_data, 0xffff, NULL);
            goto out;
        }
        /* if new data has arrived, re-arm end-timeout */
        if (post_data_new != post_data_rcvd) {
            alarm_unset(http_post_endalarm);
            /* reset alarm to 33% of wic64 timeout - should be fine in general */
            alarm_set(http_post_endalarm,
                      maincpu_clk + wic64_timeout * machine_get_cycles_per_second() / 3);
            post_data_new = post_data_rcvd;
        }
    }
    alarm_set(http_post_alarm, maincpu_clk +  machine_get_cycles_per_second() / 2);
    return;

out:
    alarm_unset(http_post_endalarm);
    alarm_unset(http_post_alarm);
    lib_free(post_data);
    post_data = NULL;
    post_error = post_data_new = post_data_rcvd = 0;
    _wic64_log(CONS_COL_NO, 2, "http post done");
}

static size_t post_write_func(char *buffer, size_t size, size_t nitems, void *userdata)
{
    CURLcode res;
    curl_off_t cl;
    size_t ret;

    ret = size * nitems;
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
    _wic64_log(CONS_COL_NO, 2, "http_post returned %d/0x%x bytes, expected len = %d...", ret, ret, cl);
    if (res != CURLE_OK) {
        _wic64_log(CONS_COL_NO, 2, "post callback failed to read length");
        post_error = 1;
        goto out;
    }
    if ((post_data_rcvd + ret) > HTTPREPLY_MAXLEN) {
        post_error = 1;
        ret = HTTPREPLY_MAXLEN - post_data_rcvd;
    }
    memcpy(post_data + post_data_rcvd, buffer, ret);
    post_data_rcvd += ret;
    _hexdump(CONS_COL_NO, 2, buffer, (int)ret);
  out:
    post_data_size = (size_t) cl; /* if -1 => largest size_t, so unknown */
    return ret;
}

static void cmd_http_post(int cmd)
{
    CURLcode res;
    static curl_mime *mime;
    static curl_mimepart *part;

    if (cmd == WIC64_CMD_HTTP_POST_URL) {
        if (post_url == NULL) {
            post_url = lib_malloc(URL_MAXLEN);
        }
        if (http_expand_url(post_url) < 0) {
            return;
        }
        send_reply_revised(SUCCESS, "Success", NULL, 0, NULL);
    } else {
        hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr);

        if (post_url == NULL) {
            send_reply_revised(CLIENT_ERROR, "URL not specified", NULL, 0, "!0");
            return;
        }

        if (!curl) {
            curl = curl_easy_init();
        }

        if (wic64_loglevel > 1) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        } else {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        }
        res = curl_easy_setopt(curl, CURLOPT_USERAGENT, http_user_agent);
        if (res != CURLE_OK) {
            wic64_log(CONS_COL_NO, "curl set user agent failed: %s", curl_easy_strerror(res));
            send_reply_revised(NETWORK_ERROR, "Failed to open connection", NULL, 0, "!0");
            return;
        }
        curl_easy_setopt(curl, CURLOPT_URL, post_url);
        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, post_write_func);
        if (res != CURLE_OK) {
            wic64_log(CONS_COL_NO, "perform failed: %s", curl_easy_strerror(res));
            send_reply_revised(NETWORK_ERROR, "Failed to open connection", NULL, 0, "!0");
            return;
        }
        post_data_rcvd = post_data_new = post_error = 0;
        post_data_size = HTTPREPLY_MAXLEN;
        /* first time prepare for receiving post resonses */
        post_data = lib_malloc(HTTPREPLY_MAXLEN);
        if (!post_data) {
            send_reply_revised(INTERNAL_ERROR, "Out of memory", NULL, 0, NULL);
            return;
        }
        mime = curl_mime_init(curl);
        part = curl_mime_addpart(mime);
        curl_mime_data(part, (const char *) commandbuffer, (size_t) commandptr);
        /* Build an HTTP form with a single field named "data", */
        curl_mime_name(part, "data");
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            wic64_log(CONS_COL_NO, "perform failed: %s", curl_easy_strerror(res));
            send_reply_revised(NETWORK_ERROR, "Failed to send POST data to server", NULL, 0, "!0");
        }
        if (http_post_alarm == NULL) {
            http_post_alarm = alarm_new(maincpu_alarm_context, "HTTPPostAlarm",
                                        http_post_alarm_handler, NULL);
        }
        if (http_post_endalarm == NULL) {
            http_post_endalarm = alarm_new(maincpu_alarm_context, "HTTPPostAlarm",
                                           http_post_endalarm_handler, NULL);
        }
        alarm_unset(http_post_alarm);
        alarm_set(http_post_alarm, maincpu_clk + (312 * 65));
    }
}

/* set wlan ssid + password */
static void cmd_wifi(int cmd)
{
    hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */

    int l = (wic64_protocol == WIC64_PROT_LEGACY) ? 0 : 1; /* kludge to make it compatible */

    switch (cmd) {
    case WIC64_CMD_SCAN_WIFI_NETWORKS:
        send_reply_revised(SUCCESS, "Success",
                           (uint8_t *) "00\001vice-emulation\00199\001",
                           strlen("00\001vice-emulation\00199\001") + l,
                           NULL);
        break;
    case WIC64_CMD_IS_CONFIGURED:
        send_reply_revised(SUCCESS, "Success", NULL, 0, NULL);
        break;
    case WIC64_CMD_CONNECT_WITH_SSID_STRING:
    case WIC64_CMD_CONNECT_WITH_SSID_INDEX:
    case WIC64_CMD_IS_CONNECTED:
        send_reply_revised(SUCCESS, "wifi config changed", NULL, 0, "0");
        break;
    case WIC64_CMD_GET_SSID:
        send_reply_revised(SUCCESS, "Success",
                           (uint8_t*) "vice-emulation",
                           strlen("vice-emulation") + l,
                           NULL);
        break;
    case WIC64_CMD_GET_RSSI:
        send_reply_revised(SUCCESS, "Success",
                           (uint8_t *) "99", strlen("99") + l, NULL);
        break;
    default:
        break;
    }
}

/* get wic64 ip address */
static void cmd_get_network(int cmd)
{
    char buffer[0x20];
    if (cmd == WIC64_CMD_GET_MAC) {
        sprintf(buffer, "%s", wic64_mac_address);
    }
    if (cmd == WIC64_CMD_GET_IP) {
        sprintf(buffer, "%s", wic64_internal_ip);
    }
    send_reply_revised(SUCCESS, "Success", (uint8_t *)buffer, strlen(buffer) + 1, NULL);
}


/* get timezone+time */
static void cmd_get_local_time(void)
{
    int dst;

    static char timestr[64];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    if ((tm == NULL) ||
        (t < 0)) {
        send_reply_revised(INTERNAL_ERROR, "Could not get local time", NULL, 0, NULL);
        return;
    }

    dst = tm->tm_isdst; /* this is somehow wrong, get dst vom target tz */
    t = t + timezones[current_tz].hour_offs * 3600 +
        ((dst > 0) ? 3600 : 0) * timezones[current_tz].dst + /* some TZs have DST others not */
        timezones[current_tz].min_offs * 60;
    tm = gmtime(&t); /* now get the UTC */
    snprintf(timestr, 63, "%02d:%02d:%02d %02d-%02d-%04d",
             tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_mday, tm->tm_mon+1, tm->tm_year + 1900);
    wic64_log(CONS_COL_NO, "get timezone + time, returning '%s'", timestr);
    send_reply_revised(SUCCESS, "Success", (uint8_t *)timestr, strlen(timestr), NULL);
}

/* set timezone */
static void cmd_set_timezone(void)
{
    hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
    wic64_timezone[0] = commandbuffer[0];
    wic64_timezone[1] = commandbuffer[1];

    int tzidx = commandbuffer[1] * 10 + commandbuffer[0];
    if (tzidx < sizeof(timezones) / sizeof (tzones_t)) {
        wic64_log(CONS_COL_NO, "setting time to %s: %dh:%dm",
                  timezones[tzidx].tz_name,
                  timezones[tzidx].hour_offs,
                  timezones[tzidx].min_offs);
        resources_set_int("WIC64Timezone", tzidx);
    } else {
        wic64_log(CONS_COL_NO, "timezone index = %d - out of range", tzidx);
    }
    send_reply_revised(SUCCESS, "Success", NULL, 0, NULL);
}

/* get timezone */
static void cmd_get_timezone(void)
{
    char buf[16];
    hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
    snprintf(buf, 16, "%d",
             timezones[current_tz].hour_offs * 3600 +
             timezones[current_tz].min_offs * 60);
    wic64_log(CONS_COL_NO, "%s: get timezone, returning '%s'", __FUNCTION__, buf);
    send_reply_revised(SUCCESS, "Success", (uint8_t *)buf, strlen(buf) + 1, NULL);
}

/* open a curl connection */
static void do_connect(uint8_t *buffer)
{
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        send_reply_revised(NETWORK_ERROR, "Could not open connection", NULL, 0, "!E");
        return;
    }

    if (wic64_loglevel > 1) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    }

    curl_easy_setopt(curl, CURLOPT_URL, buffer);
    /* Do not do the transfer - only connect to host */
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
    res = curl_easy_perform(curl);
    wic64_log(CONS_COL_NO, "%s: curl_easy_perform: %s",__FUNCTION__, curl_easy_strerror(res));
    if (res != CURLE_OK) {
        send_reply_revised(NETWORK_ERROR, "Could not open connection", NULL, 0, "!E");
    } else {
        send_reply_revised(SUCCESS, "Success", NULL, 0, "0");
    }
}

static void cmd_tcp_open(void)
{
    char tmp[COMMANDBUFFER_MAXLEN];
    hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr); /* commands may contain '0' */

    if (commandptr == 0) {
        send_reply_revised(CLIENT_ERROR, "No URL specified", NULL, 0, "!0");
        return;
    }

    strcpy(tmp, "telnet://");
    memcpy(tmp + 9, commandbuffer, commandptr + 1); /* copy '\0' */
    commandptr += 9;
    memcpy(commandbuffer, tmp, commandptr + 1);
    do_connect(commandbuffer);
}

static void cmd_tcp_available(void)
{
    CURLcode res;
    curl_socket_t sockfd;
    uint8_t t[2];
    int bytes_available;

    if (!curl) {
        send_reply_revised(NETWORK_ERROR, "NO CONNECTION", NULL, 0, NULL);
        return;
    }
    res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);
    if (res != CURLE_OK) {
        send_reply_revised(NETWORK_ERROR, "NETWORK ERROR", NULL, 0, NULL);
        return;
    }

    if (archdep_socketpeek(sockfd, &bytes_available) < 0) {
        send_reply_revised(NETWORK_ERROR, "NETWORK ERROR", NULL, 0, NULL);
        return;
    }
    t[0] = bytes_available & 0xff;
    t[1] = (bytes_available >> 8) & 0xff;
    send_reply_revised(SUCCESS, "Success", t, 2, NULL);
}

static void tcp_get_alarm_handler(CLOCK offset, void *data)
{
    CURLcode res;
    size_t nread;
    static size_t total_read;

    if (!curl) {
        return;                 /* connection might be closed */
    }
    res = curl_easy_recv(curl,
                         curl_buf + total_read,
                         sizeof(curl_buf) - total_read,
                         &nread);
    alarm_set(tcp_get_alarm, maincpu_clk + (312 * 65 * 30));
    total_read += nread;
    if ((res == CURLE_OK) && (nread == 0)) {
        /* connection closed */
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        alarm_unset(tcp_get_alarm);
        curl = NULL;
        wic64_log(CONS_COL_NO, "%s: connection closed", __FUNCTION__);
    }

    if ((res == CURLE_OK) || (res == CURLE_AGAIN)) {
        if (nread) {
            wic64_log(CONS_COL_NO, "%s: nread = %lu, total_read = %lu", __FUNCTION__, nread, total_read);
        }
        big_load = 0;
        send_reply_revised(SUCCESS, "Success", curl_buf, total_read, NULL);
    } else {
        wic64_log(CONS_COL_RED, "%s: curl_easy_recv: %s", __FUNCTION__, curl_easy_strerror(res));
        send_reply_revised(NETWORK_ERROR, "TCP connection closed", NULL, 0, "!E");
    }
    total_read = 0;
}

static void cmd_tcp_read(void)
{
    if (commandptr > 0) {
        hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
    }

    if (!curl) {
        wic64_log(CONS_COL_RED, "%s: connection lost", __FUNCTION__);
        send_reply_revised(NETWORK_ERROR, "TCP connection closed", NULL, 0, "!E");
        return;
    }

    if (tcp_get_alarm == NULL) {
        tcp_get_alarm = alarm_new(maincpu_alarm_context, "TCPGetAlarm",
                                  tcp_get_alarm_handler, NULL);
    }
    alarm_unset(tcp_get_alarm);
    alarm_set(tcp_get_alarm, maincpu_clk + (312 * 65));
    /* no reply here, but from alarm handler */
}

static void tcp_send_alarm_handler(CLOCK offset, void *data)
{
    CURLcode res;
    size_t nsent;
    static size_t nsent_total;

    if (!curl) {
        return;                 /* connection might be closed */
    }
    alarm_set(tcp_send_alarm, maincpu_clk + (312 * 65));

    nsent = 0;
    res = curl_easy_send(curl, curl_send_buf + nsent_total,
                         curl_send_len - nsent_total, &nsent);
    nsent_total += nsent;

    if (nsent_total < curl_send_len) {
        return;
    }
    nsent_total = 0; /* reset ptr for sending */

    if (res == CURLE_OK) {
        alarm_unset(tcp_send_alarm);
        wic64_log(CONS_COL_NO, "%s: tcp sent successfully", __FUNCTION__);
        send_reply_revised(SUCCESS, "Success", NULL, 0, "0");
    } else {
        wic64_log(CONS_COL_RED, "%s: curl_easy_send: %s", __FUNCTION__, curl_easy_strerror(res));
        send_reply_revised(NETWORK_ERROR, "Failed to write TCP data", NULL, 0, "!E");
    }
}

static void cmd_tcp_write(void)
{
    hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */

    if (!curl) {
        wic64_log(CONS_COL_RED, "%s: connection lost", __FUNCTION__);
        send_reply_revised(CONNECTION_ERROR, "Can't execute TCP command", NULL, 0, "!0");
        return;
    }

    memcpy(curl_send_buf, commandbuffer, commandptr);
    curl_send_len = commandptr;

    if (tcp_send_alarm == NULL) {
        tcp_send_alarm = alarm_new(maincpu_alarm_context, "TCPSendAlarm",
                                   tcp_send_alarm_handler, NULL);
    }
    alarm_unset(tcp_send_alarm);
    alarm_set(tcp_send_alarm, maincpu_clk + (312 * 65));
    /* no reply here, but from alarm handler */
}

static void cmd_tcp_close(void)
{
    if (curl) {
        /* connection closed */
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        curl = NULL;
    }
    alarm_unset(tcp_send_alarm);
    alarm_unset(tcp_get_alarm);
    send_reply_revised(SUCCESS, "Success", NULL, 0, "0");
}

static void cmd_get_statusmsg(void)
{
    uint8_t t[40];
    int i;
    bool upcase = false;

    if (commandptr > 0) {
        hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr);
        upcase = (commandbuffer[0] > 0);
    }

    for (i = 0; wic64_last_status[i]; i++) {
        t[i] = upcase ? toupper((unsigned char)wic64_last_status[i]) : wic64_last_status[i];
        t[i] = charset_p_topetcii(t[i]);
    }
    t[i] = '\0';

    send_reply_revised(SUCCESS, "Success", t, strlen((char *)t) + 1, NULL);
}

static void cmd_force_timeout(void)
{
    wic64_timeout = commandbuffer[0];
    wic64_log(CONS_COL_NO, "forcing timeout after %ds", wic64_timeout);
    force_timeout = 1;
    send_reply_revised(SUCCESS, "Success", NULL, 0, NULL);
}

/* ---------------- legacy command -----------------*/

static void cmd_legacy(int cmd)
{

    switch (cmd) {
    case WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_03:
    case WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_05:
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_04:
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, "OK");
        break;
    case WIC64_CMD_DEPRECATED_GET_STATS_07:
    {
        char t[64];
        strncpy(t, __DATE__" " __TIME__, 64);
        charset_petconvstring((uint8_t*)t, CONVERT_TO_PETSCII);
        send_reply_revised(SUCCESS, "LEGACY",
                           (uint8_t *)t, strlen(t),
                           NULL);
        break;
    }
    case WIC64_CMD_DEPRECATED_LOG_TO_SERIAL_CONSOLE_09:
        wic64_loglevel++;
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_GET_UPD_0A:
    case WIC64_CMD_DEPRECATED_GET_UPD_DUPLICATE_1E:
        wic64_log(CONS_COL_RED, "get udp/tcp package, not implemented");
        send_reply_revised(INTERNAL_ERROR, "LEGACY", NULL, 0, "");
        break;
    case WIC64_CMD_DEPRECATED_SEND_UPD_0B:
    case WIC64_CMD_DEPRECATED_SEND_UPD_DUPLICATE_1F:
        wic64_log(CONS_COL_RED, "send udp/tcp package, not implemented");
        send_reply_revised(INTERNAL_ERROR, "LEGACY", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_SET_UPD_PORT_0E:
        hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
        wic64_udp_port = commandbuffer[0];
        wic64_udp_port += commandbuffer[1] << 8;
        wic64_log(CONS_COL_NO, "set udp port to %d", wic64_udp_port);
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_GET_EXTERNAL_IP_13:
    {
        char buffer[0x20];
        sprintf(buffer, "%d.%d.%d.%d",
                wic64_external_ip[0], wic64_external_ip[1],
                wic64_external_ip[2], wic64_external_ip[3]);
        wic64_log(CONS_COL_NO, "get external IP address, returning %s", buffer);
        send_reply_revised(SUCCESS, "LEGACY", (uint8_t *)buffer, strlen(buffer), NULL);
        break;
    }
    case WIC64_CMD_DEPRECATED_FIRMWARE_UPDATE_REQUIRED_18:
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, "0");
        break;
    case WIC64_CMD_DEPRECATED_GET_PREFERENCES_19:
    {
        char buffer[256];
        hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
        snprintf(buffer, 255, "%s", "vice");
        wic64_log(CONS_COL_RED, "read prefs not implemented, returning '%s'", buffer);
        send_reply_revised(INTERNAL_ERROR, "LEGACY", (uint8_t *) buffer, strlen(buffer), NULL);
        break;
    }
    case WIC64_CMD_DEPRECATED_SET_PREFERENCES_1A:
        wic64_log(CONS_COL_RED, "write prefs not implemented");
        send_reply_revised(INTERNAL_ERROR, "LEGACY", NULL, 0, "");
        break;
    case WIC64_CMD_DEPRECATED_SET_TCP_PORT_20:
        hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
        wic64_tcp_port = commandbuffer[0];
        wic64_tcp_port += commandbuffer[1] << 8;
        wic64_log(CONS_COL_NO, "set tcp port to %d", wic64_tcp_port);
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_BIG_LOADER_25:
        hexdump(CONS_COL_NO, (const char *)commandbuffer, commandptr); /* commands may contain '0' */
        big_load = 1;
        cmd_http_get();
        break;
    case WIC64_CMD_DEPRECATED_FACTORY_RESET_63:
        userport_wic64_reset();
        userport_wic64_factory_reset();
        send_reply_revised(SUCCESS, "LEGACY", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_LEGACY_HTTP_POST_24:
        wic64_log(CONS_COL_RED, "httppost - not implemented");
        send_reply_revised(INTERNAL_ERROR, "LEGACY", NULL, 0, "!E");
        break;
    default:
        break;
    }
}

/* ------------------------------ revised commands ---------------------*/
static void do_command(void)
{
    switch (input_command) {
    case WIC64_CMD_GET_VERSION_STRING:
    case WIC64_CMD_GET_VERSION_NUMBERS:
        cmd_get_version(input_command);
        break;
    case WIC64_CMD_HTTP_GET_ENCODED: /* send http with decoded url for PHP */
        cmd_http_get_encoded();
        /* show in log that we're entering http_get */
        wic64_log(CONS_COL_BLUE, "URL decoded, proceed with %s", cmd2string[WIC64_CMD_HTTP_GET]);
        /* fall through */
    case WIC64_CMD_HTTP_GET:
        big_load = 0;
        cmd_http_get();
        break;
    case WIC64_CMD_HTTP_POST_URL:
    case WIC64_CMD_HTTP_POST_DATA:
        cmd_http_post(input_command);
        break;
    case WIC64_CMD_SCAN_WIFI_NETWORKS:
    case WIC64_CMD_CONNECT_WITH_SSID_STRING:
    case WIC64_CMD_CONNECT_WITH_SSID_INDEX:
    case WIC64_CMD_IS_CONFIGURED:
    case WIC64_CMD_IS_CONNECTED:
    case WIC64_CMD_GET_SSID:
    case WIC64_CMD_GET_RSSI:
        cmd_wifi(input_command);
        break;
    case WIC64_CMD_GET_IP:
    case WIC64_CMD_GET_MAC:
        cmd_get_network(input_command);
        break;
    case WIC64_CMD_GET_SERVER:
    {
        const char *sv;
        resources_get_string("WIC64DefaultServer", &sv);
        wic64_log(CONS_COL_NO, "get default server '%s'", sv);
        send_reply_revised(SUCCESS, "Success", (uint8_t *)sv, strlen(sv) + 1, NULL);
        break;
    }
    case WIC64_CMD_SET_SERVER:
        wic64_log(CONS_COL_NO, "set default server '%s'", commandbuffer);
        resources_set_string("WIC64DefaultServer", (char *)commandbuffer);
        send_reply_revised(SUCCESS, "Success", NULL, 0, "0");
        break;
    case WIC64_CMD_GET_LOCAL_TIME:
        cmd_get_local_time();
        break;
    case WIC64_CMD_SET_TIMEZONE:
        cmd_set_timezone();
        break;
    case WIC64_CMD_GET_TIMEZONE:
        cmd_get_timezone();
        break;
    case WIC64_CMD_TCP_OPEN:
        cmd_tcp_open();
        break;
    case WIC64_CMD_TCP_AVAILABLE:
        cmd_tcp_available();
        break;
    case WIC64_CMD_TCP_READ:
        cmd_tcp_read();
        break;
    case WIC64_CMD_TCP_WRITE:
        cmd_tcp_write();
        break;
    case WIC64_CMD_TCP_CLOSE:
        cmd_tcp_close();
        break;
    case WIC64_CMD_GET_STATUS_MESSAGE:
        cmd_get_statusmsg();
        break;
    case WIC64_CMD_ECHO:
        send_reply_revised(SUCCESS, "Success", commandbuffer, commandptr, NULL);
        break;
    case WIC64_CMD_REBOOT:
        userport_wic64_reset();
        send_reply_revised(SUCCESS, "Success", NULL, 0, NULL);
        break;
    case WIC64_CMD_SET_TIMEOUT:
        if (commandptr < 1) {
            send_reply_revised(CLIENT_ERROR, "ESP timeout not specified", NULL, 0, NULL);
            break;
        }
        if (commandbuffer[0] == 0) {
            send_reply_revised(CLIENT_ERROR, "ESP timeout must be >= 1 second", NULL, 0, NULL);
            break;
        }
        wic64_timeout = commandbuffer[0]; /* timeout in secs */
        wic64_log(CONS_COL_NO, "setting cmd timeout to %ds", wic64_timeout);
        send_reply_revised(SUCCESS, "Success", NULL, 0, NULL);
        break;
    case WIC64_CMD_IS_HARDWARE:
        send_reply_revised(INTERNAL_ERROR, "WiC64 is emulated", NULL, 0, NULL);
        break;
    case WIC64_CMD_FORCE_ERROR:
        wic64_log(CONS_COL_RED, "forcing error...");
        send_reply_revised(INTERNAL_ERROR, "Test error", NULL, 0, NULL);
        break;
    case WIC64_CMD_FORCE_TIMEOUT:
        cmd_force_timeout();
        break;
    case WIC64_CMD_UPDATE_FIRMWARE:
        send_reply_revised(SUCCESS, "OK", NULL, 0, NULL);
        break;
    case WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_03:
    case WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_04:
    case WIC64_CMD_DEPRECATED_UPDATE_FIRMWARE_05:
    case WIC64_CMD_DEPRECATED_FIRMWARE_UPDATE_REQUIRED_18:
    case WIC64_CMD_DEPRECATED_GET_STATS_07:
    case WIC64_CMD_DEPRECATED_LOG_TO_SERIAL_CONSOLE_09:
    case WIC64_CMD_DEPRECATED_GET_UPD_0A:
    case WIC64_CMD_DEPRECATED_SEND_UPD_0B:
    case WIC64_CMD_DEPRECATED_SET_UPD_PORT_0E:
    case WIC64_CMD_DEPRECATED_GET_UPD_DUPLICATE_1E:
    case WIC64_CMD_DEPRECATED_SEND_UPD_DUPLICATE_1F:
    case WIC64_CMD_DEPRECATED_GET_EXTERNAL_IP_13:
    case WIC64_CMD_DEPRECATED_GET_PREFERENCES_19:
    case WIC64_CMD_DEPRECATED_SET_PREFERENCES_1A:
    case WIC64_CMD_DEPRECATED_SET_TCP_PORT_20:
    case WIC64_CMD_DEPRECATED_BIG_LOADER_25:
    case WIC64_CMD_DEPRECATED_FACTORY_RESET_63:
    case WIC64_CMD_DEPRECATED_LEGACY_HTTP_POST_24:
        cmd_legacy(input_command);
        break;
    default:
    {
        char it[64];
        wic64_log(CONS_COL_RED, "WiC64: unsupported command 0x%02x (len: %d/0x%x)",
                  input_command, input_length, input_length);
        input_state = 0;
        snprintf(it, 63, "Undefined command id 0x%02x", input_command);
        send_reply_revised(CLIENT_ERROR, it, NULL, 0, "!E"); /* not sure for legacy */
        break;
    }
    }
}

static void wic64_prot_state(uint8_t value)
{
    switch (input_state) {
    case INPUT_EXP_CMD:         /* command */
        input_command = value;
        if (wic64_protocol != WIC64_PROT_LEGACY) {
            input_state = INPUT_EXP_LL;
        } else {
            input_state = INPUT_EXP_ARGS;
        }
        break;
    case INPUT_EXP_LL:
        input_length = value;   /* len low byte */
        input_state = INPUT_EXP_LH;
        break;
    case INPUT_EXP_LH:          /* len high byte */
        input_length = (value << 8) | input_length;
        if (wic64_protocol == WIC64_PROT_REVISED) {
            input_state = INPUT_EXP_ARGS;
        } else if (wic64_protocol == WIC64_PROT_EXTENDED) {
            input_state = INPUT_EXP_HL;
        } else if (wic64_protocol == WIC64_PROT_LEGACY) {
            input_state = INPUT_EXP_CMD;
            input_length -= 4;
        }
        break;
    case INPUT_EXP_HL:
        input_length = (value << 16) | input_length;
        input_state = INPUT_EXP_HH;
        break;
    case INPUT_EXP_HH:
        input_length = (value << 24) | input_length;
        input_state = INPUT_EXP_ARGS;
        break;
    case INPUT_EXP_ARGS:
        if (commandptr < input_length) {
            commandbuffer[commandptr] = value;
            commandptr++;
            if (commandptr >= COMMANDBUFFER_MAXLEN) {
                wic64_log(CONS_COL_RED, "command %s exceeds maxlength, forcing timeout",
                          cmd2string[input_command]);
                hexdump(CONS_COL_BLUE, (const char *)commandbuffer, commandptr - 1);
                commandbuffer[0] = 1; /* for timeout for 1s */
                cmd_force_timeout();
                return;
            }
            commandbuffer[commandptr] = 0;
        }
        break;
    default:
        wic64_log(CONS_COL_RED, "unknown input state %d", input_state);
        break;
    }
}

/* PC2 irq (pulse) triggers when C64 reads/writes to userport */
static void userport_wic64_store_pbx(uint8_t value, int pulse)
{
    if ((pulse == 1) &&
        (!force_timeout)) {
        if (wic64_inputmode) {
            _wic64_log(CONS_COL_BLUE, 3, "receiving '%c'/0x%02x, input_state = %d",
                       isprint(value) ? value : '.',
                       value,
                       input_state);

            if (input_state == INPUT_EXP_PROT) {
                int old_prot = wic64_protocol;
                wic64_protocol = value;
                switch (value) {
                case WIC64_PROT_LEGACY:
                    input_state = INPUT_EXP_LL;
                    break;
                case WIC64_PROT_REVISED:
                case WIC64_PROT_EXTENDED:
                    input_state = INPUT_EXP_CMD;
                    break;
                default:
                    wic64_log(CONS_COL_RED, "unknown protocol '%c'/0x%02x, using revised.",
                              isprint(value) ? value: '.', value);
                    wic64_protocol = WIC64_PROT_REVISED;
                    input_state = INPUT_EXP_CMD;
                    break;
                }

                if (old_prot != wic64_protocol) {
                    wic64_log(CONS_COL_NO, "using %s protocol",
                              (wic64_protocol == WIC64_PROT_LEGACY) ? "legacy" :
                              (wic64_protocol == WIC64_PROT_REVISED) ? "revised" :
                              (wic64_protocol == WIC64_PROT_EXTENDED) ? "extended" :
                              "unknown");
                }
                input_length = 0;
                commandptr = 0;
            } else {
                wic64_prot_state(value);
            }

            cmd_timeout(1);
            handshake_flag2();
            if ((input_state == INPUT_EXP_ARGS) &&
                (commandptr == input_length)) {
                wic64_log(CONS_COL_BLUE, "command %s (len=%d/0x%x, using %s protocol)",
                          cmd2string[input_command],
                          input_length, input_length,
                          (wic64_protocol == WIC64_PROT_LEGACY) ? "legacy" :
                          (wic64_protocol == WIC64_PROT_REVISED) ? "revised" :
                          (wic64_protocol == WIC64_PROT_EXTENDED) ? "extended" :
                          "unknown");
                cmd_timeout(0);
                do_command();
                commandptr = input_state = input_length = 0;
                input_command = WIC64_CMD_NONE;
                memset(commandbuffer, 0, COMMANDBUFFER_MAXLEN);
            }
        } else {
            if (reply_length) {
                reply_next_byte();
            }
            handshake_flag2();
        }
    }
}

static uint8_t userport_wic64_read_pbx(uint8_t orig)
{
    uint8_t retval = reply_port_value;
    /* FIXME: what do we have to do with original value? */
    /* CIA read is triggered once more by wic64 lib on the host,
       even if all bytes are sent, so the last byte seems to be sent twice */

    _wic64_log(CONS_COL_GREEN, 3, "sending '%c'/0x%02x - ptr = %d, rl = %d/0x%x",
               isprint(retval) ? retval : '.',
               retval, replyptr, reply_length, reply_length);
    cmd_timeout(0);
    /* FIXME: trigger mainloop */
    return retval;
}

/* PA2 interrupt toggles input/output mode */
static void userport_wic64_store_pa2(uint8_t value)
{
    _wic64_log(CONS_COL_NO, 2, "userport mode %s...(len = %d)",
               value ? "sending" : "receiving",
               reply_length);

    if ((wic64_inputmode == 1) &&
        (value == 0) &&
        (reply_length)) {
        _wic64_log(CONS_COL_NO, 2, "userport_wic64_store_pa2 val:%02x (host %s - rl = %u)",
                   value, value ? "sends" : "receives", reply_length);
        handshake_flag2();
    }
    wic64_inputmode = value;
    if (wic64_inputmode == 1) {
        if ((reply_length > 0) &&
            (replyptr > 0)) {
            replyptr--;         /* rewind by 1 byte */
            wic64_log(CONS_COL_RED, "discarding %d bytes, which were not sent to host",
                      (reply_length - replyptr));
            hexdump(CONS_COL_RED, &replybuffer[replyptr], (reply_length - replyptr));
        }
        replyptr = reply_length = 0; /* host decided to send, truncate outputbuffer */
    }
}

static void userport_wic64_reset(void)
{
    char *tmp;
    int tmp_tz;

    wic64_log(CONS_COL_NO, "%s", __FUNCTION__);
    commandptr = input_state = input_length = 0;
    input_command = WIC64_CMD_NONE;
    wic64_inputmode = 1;
    memset(sec_token, 0, 32);
    sec_init = 0;

    if ((resources_get_string("WIC64MACAddress", (const char **)&tmp) == -1) ||
        (tmp == NULL) ||
        (strcmp((const char*)tmp, "DEADBE") == 0)) {
        wic64_mac_address = lib_malloc(32);
        snprintf(wic64_mac_address, 32, "08:d1:f9:%02x:%02x:%02x",
                 lib_unsigned_rand(0, 15),
                 lib_unsigned_rand(0, 15),
                 lib_unsigned_rand(0, 15));
        _wic64_log(CONS_COL_NO, 2, "WIC64: generated MAC: %s", wic64_mac_address);
    } else {
        wic64_mac_address = tmp;
    }

    if ((resources_get_string("WIC64IPAddress", (const char **)&tmp) == -1) ||
        (tmp == NULL) ||
        (strcmp((const char *)tmp, "AAAA") == 0)) {
        wic64_internal_ip = lib_malloc(16);
        snprintf(wic64_internal_ip, 16, "192.168.%u.%u",
                 lib_unsigned_rand(1, 254),
                 lib_unsigned_rand(1, 254));
        _wic64_log(CONS_COL_NO, 2, "WIC64: generated internal IP: %s", wic64_internal_ip);
    } else {
        wic64_internal_ip = tmp;
    }

    if (resources_get_int("WIC64Timezone", &tmp_tz) == -1) {
        current_tz = 2;
    } else {
        current_tz = tmp_tz;
    }

    if (http_get_alarm) {
        alarm_unset(http_get_alarm);
    }
    if (http_post_alarm) {
        alarm_unset(http_post_alarm);
    }
    if (http_post_endalarm) {
        alarm_unset(http_post_endalarm);
    }
    if (tcp_get_alarm) {
        alarm_unset(tcp_get_alarm);
    }
    if (tcp_send_alarm) {
        alarm_unset(tcp_send_alarm);
    }
    if (cmd_timeout_alarm) {
        alarm_unset(cmd_timeout_alarm);
    }
    if (curl) {
        /* connection closed */
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        curl = NULL;
    }
    curl_global_init(CURL_GLOBAL_ALL);
    if (post_url) {
        lib_free(post_url);
        post_url = NULL;
    }
    wic64_set_status("RESET");
    if (wic64_colorize_log) {
        wic64_log(CONS_COL_BLUE, "cyan color: host -> WiC64 communication");
        wic64_log(CONS_COL_GREEN, "green color: WiC64 -> host communication");
        wic64_log(CONS_COL_RED, "red color: some error");
        wic64_log(CONS_COL_NO, "no color: other information");
    }
}

/* ---------------------------------------------------------------------*/

/* USERPORT_WIC64 snapshot module format:

   type  | name           | description
   -----------------------------
   BYTE  | input_state    |
   BYTE  | input_length   |
*/

static char snap_module_name[] = "UPWIC64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int userport_wic64_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, input_state) < 0)
        || (SMW_B(m, input_command) < 0)) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_wic64_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_B(m, &input_state) < 0)
        || (SMR_B(m, &input_command) < 0)) {
        goto fail;
    }
    return snapshot_module_close(m);

  fail:
    snapshot_module_close(m);
    return -1;
}

#endif /* WIC64 */

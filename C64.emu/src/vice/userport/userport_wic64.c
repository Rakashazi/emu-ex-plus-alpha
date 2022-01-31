/*
 * userport_wic64.c - Userport WiC64 wifi interface emulation.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#define DEBUG_WIC64

/* all http get requests will be served with hardcoded replies or files from the CWD */
/* #define DEBUG_DUMMY_HTTPGET */

/* - WiC64 (C64/C128)

C64/C128   |  I/O
-----------------
 C (PB0)   |  I/O   databits from/to C64
 D (PB1)   |  I/O
 E (PB2)   |  I/O
 F (PB3)   |  I/O
 H (PB4)   |  I/O
 J (PB5)   |  I/O
 K (PB6)   |  I/O
 L (PB7)   |  I/O
 8 (PC2)   |  O     C64 triggers PC2 IRQ whenever data is read or write
 M (PA2)   |  O     Low=device sends data High=C64 sends data (powerup=high)
 B (FLAG2) |  I     device asserts high->low transition when databyte sent to c64 is ready (triggers irq)

 enable the device and start https://www.wic64.de/wp-content/uploads/2021/10/wic64start.zip

 for more info see https://www.wic64.de
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "resources.h"
#include "joyport.h"
#include "joystick.h"
#include "snapshot.h"
#include "userport.h"
#include "userport_wic64.h"
#include "machine.h"
#include "uiapi.h"

#include "log.h"

#ifdef DEBUG_WIC64
#define DBG(x) log_debug x
#else
#define DBG(x)
#endif

#ifdef USERPORT_EXPERIMENTAL_DEVICES

static int userport_wic64_enabled = 0;

/* Some prototypes are needed */
static uint8_t userport_wic64_read_pbx(uint8_t orig);
static void userport_wic64_store_pbx(uint8_t val, int pulse);
static void userport_wic64_store_pa2(uint8_t value);
static int userport_wic64_write_snapshot_module(snapshot_t *s);
static int userport_wic64_read_snapshot_module(snapshot_t *s);
static int userport_wic64_enable(int value);
static void userport_wic64_reset(void);

static userport_device_t userport_wic64_device = {
    "Userport WiC64",                     /* device name */
    JOYSTICK_ADAPTER_ID_NONE,             /* this is NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_WIFI,            /* device is a joystick adapter */
    userport_wic64_enable,                /* enable function */
    userport_wic64_read_pbx,              /* read pb0-pb7 function */
    userport_wic64_store_pbx,             /* store pb0-pb7 function */
    NULL,                                 /* NO read pa2 pin function */
    userport_wic64_store_pa2,             /* store pa2 pin function */
    NULL,                                 /* NO read pa3 pin function */
    NULL,                                 /* NO store pa3 pin function */
    1,                                    /* pc pin IS needed */
    NULL,                                 /* NO store sp1 pin function */
    NULL,                                 /* NO read sp1 pin function */
    NULL,                                 /* NO store sp2 pin function */
    NULL,                                 /* NO read sp2 pin function */
    userport_wic64_reset,                 /* reset function */
    NULL,                                 /* NO powerup function */
    userport_wic64_write_snapshot_module, /* snapshot write function */
    userport_wic64_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_wic64_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_wic64_enabled == val) {
        return 0;
    }

    userport_wic64_enabled = val;
    return 0;
}

int userport_wic64_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_WIC64, &userport_wic64_device);
}

/* ---------------------------------------------------------------------*/

#define HTTPREPLY_MAXLEN    (0x18000)

/* https://cirosantilli.com/linux-kernel-module-cheat#socket */

#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

uint8_t httpbuffer[HTTPREPLY_MAXLEN];

/* FIXME: replace this shit by proper network code (libcurl) */
static int do_http_get(char *hostname, char *path, unsigned short server_port) {
    char *p;
    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n";
    struct protoent *protoent;
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last, nbytes_read;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, path, hostname);
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length large: %d\n", request_len);
        return -1;
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (protoent == NULL) {
        perror("getprotobyname");
        return -1;
    }
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (socket_file_descriptor == -1) {
        perror("socket");
        return -1;
    }

    /* Build the address. */
    hostent = gethostbyname(hostname);
    if (hostent == NULL) {
        fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname);
        return -1;
    }
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1) {
        fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        return -1;
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    /* Actually connect. */
    if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect");
        return -1;
    }

    /* Send HTTP request. */
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        if (nbytes_last == -1) {
            perror("write");
            return -1;
        }
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    fprintf(stderr, "debug: before first read\n");
    nbytes_read = 0;
    p = (char*)httpbuffer;
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        fprintf(stderr, "debug: after a read\n");
        memcpy(p, buffer, nbytes_total);
        p += nbytes_total;
        nbytes_read += nbytes_total;
    }
    fprintf(stderr, "debug: after last read\n");
    if (nbytes_total == -1) {
        perror("read");
        return -1;
    }

    close(socket_file_descriptor);

    /* check http response code, return if error */
    if (strncmp((char*)httpbuffer, "HTTP/1.1 200 OK", 15) != 0) {
        perror("http error");
        return -1;
    }

    /* find content length */
    p = strstr((char*)httpbuffer, "Content-Length:");
    if (p == NULL) {
        return -1;
    }
    p += 15; /* skip "Content-Length:" */
    nbytes_read = strtoul(p, NULL, 10);
    DBG(("http content len: %ld", nbytes_read));

    /* find content */
    p = strstr((char*)httpbuffer, "Content-Type:");
    if (p == NULL) {
        return -1;
    }
    /* skip to end of line (content description) */
    while ((*p != '\n') && (*p != '\r')) {
        p++;
    }
    /* skip line ending(s) */
    while ((*p == '\n') || (*p == '\r')) {
        p++;
    }

    /* copy payload to buffer */
    memmove(httpbuffer, p, nbytes_read);

    return nbytes_read;
}

/* ---------------------------------------------------------------------*/
#define WLAN_SSID   "VICE WiC64 emulation"
#define WLAN_RSSI   "123"
static unsigned char wic64_mac_address[6] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
static unsigned char wic64_internal_ip[4] = { 192, 168, 0, 1 };
static unsigned char wic64_external_ip[4] = { 204, 234, 1, 4 };
static uint8_t wic64_timezone[2] = { 0, 0};
static uint16_t wic64_udp_port = 0;
static uint16_t wic64_tcp_port = 0;

char default_server_hostname[0x100];

#define COMMANDBUFFER_MAXLEN    0x1000

#define FLAG2_ACTIVE    1
#define FLAG2_INACTIVE  0

uint8_t input_state = 0, input_command = 0;
uint8_t wic64_ddr = 1;
uint16_t input_length = 0, commandptr = 0;
uint8_t commandbuffer[COMMANDBUFFER_MAXLEN];

uint8_t replybuffer[0x10010];
uint16_t replyptr = 0, reply_length = 0;
uint8_t reply_port_value = 0;

static void handshake_flag2(void)
{
    set_userport_flag(FLAG2_ACTIVE);
    set_userport_flag(FLAG2_INACTIVE);
}

static void reply_next_byte(void)
{
    if (replyptr <= reply_length) {
        reply_port_value = replybuffer[replyptr];
        /* DBG(("reply_next_byte: %3d/%3d - %02x", replyptr, reply_length, reply_port_value)); */
        replyptr++;
    } else {
        reply_length = 0;
    }
}

static void send_reply(char * reply)
{
    int len;
    len = strlen(reply);
    /* highbyte first! */
    replybuffer[1] = len & 0xff;
    replybuffer[0] = (len >> 8) & 0xff;
    strcpy((char*)replybuffer + 2, reply);
    reply_length = len + 2;
    replyptr = 0;
}

static void send_binary_reply(uint8_t *reply, int len)
{
    /* highbyte first! */
    replybuffer[1] = len & 0xff;
    replybuffer[0] = (len >> 8) & 0xff;
    memcpy((char*)replybuffer + 2, reply, len);
    reply_length = len + 2;
    replyptr = 0;
}

/* http get */
#ifdef DEBUG_DUMMY_HTTPGET
static void dummy_send_file(char *name)
{
    unsigned char buffer[0x10000];
    FILE *f = fopen(name, "rb");
    int len;
    if (f) {
        len = fread(buffer, 1, 0x10000, f);
        fclose(f);
        DBG(("sending start.prg (%d bytes)", len));
        send_binary_reply(buffer, len);
        return;
    }
    send_reply("!0");   /* error */
}
#endif

static void do_command_01_0f(int encode)
{
    DBG(("command 01: '%s'", commandbuffer));
#ifdef DEBUG_DUMMY_HTTPGET
    if(!strcmp(commandbuffer, "http://www.wic64.de/prg/readme.txt")) {
        send_reply("start-this is a test-end");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/um.php?F=I&1=%mac")) {
        send_reply("vicetest");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/rtc.php")) {
        send_reply("003145");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/wchat.php?f=l&1=%mac")) {
        send_reply("0vicetest");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/wchat.php?f=e&1=%mac&2=vicetest")) {
        send_reply("1");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/start.prg")) {
        dummy_send_file("start.prg");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/files/giana.prg")) {
        dummy_send_file("giana.prg");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/files/koalashow.prg")) {
        dummy_send_file("koalashow.prg");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/files/clock.prg")) {
        dummy_send_file("clock.prg");
        return;
    } else if(!strcmp(commandbuffer, "http://x.wic64.net/wichat.prg")) {
        dummy_send_file("wichat.prg");
        return;
    } else if(!strcmp(commandbuffer, "http://www.wic64.de/koa/00.kob")) {
        dummy_send_file("00.kob");
        return;
    } else if(!strcmp(commandbuffer, "http://www.wic64.de/koa/01.kob")) {
        dummy_send_file("01.kob");
        return;
    } else if(!strcmp(commandbuffer, "http://www.wic64.de/koa/02.kob")) {
        dummy_send_file("02.kob");
        return;
    } else if(!strcmp(commandbuffer, "http://www.wic64.de/koa/03.kob")) {
        dummy_send_file("03.kob");
        return;
    }
    send_reply("!0");   /* error */
#else
    char *p, *cptr;
    int port = 80;
    char hostname[COMMANDBUFFER_MAXLEN];
    char path[COMMANDBUFFER_MAXLEN];
    char temppath[COMMANDBUFFER_MAXLEN];
    int gotlen;
    DBG(("command 01: '%s'", commandbuffer));

    /* if encode is enabled, there might be binary data after <$, which is
       then encoded as a stream of hex digits */
    if (encode) {
        cptr = (char*)commandbuffer;
        p = strstr(cptr, "<$");
        if (p != NULL) {
            static char hextab[16] = "0123456789abcdef";
            int encodedlen, encodeoffset, i;
            encodeoffset = p - cptr;
            DBG(("escape sequence found in commandbuffer, offset %d", encodeoffset));
            /* copy string before <$ */
            strncpy(temppath, cptr, encodeoffset);
            temppath[encodeoffset] = 0;
            DBG(("temppath:%s", temppath));
            /* copy encoded string */
            encodedlen = p[2];
            encodedlen += p[3] << 8;
            DBG(("encodedlen:%d", encodedlen));
            p += 4; /* skip escape sequence and len */
#ifdef DEBUG_WIC64
            for (i = 0; i < encodedlen; i++) {
                printf("%02x ", (unsigned int)p[i]);
            }
            printf("\n");
#endif
            for (i = 0; i < encodedlen; i++) {
                temppath[encodeoffset] = hextab[(*p >> 4) & 0xf];
                encodeoffset++;
                temppath[encodeoffset] = hextab[*p & 0xf];
                encodeoffset++;
                p++;
            }
            temppath[encodeoffset] = 0;
            DBG(("temppath:%s", temppath));

#if 0   /* do w need to do this? */
            /* copy string after <$ */
            strcat(temppath, p);
            DBG(("temppath:%s", temppath));
#endif
            /* copy back to commandbuffer buffer */
            strcpy((char*)commandbuffer, temppath);
            DBG(("encoded path:%s", commandbuffer));
        }
    }

    /* if url begins with !, replace by default server */
    if (commandbuffer[0] == '!') {
        DBG(("URL starts with !, default server is: %s", default_server_hostname));
        p = temppath;
        /* add the default server address */
        strcpy(p, default_server_hostname);
        p += strlen(default_server_hostname);
        DBG(("temppath:%s", temppath));
        /* copy command buffer */
        memcpy(p, commandbuffer + 1, COMMANDBUFFER_MAXLEN - strlen(default_server_hostname));
        DBG(("temppath:%s", temppath));
        /* copy back to commandbuffer buffer */
        memcpy(commandbuffer, temppath, COMMANDBUFFER_MAXLEN);
        DBG(("temppath:%s", temppath));
    }

    /* detect protocol and split path/hostname */
    p = (char*)commandbuffer;
    if (!strncmp(p, "http://", 7)) {
        DBG(("type:http"));
        p += 7;
        p = strtok(p, "/");
        DBG(("host:%s", p));
        strcpy(hostname, p);
        p = (char*)commandbuffer;
        p += (7 + 1);
        p += strlen(hostname);
        DBG(("path:%s", p));
        memcpy(path, p, COMMANDBUFFER_MAXLEN - (p - (char*)commandbuffer));
    } else {
        DBG(("malformed URL:%s", commandbuffer));
        return;
    }
    DBG(("host:%s", hostname));
    DBG(("path:%s", path));

    /* replace "%mac" by our MAC */
    p = strstr(path, "%mac");
    if (p != NULL) {
        char macstring[0x20];
        /* copy string before %mac */
        strncpy(temppath, path, p - path);
        temppath[p - path] = 0;
        DBG(("temppath:%s", temppath));
        /* add the MAC address */
        sprintf(macstring, "%02x%02x%02x%02x%02x%02x",
                wic64_mac_address[0], wic64_mac_address[1], wic64_mac_address[2], 
                wic64_mac_address[3], wic64_mac_address[4], wic64_mac_address[5]);
        DBG(("temppath:%s", temppath));
        strcat(temppath, macstring);
        /* copy string after %mac */
        DBG(("temppath:%s", temppath));
        strcat(temppath, p + 4);
        /* copy back to path buffer */
        DBG(("temppath:%s", temppath));
        strcpy(path, temppath);
    }
    /* replace "%ser" by the default server */
    p = strstr(path, "%ser");
    if (p != NULL) {
        /* copy string before %ser */
        strncpy(temppath, path, p - path);
        temppath[p - path] = 0;
        DBG(("temppath:%s", temppath));
        /* add the default server address */
        strcat(temppath, default_server_hostname);
        DBG(("temppath:%s", temppath));
        /* copy string after %ser */
        strcat(temppath, p + 4);
        DBG(("temppath:%s", temppath));
        /* copy back to path buffer */
        strcpy(path, temppath);
        DBG(("temppath:%s", temppath));
    }

    DBG(("path:%s", path));

    gotlen = do_http_get(hostname, path, port);
    DBG(("got %d bytes", gotlen));
    if (gotlen > 0) {
        send_binary_reply(httpbuffer, gotlen);
    }
#endif
}

/* set wlan ssid + password */
static void do_command_02(void)
{
    DBG(("command 02: '%s'", commandbuffer));
    send_reply("Wlan config changed");
}

/* get wic64 ip address */
static void do_command_06(void)
{
    char buffer[0x20];
    /* FIXME: update the internal IP */
    sprintf(buffer, "%d.%d.%d.%d", wic64_internal_ip[0], wic64_internal_ip[1],
            wic64_internal_ip[2], wic64_internal_ip[3]);
    send_reply(buffer);
}

/* get firmware stats */
static void do_command_07(void)
{
    send_reply(__DATE__ " " __TIME__);
}

/* set default server */
static void do_command_08(void)
{
    DBG(("command 08: '%s'", commandbuffer));
    strcpy(default_server_hostname, (char*)commandbuffer);
    /* this command sends no reply */
}

/* prints output to serial console */
static void do_command_09(void)
{
    DBG(("command 09: '%s'", commandbuffer));
    fprintf(stdout, "%s", commandbuffer);
    /* this command sends no reply */
}

/* get udp package */
static void do_command_0a(void)
{
    DBG(("command 0a: '%s'", commandbuffer));
    /* FIXME: not implemented */
    DBG(("get UDP not implemented"));
}

/* send udp package */
static void do_command_0b(void)
{
    DBG(("command 0b: '%s'", commandbuffer));
    /* FIXME: not implemented */
    DBG(("send UDP not implemented"));
    /* this command sends no reply */
}

/* get list of all detected wlan ssids */
static void do_command_0c(void)
{
    /* index, sep, ssid, sep, rssi, sep */
    send_reply("0\001vice\0011234");
}

/* set wlan via scan id */
static void do_command_0d(void)
{
    DBG(("command 0d: '%s'", commandbuffer));
    send_reply("Wlan config changed");
}

/* set udp port */
static void do_command_0e(void)
{
    wic64_udp_port = commandbuffer[0];
    wic64_udp_port += commandbuffer[1] << 8;
    DBG(("set udp port: %d", wic64_udp_port));
    /* this command sends no reply */
}

/* get connected wlan name */
static void do_command_10(void)
{
    send_reply(WLAN_SSID);
}

/* get wlan rssi signal level */
static void do_command_11(void)
{
    send_reply(WLAN_RSSI);
}

/* get default server */
static void do_command_12(void)
{
    send_reply(default_server_hostname);
}

/* get external ip address */
static void do_command_13(void)
{
    char buffer[0x20];
    /* FIXME: update the external IP */
    sprintf(buffer, "%d.%d.%d.%d", wic64_external_ip[0], wic64_external_ip[1],
            wic64_external_ip[2], wic64_external_ip[3]);
    send_reply(buffer);
}

/* get wic64 MAC address */
static void do_command_14(void)
{
    char buffer[0x20];
    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
            wic64_mac_address[0], wic64_mac_address[1], wic64_mac_address[2], 
            wic64_mac_address[3], wic64_mac_address[4], wic64_mac_address[5]);
    send_reply(buffer);
}

/* get timezone+time */
static void do_command_15(void)
{
    /* FIXME: should also send time+date (in what format?) */
    send_binary_reply(wic64_timezone, 2);
}

/* set timezone */
static void do_command_16(void)
{
    DBG(("set timezone: '%02x %02x'", commandbuffer[0], commandbuffer[1]));
    wic64_timezone[0] = commandbuffer[0];
    wic64_timezone[1] = commandbuffer[1];
    /* FIXME: send a reply or not? */
}

/* get tcp */
static void do_command_1e(void)
{
    DBG(("command 1e: '%s'", commandbuffer));
    /* FIXME: not implemented */
    DBG(("get TCP not implemented"));
}

/* send tcp */
static void do_command_1f(void)
{
    DBG(("command 1f: '%s'", commandbuffer));
    /* FIXME: not implemented */
    DBG(("send TCP not implemented"));
    /* this command sends no reply */
}

/* set tcp port */
static void do_command_20(void)
{
    wic64_tcp_port = commandbuffer[0];
    wic64_tcp_port += commandbuffer[1] << 8;
    DBG(("set tcp port: %d", wic64_udp_port));
}

/* factory reset */
static void do_command_63(void)
{
    /* TODO: reset resources and other stuff here */
    /* this command sends no reply */
}

static void do_command(void)
{
    switch (input_command) {
        case 0x01: /* http get */
            DBG(("command 01: http get"));
            do_command_01_0f(0);
            break;
        case 0x02: /* set wlan ssid + password */
            DBG(("command 02: set wlan ssid + password"));
            do_command_02();
            break;
        case 0x03: /* standard firmware update */
            DBG(("command 03: standard firmware update"));
            /* this command sends no reply */
            break;
        case 0x04: /* developer firmware update */
            DBG(("command 04: developer firmware update"));
            /* this command sends no reply */
            break;
        case 0x05: /* developer special update */
            DBG(("command 05: developer special update"));
            /* this command sends no reply */
            break;
        case 0x06: /* get wic64 ip address */
            DBG(("command 06: get wic64 ip address"));
            do_command_06();
            break;
        case 0x07: /* get firmware stats */
            DBG(("command 07: get firmware stats"));
            do_command_07();
            break;
        case 0x08: /* set default server */
            do_command_08();
            break;
        case 0x09: /* prints output to serial console */
            DBG(("command 09: prints output to serial console"));
            do_command_09();
            break;
        case 0x0a: /* get udp package */
            DBG(("command 0a: get UDP package"));
            do_command_0a();
            break;
        case 0x0b: /* send udp package */
            DBG(("command 0b: send UDP package"));
            do_command_0b();
            break;
        case 0x0c: /* get list of all detected wlan ssids */
            DBG(("command 0c: get list of all detected wlan ssids"));
            do_command_0c();
            break;
        case 0x0d: /* set wlan via scan id */
            DBG(("command 0d: set wlan via scan id"));
            do_command_0d();
            break;
        case 0x0e: /* set udp port */
            DBG(("command 0e: set udp port"));
            do_command_0e();
            break;
        case 0x0f: /* send http with decoded url for PHP */
            DBG(("command 0f: send http with decoded url for PHP"));
            do_command_01_0f(1);
            break;
        case 0x10: /* get connected wlan name */
            DBG(("command 10: get connected wlan name"));
            do_command_10();
            break;
        case 0x11: /* get wlan rssi signal level */
            DBG(("command 11: get wlan rssi signal level"));
            do_command_11();
            break;
        case 0x12: /* get default server */
            DBG(("command 12: get default server"));
            do_command_12();
            break;
        case 0x13: /* get external ip address */
            DBG(("command 13: get external ip address"));
            do_command_13();
            break;
        case 0x14: /* get wic64 MAC address */
            DBG(("command 14: get wic64 MAC address"));
            do_command_14();
            break;
        case 0x15: /* get timezone+time */
            DBG(("command 15: get timezone+time"));
            do_command_15();
            break;
        case 0x16: /* set timezones */
            DBG(("command 16: set timezone"));
            do_command_16();
            break;
        case 0x1e: /* get tcp */
            DBG(("command 1e: get tcp"));
            do_command_1e();
            break;
        case 0x1f: /* send tcp */
            DBG(("command 1f: send tcp"));
            do_command_1f();
            break;
        case 0x20: /* set tcp port */
            DBG(("command 20: set tcp port"));
            do_command_20();
            break;
        case 0x63: /* factory reset */
            DBG(("command 63: factory reset"));
            do_command_63();
            break;
        default:
            log_error(LOG_DEFAULT, "WiC64: unsupported command 0x%02x (len: %d)", input_command, input_length);
            input_state = 0;
        break;
    }
}

static void userport_wic64_store_pbx(uint8_t value, int pulse)
{
#if 0
    DBG(("userport_wic64_store_pbx val:%02x pulse:%d", value, pulse));
#endif
    if (reply_length) {
        if (wic64_ddr == 0) {
            reply_next_byte();
        }
    } else {

        if (pulse) {
            switch (input_state) {
                case 0:
                    input_length = 0;
                    commandptr = 0;
                    if (value == 0x57) {    /* 'w' */
                        input_state++;
                    }
                break;
                case 1: /* lenght low byte */
                    input_length = value;
                    input_state++;
                break;
                case 2: /* lenght high byte */
                    input_length |= (value << 8);
                    input_state++;
                break;
                case 3: /* command */
                    input_command = value;
                    input_state++;
                break;
                default:    /* additional data depending on command */
                    if ((commandptr + 4) < input_length) {
                        commandbuffer[commandptr] = value;
                        commandptr++;
                        commandbuffer[commandptr] = 0;
                    }
                break;
            }
#if 0
            DBG(("input_state: %d input_length: %d input_command: %02x commandptr: %02x",
                input_state, input_length, input_command, commandptr));
#endif
            if ((input_state == 4) && ((commandptr + 4) >= input_length)) {
                do_command();
                commandptr = input_command = input_state = input_length = 0;
            }

        }
    }

    handshake_flag2();
}

static uint8_t userport_wic64_read_pbx(uint8_t orig)
{
    uint8_t retval = reply_port_value;
    /* FIXME: what do we have to do with original value? */
    /* DBG(("userport_wic64_read_pbx orig:%02x retval: %02x", orig, retval)); */
    return retval;
}

static void userport_wic64_store_pa2(uint8_t value)
{
    /* DBG(("userport_wic64_store_pa2 val:%02x", value)); */
    wic64_ddr = value;
}

static void userport_wic64_reset(void)
{
    /* DBG(("userport_wic64_reset")); */
    commandptr = input_command = input_state = input_length = 0;
    wic64_ddr = 1;
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

#endif /* USERPORT_EXPERIMENTAL_DEVICES */


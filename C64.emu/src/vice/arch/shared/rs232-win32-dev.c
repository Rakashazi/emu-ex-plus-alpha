/** \file   rs232-win32-dev.c
 * \brief   RS232 Device emulation
 *
 * \author  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
 * \author  groepaz <groepaz@gmx.net>
 * \author  Greg King <gregdk@users.sf.net>
 *
 * The RS232 emulation captures the bytes sent to the RS232 interfaces
 * available (currently, ACIA 6551, std. user port,
 * and Daniel Dallmann's fast RS232 with 9600 BPS).
 *
 * I/O is done usually to a physical COM port.
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
 */

#include "vice.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <winsock.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "cmdline.h"
#include "coproc.h"
#include "log.h"
#include "resources.h"
#include "rs232.h"
#include "rs232dev.h"
#include "types.h"
#include "util.h"

#define COPROC_SUPPORT

#define DEBUGRS232

#ifdef DEBUGRS232
# define DEBUG_LOG_MESSAGE(_xxx) log_message _xxx
#else
# define DEBUG_LOG_MESSAGE(_xxx)
#endif

/* ------------------------------------------------------------------------- */

/* resource handling */

/* baudrate of physical RS232 device (unused with pipes and sockets) */
static int devbaud[RS232_NUM_DEVICES];

static int is_baud_valid(int baud)
{
    switch (baud) {
        case     110:
        case     300:
        case     600:
        case    1200:
        case    2400:
        case    4800:
        case    9600:
        case   19200:
        case   38400:
        case   57600:
        case  115200:
        case  128000:
        case  256000:
#if 0
        case  500000:
        case 1000000:
#endif
                return 1;
            break;
        default:
            break;
    }
    return 0;
}

static int set_devbaud(int val, void *param)
{
    if (is_baud_valid(val)) {
        devbaud[vice_ptr_to_int(param)] = val;
    }
    return 0;
}

static int get_devbaud(int dev)
{
    switch (devbaud[dev]) {
        case     110: return CBR_110;
        case     300: return CBR_300;
        case     600: return CBR_600;
        case    1200: return CBR_1200;
        case    2400: return CBR_2400;
        case    4800: return CBR_4800;
        case    9600: return CBR_9600;
        case   19200: return CBR_19200;
        case   38400: return CBR_38400;
        case   57600: return CBR_57600;
        case  115200: return CBR_115200;
        case  128000: return CBR_128000;
        case  256000: return CBR_256000;
#if 0
        case  500000: return CBR_500000;
        case 1000000: return CBR_1000000;
#endif
    }

    return 0; /* invalid / not set */
}

/* ------------------------------------------------------------------------- */

static const resource_int_t resources_int[] = {
    { "RsDevice1Baud", 2400, RES_EVENT_NO, NULL,
      &devbaud[0], set_devbaud, (void *)0 },
    { "RsDevice2Baud", 38400, RES_EVENT_NO, NULL,
      &devbaud[1], set_devbaud, (void *)1 },
    { "RsDevice3Baud", 2400, RES_EVENT_NO, NULL,
      &devbaud[2], set_devbaud, (void *)2 },
    { "RsDevice4Baud", 38400, RES_EVENT_NO, NULL,
      &devbaud[3], set_devbaud, (void *)3 },
    RESOURCE_INT_LIST_END
};

int rs232dev_resources_init(void)
{
    return resources_register_int(resources_int);
}

void rs232dev_resources_shutdown(void)
{
}

static const cmdline_option_t cmdline_options[] =
{
    { "-rsdev1baud", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_NEED_BRACKETS,
      NULL, NULL, "RsDevice1Baud", NULL,
      "<baudrate>", "Specify baudrate of first RS232 device" },
    { "-rsdev2baud", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_NEED_BRACKETS,
      NULL, NULL, "RsDevice2Baud", NULL,
      "<baudrate>", "Specify baudrate of second RS232 device" },
    { "-rsdev3baud", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_NEED_BRACKETS,
      NULL, NULL, "RsDevice3Baud", NULL,
      "<baudrate>", "Specify baudrate of third RS232 device" },
    { "-rsdev4baud", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_NEED_BRACKETS,
      NULL, NULL, "RsDevice4Baud", NULL,
      "<baudrate>", "Specify baudrate of fourth RS232 device" },
    CMDLINE_LIST_END
};

int rs232dev_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/** \brief  descriptor for a rs232 interface/port
 */
typedef struct rs232dev {
    int inuse;          /**< flag to mark descriptor in-use */
    int type;           /**< type of connection, T_TTY, T_FILE, T_PROC */
    HANDLE fd;          /**< Windows file handle for the open RS232 port */
    HANDLE fd_r;        /**< stdout read pipe from external process */
    HANDLE fd_w;        /**< stdin write pipe from external process */
    /* char *file; */
    DCB restore_dcb;    /**< status of the serial port before using it, for later restoration */
    int rts;            /**< current status of the serial port's RTS line */
    int dtr;            /**< current status of the serial port's DTR line */
    vice_pid_t pid;     /**< process id if type = T_PROC */
} rs232dev_t;

/* type of open connection */
#define T_TTY  0
#define T_PROC 1
#define T_FILE 2

static rs232dev_t fds[RS232_NUM_DEVICES];

static log_t rs232dev_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

void rs232dev_close(int fd);

/* initializes all RS232 stuff */
void rs232dev_init(void)
{
    rs232dev_log = log_open("RS232dev");
}

/* reset RS232 stuff */
void rs232dev_reset(void)
{
    int i;

    for (i = 0; i < RS232_NUM_DEVICES; i++) {
        if (fds[i].inuse) {
            rs232dev_close(i);
        }
    }
}

/* opens a rs232 window, returns handle to give to functions below. */
int rs232dev_open(int device)
{
    HANDLE serial_port = INVALID_HANDLE_VALUE;
    int ret = -1;
    int i;

    for (i = 0; i < RS232_NUM_DEVICES; i++) {
        if (!fds[i].inuse) {
            break;
        }
    }
    if (i >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "rs232dev_open(): No more devices available.");
        return -1;
    }

    DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(device %d), use fds[%d].", device, i));

    memset(&fds[i], 0, sizeof fds[0]);

    fds[i].pid = NULL;
#ifdef COPROC_SUPPORT
    if (rs232_devfile[device][0] == '|') {
        int fd_r, fd_w;
        vice_pid_t pid;
        log_message(rs232dev_log, "rs232dev_open(): forking '%s'", rs232_devfile[device] + 1);
        if (fork_coproc(&fd_w, &fd_r, rs232_devfile[device] + 1, &pid) < 0) {
            log_error(rs232dev_log, "Cannot fork process '%s'.", rs232_devfile[device] + 1);
            return -1;
        }
        fds[i].fd_w = (HANDLE)_get_osfhandle(fd_w);
        fds[i].fd_r = (HANDLE)_get_osfhandle(fd_r);
        fds[i].type = T_PROC;
        fds[i].inuse = 1;
        fds[i].pid = pid;
        /* fds[i].file = rs232_devfile[device]; */
        ret = i;
    } else
#endif
    do {
        DCB dcb;
        COMMTIMEOUTS comm_timeouts;
        char *mode_string = strchr(rs232_devfile[device], ':');

        if (mode_string != NULL) {
            *mode_string = 0;
        }

        DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): CreateFile(%s).", rs232_devfile[device]));
        serial_port = CreateFile(rs232_devfile[device], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (mode_string != NULL) {
            *mode_string = ':';
        }


        if (serial_port == INVALID_HANDLE_VALUE) {
            DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): CreateFile('%s') failed: %lu.", rs232_devfile[device], GetLastError()));
            break;
        }

        /* setup clean dcb */
        memset(&dcb, 0, sizeof dcb);
        dcb.DCBlength = sizeof dcb;

        /* save status of the port, so we can restore it when we do not use it anymore */
        if (!GetCommState(serial_port, &dcb)) {
            DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): GetCommState() '%s' failed: %lu.", rs232_devfile[device], GetLastError()));
            break;
        }
        fds[i].restore_dcb = dcb;

        /* set up port with some sane defaults */
        /* https://docs.microsoft.com/de-de/windows/win32/api/winbase/ns-winbase-dcb */
        /* https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-dcb */
        dcb.BaudRate = get_devbaud(device);  /* Setting BaudRate */

        dcb.ByteSize = 8;             /* Setting ByteSize = 8 */
        dcb.StopBits = ONESTOPBIT;    /* Setting StopBits = 1 */
        dcb.Parity   = NOPARITY;      /* Setting Parity = None */

        dcb.fOutxCtsFlow = FALSE;     /* disable CTS flow control */

        dcb.fOutxDsrFlow = FALSE;     /* disable DSR flow control */
        dcb.fDsrSensitivity = FALSE;

        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;

        dcb.fOutX = FALSE;            /* disable output XOFF/XON flow control */
        dcb.fInX = FALSE;             /* disable input XOFF/XON flow control */

        /* build more config options from the mode string */
        if (mode_string != NULL) {
            ++mode_string;

            while (*mode_string == ' ') {
                ++mode_string;
            }

            if (!BuildCommDCB(mode_string, &dcb)) {
                DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): BuildCommDCB() for device '%s' failed: %lu.", rs232_devfile[device], GetLastError()));
                break;
            }
        }

        DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): SetCommState() baudrate: %lu.", dcb.BaudRate));
        if (!SetCommState(serial_port, &dcb)) {
            DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): SetCommState() '%s' failed: %lu.", rs232_devfile[device], GetLastError()));
            break;
        }

        memset(&comm_timeouts, 0, sizeof comm_timeouts);

        /*
         * ensure that a read will always terminate and only return
         * what is already in the buffers
         */
        comm_timeouts.ReadIntervalTimeout = UINT32_MAX;
        comm_timeouts.ReadTotalTimeoutMultiplier = 0;
        comm_timeouts.ReadTotalTimeoutConstant = 0;

        /*
         * Do not use total timeouts for write operations
         */
        comm_timeouts.WriteTotalTimeoutConstant = 0;
        comm_timeouts.WriteTotalTimeoutMultiplier = 0;

        if (!SetCommTimeouts(serial_port, &comm_timeouts)) {
            DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_open(): SetCommTimeouts() '%s' failed: %lu.", rs232_devfile[device], GetLastError()));
            break;
        }

        if(!util_strncasecmp(rs232_devfile[device], "com", 3)) {
            fds[i].type = T_TTY;
        } else {
            fds[i].type = T_FILE;
        }
        fds[i].inuse = 1;
        fds[i].fd = serial_port;
        /* fds[i].file = rs232_devfile[device]; */
        ret = i;
        serial_port = INVALID_HANDLE_VALUE;

    } while (0);

    if (serial_port != INVALID_HANDLE_VALUE) {
        CloseHandle(serial_port);
    }

    return ret;
}

/* closes the rs232 window again */
void rs232dev_close(int fd)
{
    DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_close(fd=%d).", fd));

    if (fd < 0 || fd >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "rs232dev_close(): Attempt to close invalid fd %d.", fd);
        return;
    }
    if (!fds[fd].inuse) {
        log_error(rs232dev_log, "rs232dev_close(): Attempt to close non-open fd %d.", fd);
        return;
    }

    if (fds[fd].type == T_TTY) {
        /* restore status of the serial port to what it was before we used it */
        if (!SetCommState(fds[fd].fd, &fds[fd].restore_dcb)) {
            DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_close(): SetCommState() '%s' on close failed: %lu.", rs232_devfile[fd], GetLastError()));
        }
    }

    CloseHandle(fds[fd].fd);

    if ((fds[fd].type == T_PROC) && (fds[fd].pid != 0)) {
        kill_coproc(fds[fd].pid);
        fds[fd].pid = 0;
    }

    fds[fd].inuse = 0;
}

/* sends a byte to the RS232 line */
int rs232dev_putc(int fd, uint8_t b)
{
    uint32_t number_of_bytes = 1;
    HANDLE fdw = (fds[fd].type == T_PROC) ? fds[fd].fd_w : fds[fd].fd;

    /* DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_putc(): Output 0x%X = '%c'.", (unsigned)b, b)); */

    if (WriteFile(fdw, &b, (DWORD)1, (LPDWORD)&number_of_bytes, NULL) == 0) {
        return -1;
    }

    if (number_of_bytes == 0) {
        return -1;
    }

    return 0;
}

/* gets a byte from the RS232 line, returns 1 if byte received, byte in *b. */
int rs232dev_getc(int fd, uint8_t *b)
{
    uint32_t number_of_bytes = 0;
    HANDLE fdr = (fds[fd].type == T_PROC) ? fds[fd].fd_r : fds[fd].fd;

    if (ReadFile(fdr, b, (DWORD)1, (LPDWORD)&number_of_bytes, NULL) == 0) {
        return -1;
    }

    if (number_of_bytes) {
        /* DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_getc(): Input 0x%X = '%c'.", (unsigned)*b, *b)); */
        return 1;
    }
    return 0;
}

/* set the status lines of the RS232 device */
int rs232dev_set_status(int fd, enum rs232handshake_out status)
{
    int new_rts = status & RS232_HSO_RTS;
    int new_dtr = status & RS232_HSO_DTR;

    DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_set_status(): RTS:%s DTR:%s",
        new_rts ? "on" : "off", new_dtr ? "on" : "off"));

    if ((fd < 0) || (fd >= RS232_NUM_DEVICES)) {
        log_error(rs232dev_log, "rs232dev_set_status(): Attempted to set status of invalid fd %d.", fd);
        return -1;
    }

    if (fds[fd].type == T_TTY) {
        /* signal the RS232 device the current status, too */
        if (new_rts != fds[fd].rts) {
            fds[fd].rts = new_rts;
            EscapeCommFunction(fds[fd].fd, new_rts ? SETRTS : CLRRTS);
        }

        if (new_dtr != fds[fd].dtr) {
            fds[fd].dtr = new_dtr;
            EscapeCommFunction(fds[fd].fd, new_dtr ? SETDTR : CLRDTR);
        }
    }
    return 0;
}

/* get the status lines of the RS232 device */
enum rs232handshake_in rs232dev_get_status(int fd)
{
    /* Start with no active flags. */
    enum rs232handshake_in modem_status = 0;

    if (fd < 0 || fd >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "rs232dev_get_status(): Attempted to get status of invalid fd %d.", fd);
        return modem_status;
    }

    if (fds[fd].type == T_TTY) {
        uint32_t modemstat = 0;

        if (!GetCommModemStatus(fds[fd].fd, (LPDWORD)&modemstat)) {
            DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_get_status(): Couldn't get modem status for fd %d.", fd));
            return modem_status;
        }

        if ((modemstat & MS_CTS_ON) != 0) {
            modem_status |= RS232_HSI_CTS;
        }

        if ((modemstat & MS_DSR_ON) != 0) {
            modem_status |= RS232_HSI_DSR;
        }

        if ((modemstat & MS_RING_ON) != 0) {
            modem_status |= RS232_HSI_RI;
        }

        if ((modemstat & MS_RLSD_ON) != 0) {
            modem_status |= RS232_HSI_DCD;
        }

        DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_get_status(): got 0x%X.", (unsigned)modem_status));
        return modem_status;
    }

    /* A file always is ready to receive. */
    return RS232_HSI_DCD | RS232_HSI_DSR | RS232_HSI_CTS;
}

/* FIXME: only "aciacore.c" calls this. */
/* set the bps rate of the physical device */
void rs232dev_set_bps(int fd, unsigned int bps)
{
    /*! \todo set the physical bps rate */
    DEBUG_LOG_MESSAGE((rs232dev_log, "rs232dev_set_bps(): BPS: %u", bps));
    if ((fd < 0) || (fd >= RS232_NUM_DEVICES)) {
        log_error(rs232dev_log, "rs232dev_set_bps(): Attempted to set BPS of invalid fd %d.", fd);
    }
}

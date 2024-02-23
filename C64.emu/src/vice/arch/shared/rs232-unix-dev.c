/** \file   rs232-unix-dev.c
 * \brief   RS232 Device emulation
 *
 * \author  Andre Fachat <fachat@web.de>
 *
 * The RS232 emulation captures the bytes sent to the RS232 interfaces
 * available (currently ACIA 6551, std C64 and Daniel Dallmanns fast RS232
 * with 9600 Baud).
 * The characters captured are sent to a file or an attached process.
 * Characters sent from a process are sent back to the
 * chip emulations.
 *
 * There are four RS232 "output" devices that can be defined, so you can switch between
 * them easily: rsdev1-4. Each device definition contains the output filename
 * (see rsdev1-4 cmdline options resp. RsDevice1-4 resources in rs232drv.c), that
 * determines the action when opening up a serial device.
 *
 * If that name starts with a pipe symbol "|", the remaining name is interpreted as program
 * to start and RS232 data from the emulation is piped to that program and vice versa.
 *
 * If that name is a Unix TTY device (and not a file), the emulator detects that and
 * sets up the device as serial device, using the baud rate from the -rsdev[1-4]baud
 * command line option. So for example you can have the emulator talk to a serial port
 * on the host PC using /dev/ttyS0 (name depending on your Linux/Unix distribution)
 * and use the baud rate given in the -rsdev?baud for the external interface.
 *
 * If that name is a file, then just the RS232 output is written there.
 *
 * Note: there seems to be some extension in rs232drv.c to use some network connection,
 * but I don't know how that works.
 *
 * To use the output device, you have to select one for each of the emulated RS232 device.
 * So, to use rsdev1 in the C64 userport RS232 emulation, you have to use the "-rsuser" to
 * enable the userport emulation and "-rsuserdev 0" to select rsdev1 (unfortunately there is
 * an offset of one). And as the userport emulation can not detect the baud rate that is
 * used to shift the bits (as opposed to the ACIA emulation where the baud rate is set
 * in a register), you also have to use "-rsuserbaud 2400" to set this emulated baud rate.
 *
 * Example:
 *   x64sc -rsuser -rsuserdev 2 -rsuserbaud 2400 -rsdev3 /dev/ttyUSB0 -rsdev3baud 9600
 * lets you run your userport emulation with 2400 baud (OPEN 1,2,0,CHR$(10)+CHR$(0)), and
 * connects it to a USB-to-serial port device under /dev/ttyUSB0, that runs its RS232
 * with 9600 baud. If you have two USB-to-serial port devices connected to the same PC, you
 * could then run a terminal program on the other serial port with 9600 baud to connect
 * to the emulated C64.
 *
 * Implementation note:
 * - the T_* definitions below determine the type of serial device, TTY, file, or program.
 * - the fd[] array is a list of opened RS232 devices (where the index does not match
 *   with the device number, but most likely the order in which the device was opened - so
 *   the first opened device is in slot fd[0], the second one in fd[1] etc). Maybe a bit
 *   oversophisticated, but you could use the same PROC device for multiple RS232 emulations,
 *   e.g. use rsdev1 for both userport and ACIA, as for each opening of the device, a new
 *   process is started. (not sure if that would work with a Unix pipe to have them talk to
 *   each other).
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


#define DEBUG


#include <stdint.h>

#include "vice.h"


#ifdef HAVE_RS232DEV

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>

#include <unistd.h>

/* <sys/select.h> is required for select(2) and fd_set */
#if defined(HAVE_SYS_SELECT_H)
# include <sys/select.h>
#endif

#ifndef FD_ISSET
#define FD_ISSET(n, p) ((p)->fds_bits[(n)/NFDBITS] & (1L << ((n) % NFDBITS)))
#endif

#include "cmdline.h"
#include "coproc.h"
#include "log.h"
#include "resources.h"
#include "rs232.h"
#include "rs232dev.h"
#include "types.h"

/* ------------------------------------------------------------------------- */

/* resource handling */

/* baudrate of physical RS232 device (unused with pipes and sockets) */
static int devbaud[RS232_NUM_DEVICES];

static int set_devbaud(int val, void *param)
{
    devbaud[vice_ptr_to_int(param)] = val;
    return 0;
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

/** \brief  Whoever wrote this: please document this, thanks
 *
 * This struct saves the status of the opened RS232 device.
 * There are (currently) four (RS232_NUM_DEVICES) device slots that can be selected at the same time.
 * The state is stored in the "fds[]" array of this type below.
 */
typedef struct rs232 {
    int inuse;     /* device is in use */
    int type;      /* what type of device is this, see T_* defines below */
    int fd_r;      /* unix fd for reading; note for file and tty fd_r == fd_w, but PROC has it (potentially) different */
    int fd_w;      /* unix fd for writing */
    char *file;    /* filename */
    struct termios saved;
    vice_pid_t pid;     /* process id of the child process when type = T_PROC */
} rs232_t;

/* Also document this crap */
#define T_FILE 0   /* use a file, e.g. to dump RS232 output */
#define T_TTY  1   /* a tty, like a real RS232 device /dev/ttyUSB0 */
#define T_PROC 2   /* a process, where the RS232 data is piped to/from */

/* And these things */
static rs232_t fds[RS232_NUM_DEVICES]; /* status for all open RS232_NUM_DEVICES devices */
static log_t rs232dev_log = LOG_ERR;   /* logger */

/* ------------------------------------------------------------------------- */

void rs232dev_close(int fd);

/* initializes all RS232 stuff */
void rs232dev_init(void)
{
    int i;

    for (i = 0; i < RS232_NUM_DEVICES; i++) {
        fds[i].inuse = 0;
    }

    rs232dev_log = log_open("RS232DEV");
}

/* resets terminal to old mode */
static void unset_tty(int i)
{
    tcsetattr(fds[i].fd_r, TCSAFLUSH, &fds[i].saved);
}

static struct {
    int baud;
    speed_t speed;
} speed_tab[] = {
    { 300, B300 },
    { 600, B600 },
    { 1200, B1200 },
    { 1800, B1800 },
    { 2400, B2400 },
    { 4800, B4800 },
    { 9600, B9600 },
    { 19200, B19200 },
    { 38400, B38400 },
    { 57600, B57600 },
    { 115200, B115200},
    { 0, B9600 }                                /* fallback */
};

/* sets terminal to raw mode */
static void set_tty(int i, int baud)
{
    /*
     * set tty to raw mode as of
     * "Advanced Programming in the Unix Environment"
     * by W.R. Stevens, Addison-Wesley.
     */
    speed_t speed;
    int fd = fds[i].fd_r;
    struct termios buf;

    log_message(rs232dev_log, "rs232: trying to set device %s to %d baud.",
                fds[i].file, baud);

    if (tcgetattr(fd, &fds[i].saved) < 0) {
        return /* -1 */ ;
    }
    buf = fds[i].saved;

    buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /* echho off, canonical mode off, extended input processing
     * off, signal chars off */
    buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* no SIGINT on Break, CR-to-NL off, input parity check off,
     * don't strip 8th bit on input, output flow control off */
    buf.c_cflag &= ~(CSIZE | PARENB);

    /* clear size bits, parity checking off */
    buf.c_cflag |= CS8;

    /* set 8 bits/char */
    buf.c_oflag &= ~(OPOST);

    /* ouput processing off */
    buf.c_cc[VMIN] = 1;         /* 1 byte at a time, no timer */
    buf.c_cc[VTIME] = 0;

    for (i = 0; speed_tab[i].baud; i++) {
        if (speed_tab[i].baud >= baud) {
            break;
        }
    }
    speed = speed_tab[i].speed;

    cfsetispeed(&buf, speed);
    cfsetospeed(&buf, speed);

    tcsetattr(fd, TCSAFLUSH, &buf);
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
    int i, fd;

    for (i = 0; i < RS232_NUM_DEVICES; i++) {
        if (!fds[i].inuse) {
            break;
        }
    }
    if (i >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "No more devices available.");
        return -1;
    }

#ifdef DEBUG
    log_message(rs232dev_log, "rs232dev_open(device '%s' as dev %d).",
                rs232_devfile[device], device);
#endif

    fds[i].pid = 0;

    if (rs232_devfile[device][0] == '|') {
        vice_pid_t pid;
        if (fork_coproc(&fds[i].fd_w, &fds[i].fd_r, rs232_devfile[device] + 1, &pid) < 0) {
            log_error(rs232dev_log, "Cannot fork process '%s'.", rs232_devfile[device] + 1);
            return -1;
        }
        fds[i].type = T_PROC;
        fds[i].pid = pid;
        fds[i].inuse = 1;
        fds[i].file = rs232_devfile[device];
    } else {
        fd = open(rs232_devfile[device], O_RDWR | O_NOCTTY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0) {
            log_error(rs232dev_log, "Cannot open file \"%s\": %s",
                      rs232_devfile[device], strerror(errno));
            return -1;
        }
        fds[i].fd_r = fds[i].fd_w = fd;
        fds[i].file = rs232_devfile[device];

        if (isatty(fd)) {
            fds[i].type = T_TTY;
            set_tty(i, devbaud[device]);
        } else {
            fds[i].type = T_FILE;
        }
        fds[i].inuse = 1;
    }

    return i;
}

/* closes the rs232 window again */
void rs232dev_close(int fd)
{
#ifdef DEBUG
    log_message(rs232dev_log, "close(fd=%d).", fd);
#endif

    if (fd < 0 || fd >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "Attempt to close invalid fd %d.", fd);
        return;
    }
    if (!fds[fd].inuse) {
        log_error(rs232dev_log, "Attempt to close non-open fd %d.", fd);
        return;
    }

    if (fds[fd].type == T_TTY) {
        unset_tty(fd);
    }
    close(fds[fd].fd_r);
    if ((fds[fd].type == T_PROC) && (fds[fd].fd_r != fds[fd].fd_w)) {
        close(fds[fd].fd_w);
    }
    if ((fds[fd].type == T_PROC) && (fds[fd].pid != 0)) {
        kill_coproc(fds[fd].pid);
    }
    fds[fd].inuse = 0;
}

/* sends a byte to the RS232 line */
int rs232dev_putc(int fd, uint8_t b)
{
    ssize_t n;

    if (fd < 0 || fd >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "Attempt to write to invalid fd %d.", fd);
        return -1;
    }
    if (!fds[fd].inuse) {
        log_error(rs232dev_log, "Attempt to write to non-open fd %d.", fd);
        return -1;
    }

    /* for the beginning... */
#ifdef DEBUG
    log_message(rs232dev_log, "Output `%c'.", b);
#endif

    do {
        n = write(fds[fd].fd_w, &b, 1);
        if (n < 0) {
            log_error(rs232dev_log, "Error writing: %s.", strerror(errno));
            return -1;
        }
    } while (n != 1);

    return 0;
}

/* gets a byte to the RS232 line, returns !=0 if byte received, byte in *b. */
int rs232dev_getc(int fd, uint8_t * b)
{
    int ret;
    size_t n;
    fd_set rdset;
    struct timeval ti;

    if (fd < 0 || fd >= RS232_NUM_DEVICES) {
        log_error(rs232dev_log, "Attempt to read from invalid fd %d.", fd);
        return -1;
    }
    if (!fds[fd].inuse) {
        log_error(rs232dev_log, "Attempt to read from non-open fd %d.", fd);
        return -1;
    }

    if (fds[fd].type == T_FILE) {
        return 0;
    }

    FD_ZERO(&rdset);
    FD_SET(fds[fd].fd_r, &rdset);
    ti.tv_sec = ti.tv_usec = 0;

    ret = select(fds[fd].fd_r + 1, &rdset, NULL, NULL, &ti);

    if (ret && (FD_ISSET(fds[fd].fd_r, &rdset))) {
        n = read(fds[fd].fd_r, b, 1);
        if (n) {
            return 1;
        }
    }
    return 0;
}

/* set the status lines of the RS232 device */
int rs232dev_set_status(int fd, enum rs232handshake_out status)
{
    if ((fd < 0) || (fd >= RS232_NUM_DEVICES)) {
        log_error(rs232dev_log, "Attempted to set status of invalid fd %d.", fd);
    }
    /*! \todo dummy */
    return -1;
}

/* get the status lines of the RS232 device */
enum rs232handshake_in rs232dev_get_status(int fd)
{
    if ((fd < 0) || (fd >= RS232_NUM_DEVICES)) {
        log_error(rs232dev_log, "Attempted to get status of invalid fd %d.", fd);
    }
    /*! \todo dummy */
    return RS232_HSI_CTS | RS232_HSI_DSR;
}

/* set the bps rate of the physical device */
void rs232dev_set_bps(int fd, unsigned int bps)
{
    if ((fd < 0) || (fd >= RS232_NUM_DEVICES)) {
        log_error(rs232dev_log, "Attempted to set bps of invalid fd %d.", fd);
    }
    /*! \todo dummy */
}

#endif /* HAVE_RS232DEV */

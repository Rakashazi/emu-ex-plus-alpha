/*
 *  NAME:
 *      usleep     -- This is the precision timer for Test Set
 *                    Automation. It uses the select(2) system
 *                    call to delay for the desired number of
 *                    micro-seconds. This call returns ZERO
 *                    (which is usually ignored) on successful
 *                    completion, -1 otherwise.
 *
 *  ALGORITHM:
 *      1) We range check the passed in microseconds and log a
 *         warning message if appropriate. We then return without
 *         delay, flagging an error.
 *      2) Load the Seconds and micro-seconds portion of the
 *         interval timer structure.
 *      3) Call select(2) with no file descriptors set, just the
 *         timer, this results in either delaying the proper
 *         ammount of time or being interupted early by a signal.
 *
 *  HISTORY:
 *      Added when the need for a subsecond timer was evident.
 *
 *  AUTHOR:
 *      Michael J. Dyer                   Telephone:   AT&T 414.647.4044
 *      General Electric Medical Systems        GE DialComm  8 *767.4044
 *      P.O. Box 414  Mail Stop 12-27         Sect'y   AT&T 414.647.4584
 *      Milwaukee, Wisconsin  USA 53201                      8 *767.4584
 *      internet:  mike@sherlock.med.ge.com     GEMS WIZARD e-mail: DYER
 */

#include "vice.h"

#if defined(HAVE_UNISTD_H) && !defined(AMIGA_MORPHOS)
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef AMIGA_MORPHOS
#include <proto/socket.h>
# define select(nfds, read_fds, write_fds, except_fds, timeout) \
    WaitSelect(nfds, read_fds, write_fds, except_fds, timeout, NULL)
#endif

#ifdef __OS2__
#include <sys/select.h>
#endif

int usleep(unsigned long int microSeconds)
{
    unsigned int Seconds, uSec;
#ifdef __OS2__
    int nfds;
    fd_set readfds, writefds, exceptfds;
#else
    int nfds, readfds, writefds, exceptfds;
#endif

    struct  timeval Timer;

#ifdef __OS2__
    nfds = 0;
#else
    nfds = readfds = writefds = exceptfds = 0;
#endif

    if ((microSeconds == (unsigned long) 0) || microSeconds > (unsigned long) 4000000) {
        errno = ERANGE;                 /* value out of range */
        return -1;
    }

    Seconds = microSeconds / (unsigned long) 1000000;
    uSec = microSeconds % (unsigned long) 1000000;

    Timer.tv_sec = Seconds;
    Timer.tv_usec = uSec;

    if (select( nfds, &readfds, &writefds, &exceptfds, &Timer ) < 0) {
        return -1;
    }

    return 0;
}

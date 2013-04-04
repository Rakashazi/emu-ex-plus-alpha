/*
 * soundmidas.c - Implementation of the MIDAS sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

/* XXX: includes */

#include "vice.h"

#include <stdio.h>

#include "sound.h"
#include "vmidas.h"

static int midas_bufferspace(void);

static MIDASstreamHandle midas_stream = NULL;
static int midas_bufsize = -1;
static int midas_maxsize = -1;

static int midas_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    BOOL st;

    /* No stereo capability. */
    *channels = 1;

    st = vmidas_startup();
    if (st != TRUE) {
        return 1;
    }
    st = MIDASsetOption(MIDAS_OPTION_MIXRATE, *speed);
    if (st != TRUE) {
        return 1;
    }
    st = MIDASsetOption(MIDAS_OPTION_MIXING_MODE, MIDAS_MIX_NORMAL_QUALITY);
    if (st != TRUE) {
        return 1;
    }
    st = MIDASsetOption(MIDAS_OPTION_OUTPUTMODE, MIDAS_MODE_16BIT_MONO);
    if (st != TRUE) {
        return 1;
    }
    st = MIDASsetOption(MIDAS_OPTION_MIXBUFLEN, (*fragsize) * (*fragnr) * sizeof(SWORD));
    if (st != TRUE) {
        return 1;
    }
    st = MIDASsetOption(MIDAS_OPTION_MIXBUFBLOCKS, *fragnr);
    if (st != TRUE) {
        return 1;
    }
#ifdef __MSDOS__
#if 0
    st = MIDASconfig();
    if (st != TRUE) {
        return 1;
    }
#endif
#endif
    st = vmidas_init();
    if (st != TRUE) {
        return 1;
    }
    st = MIDASopenChannels(1);
    if (st != TRUE) {
        /* st = MIDASclose(); */
        return 1;
    }
    midas_stream = MIDASplayStreamPolling(MIDAS_SAMPLE_16BIT_MONO, *speed,
                                          (int)(*fragsize * *fragnr * 1000));
    if (!midas_stream) {
        st = MIDAScloseChannels();
        /* st = MIDASclose(); */
        return 1;
    }
    midas_bufsize = (*fragsize) * (*fragnr);
    midas_maxsize = midas_bufsize / 2;
    return 0;
}

static int midas_write(SWORD *pbuf, size_t nr)
{
    BOOL st = 1;
    unsigned int ist;

    ist = MIDASfeedStreamData(midas_stream, (unsigned char *)pbuf, nr * sizeof(SWORD), TRUE);
    if (ist != nr * sizeof(SWORD)) {
        return 1;
    }
#ifndef __MSDOS__
    st = MIDASpoll();
#endif
    return !st;
}

static int midas_bufferspace(void)
{
    int nr;
    /* MIDASgetStreamBytesBuffered returns the number of buffered bytes. */
    nr = midas_bufsize - MIDASgetStreamBytesBuffered(midas_stream);
    if (nr < 0) {
        nr = 0;
    }
    nr /= sizeof(SWORD);
    if (nr > midas_maxsize) {
        midas_maxsize = nr;
    }
    return (int)((double)nr / midas_maxsize * midas_bufsize);
}

static void midas_close(void)
{
    BOOL st;

    /* XXX: we might come here from `atexit', so MIDAS might have been shut
       down already.  This is a dirty kludge, we should find a cleaner way to
       do it. */
    if (vmidas_available()) {
        st = MIDASstopStream(midas_stream);
        st = MIDAScloseChannels();
        /* st = MIDASclose(); */
    }
    midas_stream = NULL;
    midas_bufsize = -1;
    midas_maxsize = -1;
}

static sound_device_t midas_device =
{
    "midas",
    midas_init,
    midas_write,
    NULL,
    NULL,
    midas_bufferspace,
    midas_close,
    NULL,
    NULL,
    1,
    1
};

int sound_init_midas_device(void)
{
    return sound_register_device(&midas_device);
}

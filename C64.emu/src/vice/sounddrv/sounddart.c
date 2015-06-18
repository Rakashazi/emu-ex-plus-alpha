/*
 * sounddart.c - Implementation of the OS/2-DART sound device
 *
 * Written by
 *  Thomas Bretz (tbretz@gsi.de)
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

#include "vice.h"

#define INCL_DOSPROFILE
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#include <os2.h>
#define INCL_MCIOS2
#include <os2me.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"
#include "sound.h"

#include "sounddrv.h"

/* typedefs for MCI_***_PARAMS: at the end of file */
static unsigned short usDeviceID;       /* DART Amp-mixer device ID   */

static MCI_BUFFER_PARMS BufferParms;
static MCI_MIXSETUP_PARMS MixSetupParms;
static MCI_MIX_BUFFER *buffers;

static UINT play = 0; /* this is the buffer which is played next */
static UINT last = 0; /* this is the buffer which is played last before next */
static UINT pos = 0;  /* this is the position to which buffer we have to write */

static HMTX hmtxSnd;
static HMTX hmtxOC;  /* Open, Close / not _really_ necessary */

static log_t dlog = LOG_ERR;

// --------------------------------------------------------------------------

#define MUTE_OFF   MCI_SET_ON
#define MUTE_ON    MCI_SET_OFF

static int volume = 50;

int get_volume()
{
    /*
     LONG rc;
     MCI_STATUS_PARMS mciStatus;
     mciStatus.ulItem = MCI_STATUS_VOLUME;
     rc = mciSendCommand(usDeviceID, MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM,
     (PVOID) &mciStatus, 0);
     log_debug("rc=%d, ulReturn=%d", rc, mciStatus.ulReturn);
     */
    return volume;
}

void set_volume(int vol)
{
    ULONG rc;

    if ((rc = DosRequestMutexSem(hmtxOC, SEM_INDEFINITE_WAIT))) {
        log_warning(dlog, "set_volume, DosRequestMutexSem rc=%i", rc);
        return;
    }
    if (usDeviceID) {
        MCI_SET_PARMS MciSetParms;

        memset(&MciSetParms, 0, sizeof(MCI_SET_PARMS));

        MciSetParms.ulLevel = vol;
        MciSetParms.ulAudio = MCI_SET_AUDIO_ALL;

        log_message(dlog, "Setting volume to %d%%", vol);

        rc = mciSendCommand(usDeviceID, MCI_SET, MCI_WAIT | MCI_SET_VOLUME | MCI_SET_AUDIO, (PVOID) &MciSetParms, 0);
        if (rc != MCIERR_SUCCESS) {
            sound_err(dlog, rc, "Setting up Volume (MCI_SET).");
        }
    }
    volume = vol;
    DosReleaseMutexSem(hmtxOC);
}

void mute(int state)
{
    ULONG rc;

    if ((rc = DosRequestMutexSem(hmtxOC, SEM_INDEFINITE_WAIT))) {
        log_warning(dlog, "mute, DosRequestMutexSem rc=%i", rc);
        return;
    }
    if (usDeviceID) {
        MCI_SET_PARMS MciSetParms;

        memset(&MciSetParms, 0, sizeof(MCI_SET_PARMS));

        MciSetParms.ulAudio = MCI_SET_AUDIO_ALL;
        rc = mciSendCommand(usDeviceID, MCI_SET, MCI_WAIT | state | MCI_SET_AUDIO, (PVOID) &MciSetParms, 0);

        if (rc != MCIERR_SUCCESS) {
            sound_err(dlog, rc, "Setting mute state (MCI_SET_ON/OFF).");
        }
    }
    DosReleaseMutexSem(hmtxOC);

    if (state == MCI_SET_ON) {
        set_volume(volume);
    }
}

// --------------------------------------------------------------------------

LONG APIENTRY DARTEvent (ULONG ulStatus, PMCI_MIX_BUFFER pBuffer, ULONG ulFlags)
{
    ULONG rc;
    switch (ulFlags) {
        case MIX_WRITE_COMPLETE:
            /*
               start the playback of the next buffer
            */
            rc = MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle,
                                         &(buffers[play]), 1);
            /*
               get the sound mutex
            */
            if (DosRequestMutexSem(hmtxSnd, SEM_INDEFINITE_WAIT)) {
                return TRUE;
            }
            /*
               empty the buffer which was finished
            */
            memset(buffers[last].pBuffer, 0, BufferParms.ulBufferSize);

            /*
               point to the next playable buffer and remember this buffer
               as the last one which was played
            */
            last = play++;
            play %= BufferParms.ulNumBuffers;

            /*
               release mutex
            */
            DosReleaseMutexSem(hmtxSnd);

            if (rc != MCIERR_SUCCESS) {
                sound_err(dlog, rc, "Writing to Mixer (pmixWrite, MIX_WRITE_COMPLETE).");
            }
            return TRUE;

        case MIX_STREAM_ERROR | MIX_WRITE_COMPLETE: /* 130 */  /* error occur in device */
            switch (ulStatus) {
                case ERROR_DEVICE_UNDERRUN: /* 5626 */
                    sound_err(dlog, ulStatus, "Device underrun.");
                    play += 2;
                    play %= BufferParms.ulNumBuffers;
                    break;
                case ERROR_DEVICE_OVERRUN: /* 5627 */
                    sound_err(dlog, ulStatus, "Device overrun.");
                    break;
            }
            rc = MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle, &(buffers[play]), 1);
            if (rc != MCIERR_SUCCESS) {
                sound_err(dlog, rc, "Writing to Mixer (pmixWrite, MIX_STREAM_ERROR).");
            }

            return TRUE;
    }
    return TRUE;
}

static float mmtime;  /* [1000 samples / s] */
static int written;   /* number of totaly written samples */

/* number of samples between write pos and end of buffer
   this is used because the number of samples is not
   devidable by the size of one buffer
*/ 
static int rest;

static int DartOpen(void)
{
    ULONG rc;
    MCI_AMP_OPEN_PARMS AmpOpenParms;

    memset(&AmpOpenParms, 0, sizeof (MCI_AMP_OPEN_PARMS));

    AmpOpenParms.usDeviceID = (USHORT) 0;
    AmpOpenParms.pszDeviceType = (PSZ) MCI_DEVTYPE_AUDIO_AMPMIX;
    AmpOpenParms.hwndCallback = 0;

    rc = mciSendCommand(0, MCI_OPEN,
                        MCI_WAIT | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE /* | MCI_DOS_QUEUE*/,
                        (PVOID) &AmpOpenParms, 0);

    if (rc != MCIERR_SUCCESS) {
        usDeviceID = 0;
        sound_err(dlog, rc, "Opening DART (MCI_OPEN).");

        return FALSE;
    }

    usDeviceID = AmpOpenParms.usDeviceID;
    return TRUE;
}

static void DartClose(void)
{
    MCI_GENERIC_PARMS GenericParms = { 0 };

    ULONG rc = mciSendCommand(usDeviceID, MCI_CLOSE, MCI_WAIT,
                              (PVOID) &GenericParms, 0);

    if (rc != MCIERR_SUCCESS) {
        sound_err(dlog, rc, "Closing Dart (MCI_CLOSE).");
    }

    log_message(dlog, "Sound closed.");

    usDeviceID = 0;
}

static int dart_init(const char *param, int *speed,
                     int *fragsize, int *fragnr, int *channels)
{
/* The application sends the MCI_MIX_SETUP message to the amp mixer to
 initialize the device for direct reading and writing of audio data in
 the correct mode and format-for example, PCM, MIDI, or MPEG audio.

 If waveform audio data will be played or recorded, the application
 fills in the ulDeviceType field with MCI_DEVICETYPE_WAVEFORM_AUDIO. It
 must also provide values for the following digital-audio-specific
 fields:  format tag, bits per sample, number of samples per second, and
 number of channels.*/

    /* MCI_MIXSETUP informs the mixer device of the entry point
       to report buffers being read or written.
       We will also need to tell the mixer which media type
       we will be streaming.  In this case, we'll use
       MCI_DEVTYPE_WAVEFORM_AUDIO.
    */

    ULONG i, rc;

    if (DosRequestMutexSem(hmtxOC, SEM_IMMEDIATE_RETURN)) {
        return TRUE;
    }

    /* --------- */
    if (!DartOpen()) {
        DosReleaseMutexSem(hmtxOC);
        return 1;
    }
    /* --------- */

    memset(&MixSetupParms, 0, sizeof(MCI_MIXSETUP_PARMS));

    MixSetupParms.ulBitsPerSample = 16;
    MixSetupParms.ulFormatTag = MCI_WAVE_FORMAT_PCM;
    MixSetupParms.ulSamplesPerSec = *speed;
    MixSetupParms.ulChannels = *channels;      /* Stereo/Mono */
    MixSetupParms.ulFormatMode = MCI_PLAY;
    MixSetupParms.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;

    /* MCI_MIXSETUP_QUERYMODE
       Queries a device to see if a specific mode is supported */
    /*    rc = mciSendCommand( usDeviceID, MCI_MIXSETUP,
     MCI_WAIT | MCI_MIXSETUP_QUERYMODE,
     (PVOID) &MixSetupParms, 0);

     if (rc != MCIERR_SUCCESS) return sound_err(dlog, (rc, "Can't play.");*/

    /* The mixer will inform us of entry points to
       read/write buffers to and also give us a
       handle to use with these entry points. */

    MixSetupParms.pmixEvent = DARTEvent;

    /* MCI_MIXSETUP_INIT (DEINIT)
       Initializes the mixer for the correct mode (MCI_MIXSETUP_PARMS) */
    rc = mciSendCommand(usDeviceID, MCI_MIXSETUP, MCI_WAIT | MCI_MIXSETUP_INIT, (PVOID) &MixSetupParms, 0);

    if (rc != MCIERR_SUCCESS) {
        sound_err(dlog, rc, "Initialising mixer device (MCI_MIXSETUP_INIT).");
        DartClose();
        DosReleaseMutexSem(hmtxOC);
        return 1;
    }

    /* log_message(LOG_DEFAULT, "sounddart.c: %3i buffers %6i bytes (suggested by dart)", MixSetupParms.ulNumBuffers, MixSetupParms.ulBufferSize); */
    /* log_message(LOG_DEFAULT, "sounddart.c: %3i buffers %6i bytes (wanted by vice)", *fragnr, *fragsize*sizeof(SWORD)); */

    /*  After the mixer device is set up to use DART, the application
     instructs the device to allocate memory by sending the MCI_BUFFER
     message with the MCI_ALLOCATE_MEMORY flag set. The application uses the
     MCI_BUFFER_PARMS structure to specify the number of buffers it wants
     and the size to be used for each buffer.

     Note: Because of device driver restrictions, buffers are limited to
     64KB on Intel-based systems. No such limit exists on PowerPC systems.

     The pBufList field contains a pointer to an array of MCI_MIX_BUFFER
     structures where the allocated information is to be returned.*/

    buffers = lib_calloc(*fragnr, sizeof(MCI_MIX_BUFFER));

    BufferParms.pBufList = buffers;
    BufferParms.ulNumBuffers = *fragnr;
    BufferParms.ulBufferSize = *fragsize * sizeof(SWORD);

    rc = mciSendCommand(usDeviceID, MCI_BUFFER, MCI_WAIT | MCI_ALLOCATE_MEMORY,
                        (PVOID) &BufferParms, 0);

    if (rc != MCIERR_SUCCESS) {
        sound_err(dlog, rc, "Allocating Memory (MCI_ALLOCATE_MEMORY).");
        DartClose();
        DosReleaseMutexSem(hmtxOC);
        return 1;
    }

    /* MCI driver will return the number of buffers it
       was able to allocate
       it will also return the size of the information
       allocated with each buffer. */

    if (*fragnr != BufferParms.ulNumBuffers) {
        *fragnr = BufferParms.ulNumBuffers;
        log_message(dlog, "got %3i buffers %6i bytes.", BufferParms.ulNumBuffers, BufferParms.ulBufferSize);
    }
    if (*fragsize != BufferParms.ulBufferSize / sizeof(SWORD)) {
        *fragsize = BufferParms.ulBufferSize / sizeof(SWORD);
        log_message(dlog, "got %3i buffers %6i bytes.", BufferParms.ulNumBuffers, BufferParms.ulBufferSize);
    }

    /* SECURITY for *fragnr <2 ???? */
    for (i = 0; i < *fragnr; i++) {
        memset(buffers[i].pBuffer, 0, BufferParms.ulBufferSize);
    }
    /* *fragsize*sizeof(SWORD)); */

    mmtime = (float)*speed / 1000;
    written = 0;
    rest = BufferParms.ulBufferSize;

    /* Must write at least two buffers to start mixer */
    play = 2;  /* this is the buffer which is played next */
    last = 1;  /* this is the buffer which is played last before next */
    pos = 0;   /* this is the position to which buffer we have to write */
    MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle,
                            &(buffers[0]), 2);

    DosReleaseMutexSem(hmtxOC);

    mute(MUTE_OFF);

    return 0;
}

static void dart_close()
{
    MCI_GENERIC_PARMS GenericParms = { 0 };
    ULONG rc;

    /* prevent sound from clicking */
    mute(MUTE_ON);

    if ((rc = DosRequestMutexSem(hmtxOC, SEM_INDEFINITE_WAIT))) {
        log_warning(dlog, "dart_close, DosRequestMutexSem rc=%i", rc);
        return;
    }

    /*DosRequestMutexSem(hmtxSnd, SEM_INDEFINITE_WAIT);*/
    for (rc = 0; rc < BufferParms.ulNumBuffers; rc++) {
        buffers[rc].ulFlags = MIX_BUFFER_EOS;
    }

    rc = mciSendCommand(usDeviceID, MCI_STOP, MCI_WAIT,
                        (PVOID) &GenericParms, 0);
    if (rc != MCIERR_SUCCESS) {
        sound_err(dlog, rc, "Stopping device (MCI_STOP).");
    }

    /*log_message(LOG_DEFAULT, "sounddrv.c: Sound stopped.");*/

    rc = mciSendCommand(usDeviceID, MCI_BUFFER, MCI_WAIT | MCI_DEALLOCATE_MEMORY,
                        (PVOID) &BufferParms, 0);
    if (rc != MCIERR_SUCCESS) {
        sound_err(dlog, rc, "Deallocating buffer (MCI_DEALLOCATE).");
    }

    /*log_message(LOG_DEFAULT, "sounddrv.c: Buffer deallocated.");*/
    /*    rc = mciSendCommand(usDeviceID, MCI_MIXSETUP, MCI_WAIT|MCI_MIXSETUP_DEINIT,
     (PVOID) &MixSetupParms, 0);
     if (rc != MCIERR_SUCCESS) return sound_err(dlog, (rc, "DART_ERR_MIXSETUP_DEINIT");*/

    DartClose();

    lib_free(buffers);
    /*log_message(LOG_DEFAULT, "sounddrv.c: Buffer freed.");*/

    DosReleaseMutexSem(hmtxOC);
}

/*
static int dart_write(SWORD *pbuf, size_t nr)
{
    /* The MCI_MIX_BUFFER structure is used for reading and writing data to
       and from the mixer.

       Once the device is set up and memory has been allocated, the
       application can use the function pointers obtained during MCI_MIXSETUP
       to communicate with the mixer. During a playback operation, the
       application fills the buffers with audio data and then writes the
       buffers to the mixer device using the pmixWrite entry point. When audio
       data is being recorded, the mixer device fills the buffers using the
       pmixRead entry point. Each buffer returned the the application has a
       time stamp (in milliseconds) attached so the program can determine the
       current time of the device.

       MCI_STOP, MCI_PAUSE, and MCI_RESUME are used to stop, pause, or resume
       the audio device, respectively. MCI_STOP and MCI_PAUSE can only be sent
       to the mixer device after mixRead and mixWrite have been called.
       MCI_RESUME will only work after MCI_PAUSE has been sent.

       Note:  After your application has completed data transfers, issue
       MCI_STOP to avoid a pause the next time the mixer device is started.

       If your application needs more precise timing information than
       provided by the time stamp returned with each buffer, you can use
       MCI_STATUS with the MCI_STATUS_POSITION flag to retrieve the current
       time of the device in MMTIME units.
    */
    APIRET rc;
    if ((rc=DosRequestMutexSem(hmtxSnd, SEM_INDEFINITE_WAIT)))
    {
       log_debug("sounddart.c: dart_write, DosRequestMutexSem rc=%i", rc);
       return 1;
    }

    /* if we wanna write the buffer which is played next
       write the overnext (skip one buffer). */
    if (play==(pos+1)%BufferParms.ulNumBuffers)
    {
        pos++;
        pos %= BufferParms.ulNumBuffers;
    }

    memcpy(buffers[pos++].pBuffer, pbuf, nr*sizeof(SWORD));
    pos %= BufferParms.ulNumBuffers;

    DosReleaseMutexSem(hmtxSnd);

    return 0;
}*/

static int dart_write2(SWORD *pbuf, size_t nr)
{
    APIRET rc;
    int nrtowrite;

    written += nr;
    nr *= sizeof(SWORD);

    while (nr)
    {
        /*
           check if the buffer can be filled completely
        */
        nrtowrite = nr > rest ? rest : nr;

        if ((rc = DosRequestMutexSem(hmtxSnd, SEM_INDEFINITE_WAIT))) {
            log_warning(dlog, "dart_write2, DosRequestMutexSem rc=%i", rc);
            return 1;
        }

        /*
           if the buffer we want to write to is the next one which is
           played, skip one buffer and write to the overnext
        */

        if (play == (pos + 1) % BufferParms.ulNumBuffers) {
            pos = (++pos) % BufferParms.ulNumBuffers;
            written += BufferParms.ulBufferSize / sizeof(SWORD);
        }

        /*
           fill the pos-th buffer
        */
        memcpy((void*)((ULONG)buffers[pos].pBuffer + (BufferParms.ulBufferSize - rest)),
               pbuf, nrtowrite);

        rest -= nrtowrite;

        /*
           check if the buffer was filled completely
        */
        if (!rest) {
            pos++;
            pos %= BufferParms.ulNumBuffers;
            rest = BufferParms.ulBufferSize;
        }

        DosReleaseMutexSem(hmtxSnd);

        nr -= nrtowrite;
    }
    return 0;
}

/* return number of free samples in the kernel buffer at the moment */
static int dart_bufferspace(void)
{
    /*
       FIXME!!! This is not the best I can do...
       but better than nothing
    */
    ULONG rc;
    if ((rc = DosRequestMutexSem(hmtxSnd, SEM_INDEFINITE_WAIT))) {
        log_warning(dlog, "dart_bufferspace, DosRequestMutexSem rc=%i", rc);
        return -1;
    }

    rc = last - pos;  /* remember: last is nothing else than play-1 */

    DosReleaseMutexSem(hmtxSnd);

    if (pos > last) {
        rc += BufferParms.ulNumBuffers;
    }

    return rc * BufferParms.ulBufferSize / (MixSetupParms.ulChannels * sizeof(SWORD));
#if 0
    LONG rc;
    MCI_STATUS_PARMS mciStatus;

    const int bufsz = BufferParms.ulBufferSize/sizeof(SWORD)*BufferParms.ulNumBuffers;

    mciStatus.ulItem = MCI_STATUS_POSITION;

    /*
       mciSendCommand() query for MCI_STATUS_POSITION returns position
       in MMTIME units (currently set to milliseconds).
    */
    rc = mciSendCommand(usDeviceID, MCI_STATUS,
                        MCI_WAIT|MCI_STATUS_ITEM,
                        (PVOID) &mciStatus, 0);

    /*
       number of samples unplayed in the kernel buffer at the moment
      
       The number can become negative if we are running too slow,
       because then we played more samples than we have written actually
      
       written:   all samples ever written
       mciStatus: all samples ever played
    */
    rc = written - mmtime*mciStatus.ulReturn;

    /*
       It would also be possible to return 'rest'. But rest are doesn't
       store the actual number of free samples. It stores the number
       of free samples at the time dart_write2 was called the last time
    */
    if (bufsz<rc) {
        return 0;
    }
    if (rc<0) {
        return bufsz;
    }
    return bufsz-rc;
#endif
}

static int dart_suspend()
{
    mute(MUTE_ON);
    log_message(dlog, "Sound output suspended.");
    return 0;
}

static int dart_resume()
{
    mute(MUTE_OFF);
    log_message(dlog, "Sound output resumed.");
    return 0;
}

static sound_device_t dart_device =
{
    "dart",
    dart_init,         /* dart_init */
    dart_write2,       /* dart_write */
    NULL,              /* dart_dump */
    NULL,              /* dart_flush */
    dart_bufferspace,  /* dart_bufferspace */
    dart_close,        /* dart_close */
    dart_suspend,      /* dart_suspend */
    dart_resume,       /* dart_resume */
    1,
    2
};

#if 0
static sound_device_t dart_device =
{
    "dart",
    dart_init,         /* dart_init */
    dart_write,        /* dart_write */
    NULL,              /* dart_dump */
    NULL,              /* dart_flush */
    NULL,              /* dart_bufferspace */
    dart_close,        /* dart_close */
    dart_suspend,      /* dart_suspend */
    dart_resume        /* dart_resume */
};

static sound_device_t dart2_device =
{
    "dart2",
    dart_init,         /* dart_init */
    dart_write2,       /* dart_write */
    NULL,              /* dart_dump */
    NULL,              /* dart_flush */
    dart_bufferspace,  /* dart_bufferspace */
    dart_close,        /* dart_close */
    dart_suspend,      /* dart_suspend */
    dart_resume        /* dart_resume */
};
#endif

void sound_init_dart(void)
{
/*    static int first=TRUE;
    if (!first)
        return;
*/
    dlog = log_open("Dart");

    DosCreateMutexSem("\\SEM32\\Vice2\\Sound\\OC.sem", &hmtxOC, 0, FALSE);
    DosCreateMutexSem("\\SEM32\\Vice2\\Sound\\Write.sem", &hmtxSnd, 0, FALSE);
/*    first=FALSE;*/
}

int sound_init_dart_device(void)
{
    sound_init_dart();
    return sound_register_device(&dart_device);
}

/*
int sound_init_dart2_device(void)
{
    sound_init_dart();
    return sound_register_device(&dart2_device);
}
*/

#if 0
typedef struct_MCI_MIXSETUP_PARMS
{
    HWND        hwndCallback;    /* IN  Window for notifications */
    ULONG       ulBitsPerSample; /* IN  Number of bits per sample */
    ULONG       ulFormatTag;     /* IN  Format tag */
    ULONG       ulSamplesPerSec; /* IN  Sampling rate */
    ULONG       ulChannels;      /* IN  Number of channels */
    ULONG       ulFormatMode;    /* IN  MCI_RECORD or MCI_PLAY */
    ULONG       ulDeviceType;    /* IN  MCI_DEVTYPE */
    ULONG       ulMixHandle;     /* OUT Read/Write handle */
    PMIXERPROC  pmixWrite;       /* OUT Write routine entry point */
    PMIXERPROC  pmixRead;        /* OUT Read routine entry point */
    PMIXEREVENT pmixEvent;       /* IN  Event routine entry point */
    PVOID       pExtendedInfo;   /* IN  Media-specific info */
    ULONG       ulBufferSize;    /* OUT Recommended buffer size */
    ULONG       ulNumBuffers;    /* OUT Recommended num buffers */
} MCI_MIXSETUP_PARMS;

typedef struct_MCI_BUFFER_PARMS {
    HWND   hwndCallback;    /* Window for notifications */
    ULONG  ulStructLength;  /* Length of MCI_BUFFER_PARMS */
    ULONG  ulNumBuffers;    /* Number of buffers to allocate (IN/OUT)*/
    ULONG  ulBufferSize;    /* Size of buffers mixer should use */
    ULONG  ulMintoStart;    /* Unused */
    ULONG  ulSrcStart;      /* Unused */
    ULONG  ulTgtStart;      /* Unused */
    PVOID  pBufList;        /* Pointer to array of buffers */
} MCI_BUFFER_PARMS;

typedef MCI_BUFFER_PARMS *PMCI_BUFFER_PARMS;

typedef struct_MCI_MIX_BUFFER {
    ULONG  ulStructLength;  /* Length of the structure */
    ULONG  pBuffer;         /* Pointer to a buffer */
    ULONG  ulBufferLength;  /* Length of the buffer */
    ULONG  ulFlags;         /* Flags */
    ULONG  ulUserParm;      /* User buffer parameter */
    ULONG  ulTime;          /* Device time in milliseconds */
    ULONG  ulReserved1;     /* Unused */
    ULONG  ulReserved2;     /* Unused */
} MCI_MIX_BUFFER;

typedef MCI_MIX_BUFFER *PMCI_MIX_BUFFER
#endif

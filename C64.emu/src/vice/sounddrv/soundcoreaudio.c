/*
 * soundcoreaudio.c - Implementation of the CoreAudio sound device.
 *
 * Written by
 *  Michael Klein <michael.klein@puffin.lb.shuttle.de>
 *  Christian Vogelgsang <C.Vogelgsang@web.de> (Ported to Intel Mac)
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

/*
 * Notation Hints:
 *
 * Mac OS X Audio:
 *  1 Packet = 1 Frame
 *  1 Frame  = 1 or 2 Samples (SWORD)  1:mono SID, 2:stereo SID
 *  1 Slice  = n Frames
 *
 * VICE Audio:
 *  1 Fragment = n Frames     (n=fragsize)
 *  Soundbuffer = m Fragments (m=fragnum)
 *
 * VICE Fragment = CoreAudio Slice
 */

#include "vice.h"

#ifdef USE_COREAUDIO

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

#include "lib.h"
#include "log.h"
#include "sound.h"

/* Requested audio device name */
CFStringRef requested_device_name_ref = NULL;
char * requested_device_name = NULL;

/* resolved device id */
static AudioDeviceID device = kAudioDeviceUnknown;

/* type for atomic increments */
typedef volatile int atomic_int_t;

/* the cyclic buffer containing m fragments */
static int16_t *soundbuffer;

/* silence fragment */
static int16_t *silence;

/* current read position: no. of fragment in soundbuffer */
static unsigned int read_position;

/* the next position to write: no. of fragment in soundbuffer */
static unsigned int write_position;

/* frames in fragment  */
static unsigned int frames_in_fragment;

/* Size of fragment (bytes).  */
static unsigned int bytes_in_fragment;

/* Size of fragment (SWORDs) */
static unsigned int swords_in_fragment;

/* total number of fragments */
static unsigned int fragment_count;

/* current number of fragments in buffer */
static atomic_int_t fragments_in_queue;

/* number of interleaved channels (mono SID=1, stereo SID=2) */
static int in_channels;

/* samples left in current fragment */
static unsigned int frames_left_in_fragment;

/* bytes per input frame */
static unsigned int in_frame_byte_size;

/* ----- Atomic Increment/Decrement for Thread-Safe Audio Buffers -------- */

#if defined(__x86_64__) || defined(__i386__)
/* Intel Mac Implementation */

static inline void atomic_increment(atomic_int_t * addr)
{
    __asm__ __volatile__ ("lock ; incl %0"
                          :"=m" (*addr)
                          :"m" (*addr));
}

static inline void atomic_decrement(atomic_int_t * addr)
{
    __asm__ __volatile__ ("lock ; decl %0"
                          :"=m" (*addr)
                          :"m" (*addr));
}

#else
/* PowerPC Mac Implementation */

static inline void atomic_add(atomic_int_t * addr, int val)
{
    register int tmp;
    asm volatile("    lwarx  %0,0,%2  \n\t"  /* load value & reserve */
                 "    addc   %0,%0,%3 \n\t"  /* add <val> */
                 "    stwcx. %0,0,%2  \n\n"  /* store new value */
                 "    bne-   $-12"           /* check if store was successful */
                 : "=&r"(tmp), "=m"(addr)
                 : "r"(addr), "r"(val), "m"(addr)
                 : "cr0"
                );
}

static inline void atomic_increment(atomic_int_t * addr)
{
    atomic_add(addr, 1);
}

static inline void atomic_decrement(atomic_int_t * addr)
{
    atomic_add(addr, -1);
}

#endif

/* ----- Audio Converter ------------------------------------------------ */

static AudioConverterRef converter = 0;

static OSStatus converter_input(AudioConverterRef inAudioConverter,
                                UInt32 * ioNumberDataPackets,
                                AudioBufferList * ioData,
                                AudioStreamPacketDescription** outDataPacketDescription,
                                void * inUserData)
{
    UInt32 num_frames = *ioNumberDataPackets;

    int16_t *buffer;
    if (fragments_in_queue) {
        /* too many -> crop to available in current fragment */
        if (num_frames > frames_left_in_fragment) {
            num_frames = frames_left_in_fragment;
        }

        /* calc position in sound buffer */
        int sample_offset_in_fragment = frames_in_fragment - frames_left_in_fragment;
        buffer = soundbuffer + swords_in_fragment * read_position + sample_offset_in_fragment * in_channels;

        /* update the samples left in the current fragment */
        frames_left_in_fragment -= num_frames;

        /* fetch next fragment */
        if (frames_left_in_fragment == 0) {
            read_position = (read_position + 1) % fragment_count;
            atomic_decrement(&fragments_in_queue);
            frames_left_in_fragment = frames_in_fragment;
        }
    } else {
        if (num_frames > frames_in_fragment) {
            num_frames = frames_in_fragment;
        }

        /* output silence */
        buffer = silence;
    }

    /* prepare return buffer */
    ioData->mBuffers[0].mDataByteSize = num_frames * in_frame_byte_size;
    ioData->mBuffers[0].mData = buffer;
    *ioNumberDataPackets = num_frames;

    return kAudioHardwareNoError;
}

static int converter_open(AudioStreamBasicDescription *in,
                          AudioStreamBasicDescription *out)
{
    OSStatus err;

    /* need to to sample rate conversion? */
    if (out->mSampleRate != in->mSampleRate) {
        log_warning(LOG_DEFAULT, "sound (coreaudio_init): sampling rate conversion %dHz->%dHz",
                    (int)in->mSampleRate, (int)out->mSampleRate);
    }

    /* create a new audio converter */
    err = AudioConverterNew(in, out, &converter);
    if (err != noErr) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): could not create AudioConverter: err=%d", (int)err);
        return -1;
    }

    /* duplicate mono stream to all output channels */
    if (in->mChannelsPerFrame == 1 && out->mChannelsPerFrame > 1) {
        Boolean writable;
        UInt32 size;
        err = AudioConverterGetPropertyInfo(converter, kAudioConverterChannelMap, &size, &writable);
        if (err == noErr && writable) {
            SInt32 * channel_map = lib_malloc(size);
            if (channel_map) {
                memset(channel_map, 0, size);
                AudioConverterSetProperty(converter, kAudioConverterChannelMap, size, channel_map);
                lib_free(channel_map);
            }
        }
    }

    return 0;
}

static void converter_close(void)
{
    if (converter) {
        AudioConverterDispose(converter);
        converter = NULL;
    }
}

static int string_buf_size_for_utf8_char_length(int utf8Chars)
{
    return utf8Chars * 4 + 1;
}

#if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_6)
static int determine_output_device_id()
{
    OSStatus err;
    UInt32 size;
    AudioDeviceID default_device;
    AudioObjectPropertyAddress property_address = {
        0,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    /* get default audio device id */
    property_address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    size = sizeof(default_device);
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                     &property_address,
                                     0,
                                     NULL,
                                     &size,
                                     &default_device);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): Failed to get default output device");
        return -1;
    }

    /* use the default audio device unless overridden */
    device = default_device;

    if (requested_device_name) {
        log_message(LOG_DEFAULT, "sound (coreaudio_init): Searching for audio output device: %s", requested_device_name);
    }

    /* list audio devices */
    property_address.mSelector = kAudioHardwarePropertyDevices;
    size = 0;
    err = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &property_address, 0, NULL, &size);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): AudioObjectGetPropertyDataSize (kAudioHardwarePropertyDevices) failed: %d", (int)err);
        return -1;
    }

    UInt32 device_count = size / sizeof(AudioDeviceID);

    AudioDeviceID *audio_devices = (AudioDeviceID *)(lib_calloc(device_count, sizeof(AudioDeviceID)));
    if (audio_devices == NULL) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): Unable to allocate memory");
        return -1;
    }

    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, NULL, &size, audio_devices);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): AudioObjectGetPropertyData (kAudioHardwarePropertyDevices) failed: %d", (int)err);
        lib_free(audio_devices);
        audio_devices = NULL;
        return -1;
    }

    CFStringRef device_name_ref = NULL;
    char * device_name = NULL;
    CFIndex buffer_size = 0;

    /* search list of output devices for matching name */
    for(UInt32 i = 0; i < device_count; ++i) {
        property_address.mSelector = kAudioDevicePropertyStreams;
        property_address.mScope = kAudioDevicePropertyScopeOutput;

        size = 0;
        err = AudioObjectGetPropertyDataSize(audio_devices[i], &property_address, 0, NULL, &size);
        if (err != kAudioHardwareNoError) {
            log_error(LOG_DEFAULT, "sound (coreaudio_init): AudioObjectGetPropertyDataSize (kAudioDevicePropertyStreamConfiguration) failed: %d", (int)err);
            lib_free(audio_devices);
            audio_devices = NULL;
            return -1;
        }

        bool outputs_found = size > 0;

        /* get device name */
        size = sizeof(device_name_ref);
        property_address.mSelector = kAudioDevicePropertyDeviceNameCFString;
        err = AudioObjectGetPropertyData(audio_devices[i], &property_address, 0, NULL, &size, &device_name_ref);
        if (err != kAudioHardwareNoError) {
            log_error(LOG_DEFAULT, "sound (coreaudio_init): AudioObjectGetPropertyData (kAudioDevicePropertyDeviceNameCFString) failed: %d", (int)err);
            lib_free(audio_devices);
            audio_devices = NULL;
            return -1;
        }

        buffer_size = string_buf_size_for_utf8_char_length(CFStringGetLength(device_name_ref));
        device_name = lib_calloc(1, buffer_size);

        if(!CFStringGetCString(device_name_ref, device_name, buffer_size, kCFStringEncodingUTF8)) {
            strcpy(device_name, "");
        }

        if (!outputs_found) {
            log_message(LOG_DEFAULT, "sound (coreaudio_init): Found audio device with no outputs: %s", device_name);
            lib_free(device_name);
            continue;
        }

        if (audio_devices[i] == default_device) {
            log_message(LOG_DEFAULT, "sound (coreaudio_init): Found output audio device: %s (Default)", device_name);
        } else {
            log_message(LOG_DEFAULT, "sound (coreaudio_init): Found output audio device: %s", device_name);
        }

        if (requested_device_name == NULL) {
            lib_free(device_name);
            continue;
        }

        if (kCFCompareEqualTo == CFStringCompare(requested_device_name_ref, device_name_ref, 0)) {
            /* matches the requested audio device */
            device = audio_devices[i];
        }

        lib_free(device_name);
    }

    lib_free(audio_devices);
    audio_devices = NULL;

    /* get final device name */
    device_name_ref = NULL;
    size = sizeof(device_name_ref);
    property_address.mSelector = kAudioDevicePropertyDeviceNameCFString;
    err = AudioObjectGetPropertyData(device, &property_address, 0, NULL, &size, &device_name_ref);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): AudioObjectGetPropertyData (kAudioDevicePropertyDeviceNameCFString) failed: %d", (int)err);
        return -1;
    }

    buffer_size = string_buf_size_for_utf8_char_length(CFStringGetLength(device_name_ref));
    device_name = lib_calloc(1, buffer_size);

    if(!CFStringGetCString(device_name_ref, device_name, buffer_size, kCFStringEncodingUTF8)) {
        strcpy(device_name, "");
    }

    log_message(LOG_DEFAULT, "sound (coreaudio_init): Using output audio device: %s", device_name);

    lib_free(device_name);

    return 0;
}
#endif

/* ----- Audio API before AudioUnits ------------------------------------- */

#ifndef HAVE_AUDIO_UNIT

/* bytes per output frame */
static unsigned int out_frame_byte_size;

/* proc id */
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
static AudioDeviceIOProcID procID;
#endif

static OSStatus audio_render(AudioDeviceID device,
                             const AudioTimeStamp  * now,
                             const AudioBufferList * input_data,
                             const AudioTimeStamp  * input_time,
                             AudioBufferList       * output_data,
                             const AudioTimeStamp  * output_time,
                             void                  * client_data)
{
    /* get the number of frames(=packets) in the output buffer */
    UInt32 bufferPacketSize = output_data->mBuffers[0].mDataByteSize / out_frame_byte_size;

    OSStatus result = AudioConverterFillComplexBuffer(converter,
                                                      converter_input,
                                                      NULL,
                                                      &bufferPacketSize,
                                                      output_data,
                                                      NULL);

    return result;
}

static int audio_open(AudioStreamBasicDescription *in)
{
    OSStatus err;
    UInt32 size;

    if (0 != determine_output_device_id()) {
        return -1;
    }

    AudioStreamBasicDescription out;
    /* get default output format */
    size = sizeof(out);
    err = AudioDeviceGetProperty(device, 0, false,
                                 kAudioDevicePropertyStreamFormat,
                                 &size, (void*)&out);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): stream format not support");
        return -1;
    }
    /* store size of output frame */
    out_frame_byte_size = out.mBytesPerPacket;

    /* setup audio renderer callback */
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
    err = AudioDeviceCreateIOProcID( device, audio_render, NULL, &procID );
#else
    err = AudioDeviceAddIOProc( device, audio_render, NULL );
#endif
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): could not add IO proc: err=%d", (int)err);
        return -1;
    }

    /* open audio converter */
    return converter_open(in, &out);
}

static void audio_close(void)
{
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
    AudioDeviceDestroyIOProcID(device, procID);
#else
    AudioDeviceRemoveIOProc(device, audio_render);
#endif

    converter_close();
}

static int audio_start(void)
{
    OSStatus err = AudioDeviceStart(device, audio_render);
    if (err != kAudioHardwareNoError) {
        return -1;
    } else {
        return 0;
    }
}

static int audio_stop(void)
{
    OSStatus err = AudioDeviceStop(device, audio_render);
    if (err != kAudioHardwareNoError) {
        return -1;
    } else {
        return 0;
    }
}

#else /* HAVE_AUDIO_UNIT */
/* ------ Audio Unit API ------------------------------------------------- */

#ifdef DEBUG
/* FIXME: this hack fixes the processing of <CoreServices/CoreServices.h>
   during a debug build. */
#undef DEBUG
#define DEBUG 1
#endif

#include <AudioUnit/AudioUnit.h>
#include <CoreServices/CoreServices.h>

static AudioUnit outputUnit;

static OSStatus audio_render(void *inRefCon,
                             AudioUnitRenderActionFlags  *ioActionFlags,
                             const AudioTimeStamp        *inTimeStamp,
                             UInt32 inBusNumber,
                             UInt32 inNumberFrames,
                             AudioBufferList             *ioData)
{
    UInt32 numFrames = inNumberFrames;
    return AudioConverterFillComplexBuffer(converter,
                                           converter_input,
                                           NULL,
                                           &numFrames,
                                           ioData,
                                           NULL);
}

#if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_6)
static int audio_open(AudioStreamBasicDescription *in)
{
    OSStatus err;
    AudioComponentDescription desc;
    AudioStreamBasicDescription out;
    UInt32 size;
    AudioComponent output_component;

    /* find the default audio component */
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    output_component = AudioComponentFindNext(NULL, &desc);
    if (output_component == NULL) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): can't find HAL output component");
        return -1;
    }

    /* open audio component and enable output IO */
    err = AudioComponentInstanceNew(output_component, &outputUnit);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error opening output unit");
        return -1;
    }

    /* select output device */
    if(0 != determine_output_device_id()) {
        return -1;
    }

    err = AudioUnitSetProperty(outputUnit,
                               kAudioOutputUnitProperty_CurrentDevice,
                               kAudioUnitScope_Global,
                               0,
                               &device,
                               sizeof(device));

    if (err) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): error setting device id");
        return -1;
    }

    /* Set up a callback function to generate output to the output unit */
    AURenderCallbackStruct input;
    input.inputProc = audio_render;
    input.inputProcRefCon = NULL;
    err = AudioUnitSetProperty(outputUnit,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Input,
                               0,
                               &input,
                               sizeof(input));
    if (err) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): error setting render callback");
        return -1;
    }

    /* Get output properties */
    size = sizeof(AudioStreamBasicDescription);
    err = AudioUnitGetProperty(outputUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0,
                               &out,
                               &size);
    if (err) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): error setting desired input format");
        return -1;
    }

    /* Init unit */
    err = AudioUnitInitialize(outputUnit);
    if (err) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): error initializing audio unit");
        return -1;
    }

    /* open converter */
    return converter_open(in, &out);
}

static void audio_close(void)
{
    OSStatus err;

    converter_close();

    /* Uninit unit */
    err = AudioUnitUninitialize(outputUnit);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_close): error uninitializing audio unit");
    }

    /* Close component */
    AudioComponentInstanceDispose(outputUnit);
}
#else
static int audio_open(AudioStreamBasicDescription *in)
{
    OSStatus err;
    UInt32 size;
    AudioStreamBasicDescription out;

    /* get default audio device */
    size = sizeof(device);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,
                                   &size, (void*)&device);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): Failed to get default output device");
        return -1;
    }

    /* get default output format */
    size = sizeof(out);
    err = AudioDeviceGetProperty(device, 0, false,
                                 kAudioDevicePropertyStreamFormat,
                                 &size, (void*)&out);
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): stream format not support");
        return -1;
    }
    /* store size of output frame */
    out_frame_byte_size = out.mBytesPerPacket;

    /* setup audio renderer callback */
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
    err = AudioDeviceCreateIOProcID( device, audio_render, NULL, &procID );
#else
    err = AudioDeviceAddIOProc( device, audio_render, NULL );
#endif
    if (err != kAudioHardwareNoError) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_init): could not add IO proc: err=%d", (int)err);
        return -1;
    }

    /* open audio converter */
    return converter_open(in, &out);
}

static void audio_close(void)
{
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
    AudioDeviceDestroyIOProcID(device, procID);
#else
    AudioDeviceRemoveIOProc(device, audio_render);
#endif

    converter_close();
}
#endif

static int audio_start(void)
{
    OSStatus err = AudioOutputUnitStart(outputUnit);
    if (err) {
        return -1;
    } else {
        return 0;
    }
}

static int audio_stop(void)
{
    OSStatus err = AudioOutputUnitStop(outputUnit);
    if (err) {
        return -1;
    } else {
        return 0;
    }
}

#endif /* HAVE_AUDIO_UNIT */

/* ---------- coreaudio VICE API ------------------------------------------ */

static int coreaudio_resume(void);

static int coreaudio_init(const char *param, int *speed,
                          int *fragsize, int *fragnr, int *channels)
{
    AudioStreamBasicDescription in;
    int result;

    /* store fragment parameters */
    if (param) {
        requested_device_name_ref = CFStringCreateWithCString(NULL, param, kCFStringEncodingUTF8);

        CFIndex buffer_size = string_buf_size_for_utf8_char_length(CFStringGetLength(requested_device_name_ref));
        requested_device_name = lib_calloc(1, buffer_size);

        if(!CFStringGetCString(requested_device_name_ref, requested_device_name, buffer_size, kCFStringEncodingUTF8)) {
            strcpy(requested_device_name, "");
        }
    }
    fragment_count = *fragnr;
    frames_in_fragment = *fragsize;
    in_channels = *channels;

    /* the size of a fragment in bytes and SWORDs */
    swords_in_fragment = frames_in_fragment * in_channels;
    bytes_in_fragment = swords_in_fragment * sizeof(int16_t);

    /* the size of a sample */
    in_frame_byte_size = sizeof(int16_t) * in_channels;

    /* allocate sound buffers */
    soundbuffer = lib_calloc(fragment_count, bytes_in_fragment);
    silence = lib_calloc(1, bytes_in_fragment);

    /* define desired input format */
    in.mChannelsPerFrame = *channels;
    in.mSampleRate = (float)*speed;
    in.mFormatID = kAudioFormatLinearPCM;
#if defined(__x86_64__) || defined(__i386__)
    in.mFormatFlags = kAudioFormatFlagIsSignedInteger;
#else
    in.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsBigEndian;
#endif
    in.mBytesPerFrame = sizeof(int16_t) * *channels;
    in.mBytesPerPacket = in.mBytesPerFrame;
    in.mFramesPerPacket = 1;
    in.mBitsPerChannel = 8 * sizeof(int16_t);
    in.mReserved = 0;

    /* setup audio device */
    result = audio_open(&in);
    if (result < 0) {
        return result;
    }

    coreaudio_resume();

    return 0;
}

static int coreaudio_write(int16_t *pbuf, size_t nr)
{
    int i, count;

    /* number of fragments */
    count = nr / swords_in_fragment;

    for (i = 0; i < count; i++)
    {
        if (fragments_in_queue == fragment_count) {
            log_warning(LOG_DEFAULT, "sound (coreaudio): buffer overrun");
            return -1;
        }

        memcpy(soundbuffer + swords_in_fragment * write_position,
               pbuf + i * swords_in_fragment,
               bytes_in_fragment);

        write_position = (write_position + 1) % fragment_count;

        atomic_increment(&fragments_in_queue);
    }

    return 0;
}

static int coreaudio_bufferspace(void)
{
    return (fragment_count - fragments_in_queue) * frames_in_fragment;
}

static void coreaudio_close(void)
{
    audio_close();

    lib_free(soundbuffer);
    lib_free(silence);

    if (requested_device_name_ref) {
        CFRelease(requested_device_name_ref);
        requested_device_name_ref = NULL;
    }

    if(requested_device_name) {
        lib_free(requested_device_name);
        requested_device_name = NULL;
    }
}

static int coreaudio_suspend(void)
{
    int result = audio_stop();
    if (result < 0) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): could not stop audio");
    }
    return result;
}

static int coreaudio_resume(void)
{
    int result;

    /* reset buffers before resume */
    read_position = 0;
    write_position = 0;
    fragments_in_queue = 0;
    frames_left_in_fragment = frames_in_fragment;

    result = audio_start();
    if (result < 0) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): could not start audio");
    }
    return result;
}

static sound_device_t coreaudio_device =
{
    "coreaudio",
    coreaudio_init,
    coreaudio_write,
    NULL,
    NULL,
    coreaudio_bufferspace,
    coreaudio_close,
    coreaudio_suspend,
    coreaudio_resume,
    1,
    2
};

int sound_init_coreaudio_device(void)
{
    return sound_register_device(&coreaudio_device);
}
#endif

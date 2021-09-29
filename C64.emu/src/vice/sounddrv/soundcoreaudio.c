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
#include <stdatomic.h>
#include <libkern/OSAtomic.h>

#include "lib.h"
#include "log.h"
#include "sound.h"
#include "tick.h"
#include "vsyncapi.h"

/* Requested audio device name */
CFStringRef requested_device_name_ref = NULL;
char *requested_device_name = NULL;

/* resolved device id */
static AudioDeviceID device = kAudioDeviceUnknown;


/* the cyclic buffer containing m fragments */
static volatile int16_t *ringbuffer;

/* the buffer used to pass non cyclic data to the driver  */
static volatile int16_t *copybuffer;
static volatile int copybuffer_size_bytes;

/* current read position: no. of fragment in soundbuffer */
static volatile int read_position_2;

/* the next position to write: no. of fragment in soundbuffer */
static volatile int write_position_2;

/* current number of fragments in buffer */
static volatile int fragments_in_queue_2;

/* samples left in current fragment */
static volatile int frames_left_in_fragment_2;



/* frames in fragment  */
static unsigned int frames_in_fragment;

/* Size of fragment (bytes).  */
static unsigned int bytes_in_fragment;

/* Size of fragment (SWORDs) */
static unsigned int swords_in_fragment;

/* total number of fragments */
static unsigned int fragment_count;

/* number of interleaved channels (mono SID=1, stereo SID=2) */
static int in_channels;

/* bytes per input frame */
static unsigned int in_frame_byte_size;

/* How many times the audio code didn't get enough audio */
static unsigned long underflow_count;

#if 0
static void coreaudio_buffer_stats(bool force)
{
    static unsigned long last_log_time_sec;

    unsigned long this_log_time_sec;
    unsigned long now;

    now = tick_now();
    this_log_time_sec = now / (tick_per_second());

    if (!force && this_log_time_sec == last_log_time_sec) {
        return;
    }

    last_log_time_sec = this_log_time_sec;

    log_message(LOG_DEFAULT, "%d of %d (underflows: %lu)", fragments_in_queue_2, fragment_count, underflow_count);
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
    UInt32 needed_frames = *ioNumberDataPackets;
    int needed_bytes = needed_frames * in_frame_byte_size;
    int this_read_position  = read_position_2;
    int consumed_fragments = 0;
    
    int16_t *source;
    int16_t *dest;
    
    /* ensure our copy buffer is large enough */
    if (needed_bytes > copybuffer_size_bytes) {
        lib_free((void *)copybuffer);
        copybuffer_size_bytes = needed_bytes;
        copybuffer = lib_malloc(copybuffer_size_bytes);
        log_message(LOG_DEFAULT, "Copybuffer increase to %d bytes", copybuffer_size_bytes);
    }
    
    /* prepare return buffer */
    ioData->mBuffers[0].mNumberChannels = in_channels;
    ioData->mBuffers[0].mData = (void *)copybuffer;
    ioData->mBuffers[0].mDataByteSize = needed_bytes;
    ioData->mNumberBuffers = 1;
    
    dest = (int16_t *)copybuffer;
    
    while (fragments_in_queue_2 && needed_frames) {
        /*
         * Assemble a single block of samples for the converter
         */

        /* calc position in ring buffer */
        int sample_offset_in_fragment = frames_in_fragment - frames_left_in_fragment_2;
        source = (int16_t *)ringbuffer + (swords_in_fragment * this_read_position) + (sample_offset_in_fragment * in_channels);

        if (needed_frames < frames_left_in_fragment_2) {
            /* The current fragement has more than enough, so it will be read from again next time. */
            memcpy(dest, source, needed_frames * in_frame_byte_size);

            dest += needed_frames * in_channels;
            frames_left_in_fragment_2 -= needed_frames;

            needed_frames = 0;
        } else {
            /* We'll need all the audio in the current frament. */
            memcpy(dest, source, frames_left_in_fragment_2 * in_frame_byte_size);

            dest += frames_left_in_fragment_2 * in_channels;
            needed_frames -= frames_left_in_fragment_2;

            this_read_position = (this_read_position + 1) % fragment_count;

            consumed_fragments++;
            frames_left_in_fragment_2 = frames_in_fragment;
        }
    }
    
    if (needed_frames) {
        /* Underflow */
        underflow_count++;
        *ioNumberDataPackets = 0;
        
        //memset(dest, 0, needed_frames * in_frame_byte_size);
    } else {
        /*
        * There was enough data, apply the read.
        */
        
        while(consumed_fragments--) {
            OSAtomicDecrement32(&fragments_in_queue_2);
            read_position_2 = this_read_position;
        }
    }

    return kAudioHardwareNoError;
}

static int converter_open(AudioStreamBasicDescription *in,
                          AudioStreamBasicDescription *out)
{
    OSStatus err;

    /* need to to sample rate conversion? */
    if (out->mSampleRate != in->mSampleRate) {
        log_message(LOG_DEFAULT, "sound (coreaudio_init): sampling rate conversion %dHz->%dHz",
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

static CFIndex string_buf_size_for_utf8_char_length(CFIndex utf8Chars)
{
    return utf8Chars * 4 + 1;
}

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
    bool requested_device_found = false;

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
            requested_device_found = true;
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
        outputUnit = NULL;
        return -1;
    }

    /* select output device */
    if(determine_output_device_id()) {
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
    
    /*
     * For accuracy, we want to convert directly from the emulator output
     * to the format used by the audio hardware, in a single step.
     */

    /* Get hardware output properties */
    size = sizeof(AudioStreamBasicDescription);
    err = AudioUnitGetProperty(outputUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Output,
                               0,
                               &out,
                               &size);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error getting default input format");
        return -1;
    }
    
    /* Tell the AudioUnit that we'll give it samples in the same format */
    err = AudioUnitSetProperty(outputUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0,
                               &out,
                               size);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error setting desired sample rate");
    }
    
    /* Get the final format that we'll need to use - the above may have failed */
    err = AudioUnitGetProperty(outputUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0,
                               &out,
                               &size);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error getting final output format");
        return -1;
    }

    /* Determine the range of supported buffer sizes */
    AudioObjectPropertyAddress frame_size_range_address = {
        kAudioDevicePropertyBufferFrameSizeRange,
        kAudioObjectPropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };
    AudioValueRange range = {0, 0};
    size = sizeof(AudioValueRange);

    err = AudioObjectGetPropertyData(device, &frame_size_range_address, 0, NULL, &size, &range);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error getting kAudioDevicePropertyBufferFrameSizeRange");
        return -1;
    }
    
    size = range.mMinimum > frames_in_fragment ? range.mMinimum : frames_in_fragment;
    
    log_message(LOG_DEFAULT, "sound (coreaudio_init): audio frame buffer size in samples min: %f max: %f fragment size: %u, chosen: %d", range.mMinimum, range.mMaximum, frames_in_fragment, size);

    /* set the buffer size */
    AudioObjectPropertyAddress buffer_size_address = {
        kAudioDevicePropertyBufferFrameSize,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    err = AudioObjectSetPropertyData(device, &buffer_size_address, 0, NULL, sizeof(UInt32), &size);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error setting buffer size to %d", size);
        return -1;
    }
    err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &size, sizeof(size));
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error setting max frames per slice to %d", size);
        return -1;
    }
    
    /* open converter */
    err = converter_open(in, &out);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_init): error initializing audio converter");
        return -1;
    }

    return 0;
}

static int audio_start(void)
{
    OSStatus err;
    
    /* Init unit */
    err = AudioUnitInitialize(outputUnit);
    if (err) {
        log_error(LOG_DEFAULT,
                  "sound (coreaudio_start): error initializing audio unit");
        return -1;
    }
    
    err = AudioOutputUnitStart(outputUnit);
    if (err) {
        log_error(LOG_DEFAULT, "sound (coreaudio_start): failed to start audio device");
        return -1;
    }
    
    log_message(LOG_DEFAULT, "sound (coreaudio_start): Started");
    return 0;
}

static int audio_stop(void)
{
    OSStatus err;
    
    err = AudioOutputUnitStop(outputUnit);
    if (err) {
        log_error(LOG_DEFAULT, "sound (audio_stop): error uninitializing audio unit");
        return -1;
    }
    
    err = AudioUnitUninitialize(outputUnit);
    if (err) {
        log_error(LOG_DEFAULT, "sound (audio_stop): error uninitializing audio unit");
        return -1;
    }

    return 0;
}

static void audio_close(void)
{
    OSStatus err;
    
    if (outputUnit) {
        audio_stop();
        
        log_message(LOG_DEFAULT, "AudioComponentInstanceDispose");
        err = AudioComponentInstanceDispose(outputUnit);
        if (err) {
            log_error(LOG_DEFAULT, "sound (audio_close): error disposing of audio unit");
        }

        outputUnit = NULL;
    }
    
    converter_close();
}

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
    ringbuffer = lib_calloc(fragment_count, bytes_in_fragment);

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
    int i;
    size_t count;

    /* number of fragments */
    count = nr / swords_in_fragment;

    for (i = 0; i < count; i++)
    {
        if (fragments_in_queue_2 == fragment_count) {
            log_warning(LOG_DEFAULT, "sound (coreaudio): buffer overrun");
            return 0;
        }

        memcpy((int16_t *)ringbuffer + (swords_in_fragment * write_position_2),
               pbuf + (i * swords_in_fragment),
               bytes_in_fragment);

        write_position_2 = (write_position_2 + 1) % fragment_count;

        OSAtomicIncrement32(&fragments_in_queue_2);
    }
    
    /* coreaudio_buffer_stats(false); */

    return 0;
}

static int coreaudio_bufferspace(void)
{
    return (fragment_count - fragments_in_queue_2) * frames_in_fragment;
}

static void coreaudio_close(void)
{
    audio_close();

    lib_free((void *)ringbuffer);
    lib_free((void *)copybuffer);

    ringbuffer = NULL;
    copybuffer = NULL;
    copybuffer_size_bytes = 0;

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
    read_position_2 = 0;
    write_position_2 = 0;
    fragments_in_queue_2 = 0;
    frames_left_in_fragment_2 = frames_in_fragment;

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
    2,
    true
};

int sound_init_coreaudio_device(void)
{
    return sound_register_device(&coreaudio_device);
}

#endif /* USE_COREAUDIO_SUPPORT */

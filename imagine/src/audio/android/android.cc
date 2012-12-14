/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "audio:android"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/jni.hh>
#include <base/android/private.hh>

namespace Audio
{
using namespace Base;

// Android API constants
namespace AudioManager
{
	static const int STREAM_MUSIC = 3,
			USE_DEFAULT_STREAM_TYPE = 0x80000000;
};

namespace AudioTrack
{
	static const int MODE_STREAM = 1;
};

namespace AudioFormat
{
	static const int ENCODING_PCM_16BIT = 2, ENCODING_PCM_8BIT = 3,
		CHANNEL_CONFIGURATION_MONO = 2, CHANNEL_CONFIGURATION_STEREO = 3;
};

PcmFormat preferredPcmFormat { 44100, &SampleFormats::s16, 2 };
PcmFormat pcmFormat;
static bool pcmOpen = 0;

// JNI classes/methods
static jclass audioTrackCls;
static JavaInstMethod<void> jAudioTrack, jPlay, jStop, jRelease;
static JavaInstMethod<jint> jWrite, jGetPlaybackHeadPosition;//, jSetPlaybackRate;
static JavaClassMethod<jint> jGetMinBufferSize;
static jobject audioTrk = nullptr;

static int trkFramesWritten, // total frames written to track since opening
	trkBufferSize, // audio buffer size in bytes
	trkBufferFrames; // trkBufferSize in frames
static jbyteArray jSamples; // array in jvm for writing samples
static jboolean jSamplesArrayIsCopy;
static const uint jSamplesBytes = 4096;
static uint jSamplesFrames;
static BufferContext audioBuffLockCtx;
static bool isPlaying = 0;

CallResult openPcm(const PcmFormat &format)
{
	if(isOpen())
	{
		closePcm();
	}
	//logMsg("opening pcm");

	assert(audioTrk == nullptr);
	using namespace AudioFormat;
	int channelConfig = format.channels == 1 ? CHANNEL_CONFIGURATION_MONO : CHANNEL_CONFIGURATION_STEREO;
	int audioFormat = (format.sample->bits == 16) ? ENCODING_PCM_16BIT : ENCODING_PCM_8BIT;

	trkBufferSize = 2*jGetMinBufferSize(eEnv(), format.rate, channelConfig, audioFormat);
	trkBufferFrames = format.bytesToFrames(trkBufferSize);
	audioTrk = eEnv()->NewObject(audioTrackCls, jAudioTrack.m, AudioManager::STREAM_MUSIC, format.rate,
		channelConfig, audioFormat, trkBufferSize, AudioTrack::MODE_STREAM);
	logMsg("created %dHz track with buffer size %d, frames %d", format.rate, trkBufferSize, trkBufferFrames);
	jthrowable exc = eEnv()->ExceptionOccurred();
	if(exc)
	{
		logErr("exception while creating track");
		audioTrk = nullptr;
		eEnv()->ExceptionClear();
		return INVALID_PARAMETER;
	}
	audioTrk = eEnv()->NewGlobalRef(audioTrk);
	trkFramesWritten = 0;
	jSamplesFrames = format.bytesToFrames(jSamplesBytes);
	//logMsg("opened audio @ %dHz", format.rate);
	//jEnv->CallIntMethod(audioTrk, jSetPlaybackRate.m, format.rate);
	pcmFormat = format;
	return OK;
}

void closePcm()
{
	if(audioTrk)
	{
		logMsg("closing pcm");
		//jEnv->CallVoidMethod(audioTrk, jStop.m);
		jRelease(eEnv(), audioTrk);
		eEnv()->DeleteGlobalRef(audioTrk);
		audioTrk = nullptr;
		isPlaying = 0;
		mem_zero(pcmFormat);
	}
	else
		logMsg("called closePcm when pcm already off");
}

bool isOpen()
{
	return audioTrk != nullptr;
}

BufferContext *getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!isOpen()))
		return 0;
	audioBuffLockCtx.frames = IG::min(wantedFrames, jSamplesFrames);
	return &audioBuffLockCtx;
}

static void writeToTrack(uint frames)
{
	uint free = framesFree();
	if(frames > free)
	{
		//logMsg("trying to write %d frames but only %d free", frames, free);
		if(unlikely(!free))
			return;
		frames = free;
	}
	else
	{
		//logMsg("trying to write %d frames, %d delay", frames, audio_frameDelay());
	}
	int written = jWrite(eEnv(), audioTrk, jSamples, 0, pcmFormat.framesToBytes(frames));
	trkFramesWritten += pcmFormat.bytesToFrames(written);

	static int debugCount = 0;
	/*if(countToValueLooped(debugCount, 120))
	{
		logMsg("%d frames free", free);
	}*/

	if(unlikely(!isPlaying && trkFramesWritten >= trkBufferFrames))
	{
		logMsg("starting playback after %d frames written", trkFramesWritten);
		jPlay(eEnv(), audioTrk);
		isPlaying = 1;
	}

	if(!isPlaying)
	{
		//logMsg("%d frames written out of %d", trkFramesWritten, trkBufferFrames);
	}
}

void commitPlayBuffer(BufferContext *buffer, uint frames)
{
	//logMsg("commit %d frames", frames);
	assert(frames <= buffer->frames);
	if(unlikely(jSamplesArrayIsCopy))
		eEnv()->ReleaseByteArrayElements(jSamples, (jbyte*)audioBuffLockCtx.data, JNI_COMMIT);
	writeToTrack(frames);
}

void writePcm(uchar *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	framesToWrite = IG::min(framesToWrite, jSamplesFrames);
	eEnv()->SetByteArrayRegion(jSamples, 0, pcmFormat.framesToBytes(framesToWrite), (jbyte*)samples);
	writeToTrack(framesToWrite);
}

int frameDelay()
{
	if(likely(audioTrk != 0))
		return trkFramesWritten - jGetPlaybackHeadPosition(eEnv(), audioTrk);
	else
		return 0;
}

int framesFree()
{
	if(likely(audioTrk != 0))
		return trkBufferFrames - frameDelay();
	else
		return 0;
}

void setHintPcmFramesPerWrite(uint frames) { }
void setHintPcmMaxBuffers(uint maxBuffers) { }
uint hintPcmMaxBuffers() { return 0; }

CallResult init()
{
	//logMsg("init audio");
	JavaInstMethod<void> jSetVolumeControlStream;
	jSetVolumeControlStream.setup(eEnv(), jBaseActivityCls, "setVolumeControlStream", "(I)V");
	jSetVolumeControlStream(eEnv(), jBaseActivity, AudioManager::STREAM_MUSIC);
	audioTrackCls = (jclass)eEnv()->NewGlobalRef(eEnv()->FindClass("android/media/AudioTrack"));
	jAudioTrack.setup(eEnv(), audioTrackCls, "<init>", "(IIIIII)V");
	jGetMinBufferSize.setup(eEnv(), audioTrackCls, "getMinBufferSize", "(III)I");
	jPlay.setup(eEnv(), audioTrackCls, "play", "()V");
	jStop.setup(eEnv(), audioTrackCls, "stop", "()V");
	jWrite.setup(eEnv(), audioTrackCls, "write", "([BII)I");
	//jSetPlaybackRate.setup(jEnv, audioTrackCls, "setPlaybackRate", "(I)I");
	jGetPlaybackHeadPosition.setup(eEnv(), audioTrackCls, "getPlaybackHeadPosition", "()I");
	jRelease.setup(eEnv(), audioTrackCls, "release", "()V");
	jSamples = (jbyteArray)eEnv()->NewGlobalRef(eEnv()->NewByteArray(jSamplesBytes));
	audioBuffLockCtx.data = eEnv()->GetByteArrayElements(jSamples, &jSamplesArrayIsCopy);
	if(jSamplesArrayIsCopy) logWarn("jSamples array is a copy");
	if(!audioBuffLockCtx.data)
	{
		logErr("couldn't get audio buffer pointer, error in GetByteArrayElements");
		return INVALID_PARAMETER;
	}

	// Update the native sample rate if better than 44.1Khz, but no more than 48Khz
	// Supposedly Audioflinger will just resample back to 44.1Khz (as of Android 4.0) so this isn't useful
	/*JavaClassMethod jGetNativeOutputSampleRate;
	jGetNativeOutputSampleRate.setup(jEnv, audioTrackCls, "getNativeOutputSampleRate", "(I)I");
	int nativeRate = jEnv->CallStaticIntMethod(jGetNativeOutputSampleRate.c, jGetNativeOutputSampleRate.m, AudioManager.STREAM_MUSIC);
	logMsg("native output rate %d", nativeRate);
	if(nativeRate > 44100 && nativeRate <= 48000)
	{
		pPCM.rate = nativeRate;
	}*/
	return OK;
}

}

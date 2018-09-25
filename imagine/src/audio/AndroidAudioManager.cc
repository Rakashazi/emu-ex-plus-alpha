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

#include <imagine/audio/AudioManager.hh>
#include <imagine/util/jni.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include "../base/android/android.hh"

namespace AudioManager
{

static jobject audioManager{};
static JavaInstMethod<jint(jobject, jint, jint)> jRequestAudioFocus{};
static JavaInstMethod<jint(jobject)> jAbandonAudioFocus{};
static bool soloMix_ = true;
static bool sessionActive = false;
static int outputBufferFrames = -1;
static Audio::PcmFormat nativeFmt{0, Audio::SampleFormats::s16, 2};

static int audioManagerIntProperty(JNIEnv* env, JavaInstMethod<jobject(jstring)> &jGetProperty, const char *propStr)
{
	auto propJStr = env->NewStringUTF(propStr);
	auto valJStr = (jstring)jGetProperty(env, audioManager, propJStr);
	if(!valJStr)
	{
		logWarn("%s is null", propStr);
		return 0;
	}
	auto valStr = env->GetStringUTFChars(valJStr, nullptr);
	int val = atoi(valStr);
	env->ReleaseStringUTFChars(valJStr, valStr);
	return val;
}

static void setupAudioManagerJNI(JNIEnv* env)
{
	using namespace Base;
	if(!audioManager)
	{
		JavaInstMethod<jobject()> jAudioManager{env, jBaseActivityCls, "audioManager", "()Landroid/media/AudioManager;"};
		audioManager = jAudioManager(env, jBaseActivity);
		assert(audioManager);
		audioManager = env->NewGlobalRef(audioManager);
		jclass jAudioManagerCls = env->GetObjectClass(audioManager);
		jRequestAudioFocus.setup(env, jAudioManagerCls, "requestAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;II)I");
		jAbandonAudioFocus.setup(env, jAudioManagerCls, "abandonAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;)I");
	}
}

static void requestAudioFocus(JNIEnv* env)
{
	setupAudioManagerJNI(env);
	auto res = jRequestAudioFocus(env, audioManager, Base::jBaseActivity, 3, 1);
	//logMsg("%d from requestAudioFocus()", (int)res);
}

static void abandonAudioFocus(JNIEnv* env)
{
	setupAudioManagerJNI(env);
	jAbandonAudioFocus(env, audioManager, Base::jBaseActivity);
}

static void setAudioManagerProperties()
{
	using namespace Base;
	auto env = jEnvForThread();
	setupAudioManagerJNI(env);
	jclass jAudioManagerCls = env->GetObjectClass(audioManager);
	JavaInstMethod<jobject(jstring)> jGetProperty{env, jAudioManagerCls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;"};
	nativeFmt.rate = audioManagerIntProperty(env, jGetProperty, "android.media.property.OUTPUT_SAMPLE_RATE");
	if(nativeFmt.rate != 44100 && nativeFmt.rate != 48000)
	{
		// only support 44KHz and 48KHz for now
		logMsg("ignoring native sample rate: %d", nativeFmt.rate);
		nativeFmt.rate = 44100;
	}
	else
	{
		logMsg("set native sample rate: %d", nativeFmt.rate);
		// find the preferred buffer size for this rate if device has low-latency support
		JavaInstMethod<jboolean()> jHasLowLatencyAudio{env, jBaseActivityCls, "hasLowLatencyAudio", "()Z"};
		if(jHasLowLatencyAudio(env, jBaseActivity))
		{
			outputBufferFrames = audioManagerIntProperty(env, jGetProperty, "android.media.property.OUTPUT_FRAMES_PER_BUFFER");
			if(!outputBufferFrames)
				logMsg("native buffer frames value not present");
			else
				logMsg("set native buffer frames: %d", outputBufferFrames);
		}
		else
		{
			outputBufferFrames = 0;
			logMsg("no low-latency support");
		}
	}
}

Audio::PcmFormat nativeFormat()
{
	if(unlikely(!nativeFmt))
	{
		if(Base::androidSDK() >= 17)
		{
			setAudioManagerProperties();
		}
		else
		{
			nativeFmt.rate = 44100;
		}
	}
	assumeExpr(nativeFmt);
	return nativeFmt;
}

uint nativeOutputFramesPerBuffer()
{
	if(unlikely(outputBufferFrames == -1))
	{
		if(Base::androidSDK() >= 17)
		{
			setAudioManagerProperties();
		}
		else
		{
			outputBufferFrames = 0;
		}
	}
	assumeExpr(outputBufferFrames >= 0);
	return outputBufferFrames;
}

bool hasLowLatency()
{
	return nativeOutputFramesPerBuffer();
}

void setSoloMix(bool newSoloMix)
{
	if(soloMix_ != newSoloMix)
	{
		logMsg("setting solo mix: %d", newSoloMix);
		soloMix_ = newSoloMix;
		if(sessionActive)
		{
			// update the current audio focus
			if(newSoloMix)
				requestAudioFocus(Base::jEnvForThread());
			else
				abandonAudioFocus(Base::jEnvForThread());
		}
	}
}

bool soloMix()
{
	return soloMix_;
}

void setMusicVolumeControlHint()
{
	using namespace Base;
	auto env = jEnvForThread();
	JavaInstMethod<void(jint)> jSetVolumeControlStream{env, jBaseActivityCls, "setVolumeControlStream", "(I)V"};
	jSetVolumeControlStream(env, jBaseActivity, 3);
}

void startSession()
{
	if(sessionActive)
		return;
	sessionActive = true;
	if(soloMix_)
	{
		requestAudioFocus(Base::jEnvForThread());
	}
}

void endSession()
{
	if(!sessionActive)
		return;
	sessionActive = false;
	if(soloMix_)
	{
		abandonAudioFocus(Base::jEnvForThread());
	}
}

}

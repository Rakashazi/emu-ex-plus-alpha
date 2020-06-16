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
#include <imagine/audio/defs.hh>
#include <imagine/util/jni.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include "../base/android/android.hh"

namespace IG::AudioManager
{

static jobject audioManager{};
static JavaInstMethod<jint(jobject, jint, jint)> jRequestAudioFocus{};
static JavaInstMethod<jint(jobject)> jAbandonAudioFocus{};
static JavaInstMethod<jobject(jstring)> jGetProperty{};
static bool soloMix_ = true;
static bool sessionActive = false;
static constexpr uint32_t defaultOutputBufferFrames = 192; // default used in Google Oboe library

static int audioManagerIntProperty(JNIEnv* env, JavaInstMethod<jobject(jstring)> jGetProperty, const char *propStr)
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
	if(audioManager)
		return;
	JavaInstMethod<jobject()> jAudioManager{env, Base::jBaseActivityCls, "audioManager", "()Landroid/media/AudioManager;"};
	audioManager = jAudioManager(env, Base::jBaseActivity);
	assert(audioManager);
	audioManager = env->NewGlobalRef(audioManager);
	jclass jAudioManagerCls = env->GetObjectClass(audioManager);
	jRequestAudioFocus.setup(env, jAudioManagerCls, "requestAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;II)I");
	jAbandonAudioFocus.setup(env, jAudioManagerCls, "abandonAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;)I");
	if(Base::androidSDK() >= 17)
		jGetProperty.setup(env, jAudioManagerCls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
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

Audio::PcmFormat nativeFormat()
{
	Audio::PcmFormat nativeFmt{0, Audio::SampleFormats::i16, 2};
	if(Base::androidSDK() >= 17)
	{
		auto env = Base::jEnvForThread();
		setupAudioManagerJNI(env);
		nativeFmt.rate = audioManagerIntProperty(env, jGetProperty, "android.media.property.OUTPUT_SAMPLE_RATE");
		if(nativeFmt.rate != 44100 && nativeFmt.rate != 48000)
		{
			// only support 44KHz and 48KHz for now
			logWarn("ignoring OUTPUT_SAMPLE_RATE value:%d", nativeFmt.rate);
			nativeFmt.rate = 44100;
		}
		else
		{
			logMsg("native sample rate: %d", nativeFmt.rate);
		}
	}
	else
	{
		nativeFmt.rate = 44100;
	}
	return nativeFmt;
}

uint32_t nativeOutputFramesPerBuffer()
{
	if(Base::androidSDK() >= 17)
	{
		auto env = Base::jEnvForThread();
		setupAudioManagerJNI(env);
		int outputBufferFrames = audioManagerIntProperty(env, jGetProperty, "android.media.property.OUTPUT_FRAMES_PER_BUFFER");
		//logMsg("native buffer frames: %d", outputBufferFrames);
		if(outputBufferFrames <= 0 || outputBufferFrames > 4096)
		{
			logWarn("ignoring OUTPUT_FRAMES_PER_BUFFER value:%d", outputBufferFrames);
			return defaultOutputBufferFrames;
		}
		return outputBufferFrames;
	}
	else
	{
		return defaultOutputBufferFrames;
	}
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

namespace IG::Audio
{

std::vector<ApiDesc> audioAPIs()
{
	std::vector<ApiDesc> desc;
	if(Base::androidSDK() >= 26)
	{
		desc.reserve(2);
		desc.emplace_back("AAudio", Api::AAUDIO);
	}
	desc.emplace_back("OpenSL ES", Api::OPENSL_ES);
	return desc;
}

Api makeValidAPI(Api api)
{
	if(Base::androidSDK() >= 26)
	{
		if(api == Api::OPENSL_ES)
			return Api::OPENSL_ES; // OpenSL ES was explicitly requested
		return Api::AAUDIO;
	}
	else
	{
		return Api::OPENSL_ES;
	}
}

}

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

#include <imagine/audio/Manager.hh>
#include <imagine/audio/defs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/jni.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>

namespace IG::Audio
{

static constexpr int defaultOutputBufferFrames = 192; // default used in Google Oboe library
static constexpr int AUDIOFOCUS_GAIN = 1;
static constexpr int STREAM_MUSIC = 3;

AndroidManager::AndroidManager(ApplicationContext ctx_):
	ctx{ctx_}
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx_.baseActivityObject();
	JNI::InstMethod<jobject()> jManager{env, baseActivity, "audioManager", "()Landroid/media/AudioManager;"};
	audioManager = {env, jManager(env, baseActivity)};
	jclass jManagerCls = env->GetObjectClass(audioManager);
	jRequestAudioFocus = {env, jManagerCls, "requestAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;II)I"};
	jAbandonAudioFocus = {env, jManagerCls, "abandonAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;)I"};
	if(ctx_.androidSDK() >= 17)
		jGetProperty = {env, jManagerCls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;"};
}

SampleFormat Manager::nativeSampleFormat() const
{
	return ctx.androidSDK() >= 21 ? SampleFormats::f32 : SampleFormats::i16;
}

int Manager::nativeRate() const
{
	int rate = 44100;
	if(ctx.androidSDK() >= 17)
	{
		auto env = ctx.mainThreadJniEnv();
		rate = audioManagerIntProperty(env, "android.media.property.OUTPUT_SAMPLE_RATE");
		if(rate != 44100 && rate != 48000)
		{
			// only support 44KHz and 48KHz for now
			logWarn("ignoring OUTPUT_SAMPLE_RATE value:%u", rate);
			rate = 44100;
		}
		else
		{
			logMsg("native sample rate: %u", rate);
		}
	}
	return rate;
}

Format Manager::nativeFormat() const
{
	return {nativeRate(), nativeSampleFormat(), 2};
}

void Manager::setSoloMix(std::optional<bool> opt)
{
	if(!opt || soloMix_ == *opt)
		return;
	logMsg("setting solo mix:%s", *opt ? "on" : "off");
	soloMix_ = *opt;
	if(sessionActive)
	{
		// update the current audio focus
		if(*opt)
			requestAudioFocus(ctx.mainThreadJniEnv(), ctx.baseActivityObject());
		else
			jAbandonAudioFocus(ctx.mainThreadJniEnv(), audioManager, ctx.baseActivityObject());
	}
}

bool Manager::soloMix() const
{
	return soloMix_;
}

void Manager::setMusicVolumeControlHint()
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	JNI::InstMethod<void(jint)> jSetVolumeControlStream{env, baseActivity, "setVolumeControlStream", "(I)V"};
	jSetVolumeControlStream(env, baseActivity, 3);
}

void Manager::startSession()
{
	if(sessionActive)
		return;
	sessionActive = true;
	if(soloMix_)
	{
		requestAudioFocus(ctx.mainThreadJniEnv(), ctx.baseActivityObject());
	}
}

void Manager::endSession()
{
	if(!sessionActive)
		return;
	sessionActive = false;
	if(soloMix_)
	{
		jAbandonAudioFocus(ctx.mainThreadJniEnv(), audioManager, ctx.baseActivityObject());
	}
}

std::vector<ApiDesc> Manager::audioAPIs() const
{
	std::vector<ApiDesc> desc;
	if(ctx.androidSDK() >= 26)
	{
		desc.reserve(2);
		desc.emplace_back(ApiDesc{"AAudio", Api::AAUDIO});
	}
	desc.emplace_back(ApiDesc{"OpenSL ES", Api::OPENSL_ES});
	return desc;
}

Api Manager::makeValidAPI(Api api) const
{
	// Don't default to AAudio on Android 8.0 (SDK 26) due
	// to various device-specific driver bugs:
	// ASUS ZenFone 4 (ZE554KL) crashes randomly ~5 mins after playback starts
	// Samsung Galaxy S7 may crash when closing audio stream even when stopping it beforehand
	if(ctx.androidSDK() >= 27)
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

int AndroidManager::nativeOutputFramesPerBuffer() const
{
	if(ctx.androidSDK() >= 17)
	{
		auto env = ctx.mainThreadJniEnv();
		int outputBufferFrames = audioManagerIntProperty(env, "android.media.property.OUTPUT_FRAMES_PER_BUFFER");
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

int AndroidManager::audioManagerIntProperty(JNIEnv* env, const char *propStr) const
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

void AndroidManager::requestAudioFocus(JNIEnv* env, jobject baseActivity) const
{
	[[maybe_unused]] auto res = jRequestAudioFocus(env, audioManager, baseActivity, STREAM_MUSIC, AUDIOFOCUS_GAIN);
	//logMsg("%d from requestAudioFocus()", (int)res);
}

bool AndroidManager::hasFloatFormat() const
{
	return ctx.androidSDK() >= 21;
}

bool AndroidManager::hasStreamUsage() const
{
	return ctx.androidSDK() >= 28;
}

int AndroidManager::defaultOutputBuffers() const
{
	return ctx.androidSDK() >= 18 ? 1 : 2;
}

}

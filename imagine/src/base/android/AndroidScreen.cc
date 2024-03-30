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

#define LOGTAG "Screen"
#include <unistd.h>
#include <cerrno>
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "android.hh"
#include <imagine/base/SimpleFrameTimer.hh>

namespace IG
{

static JNI::InstMethod<void(jboolean)> jSetListener{};
static JNI::InstMethod<void(jlong)> jEnumDisplays{};

void AndroidApplication::initScreens(JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK, ANativeActivity *nActivity)
{
	assert(!jEnumDisplays);
	if(androidSDK >= 17)
	{
		JNI::InstMethod<jobject(jlong)> jDisplayListenerHelper{env, baseActivityClass, "displayListenerHelper", "(J)Lcom/imagine/DisplayListenerHelper;"};
		displayListenerHelper = {env, jDisplayListenerHelper(env, baseActivity, (jlong)nActivity)};
		auto displayListenerHelperCls = env->GetObjectClass(displayListenerHelper);
		JNINativeMethod method[]
		{
			{
				"displayAdd", "(JILandroid/view/Display;FJLandroid/util/DisplayMetrics;)V",
				(void*)
				+[](JNIEnv* env, jobject thiz, jlong nActivityAddr, jint id, jobject disp, jfloat refreshRate, jlong presentationDeadline, jobject metrics)
				{
					ApplicationContext ctx{(ANativeActivity*)nActivityAddr};
					auto &app = ctx.application();
					if(app.findScreen(id))
					{
						logMsg("screen id:%d already in device list", id);
						return;
					}
					app.addScreen(ctx, std::make_unique<Screen>(ctx,
						Screen::InitParams{env, disp, metrics, id, refreshRate, Nanoseconds{presentationDeadline}, Rotation::UP}), true);
				}
			},
			{
				"displayChange", "(JIF)V",
				(void*)
				+[](JNIEnv* env, jobject thiz, jlong nActivityAddr, jint id, jfloat refreshRate)
				{
					ApplicationContext ctx{(ANativeActivity*)nActivityAddr};
					auto &app = ctx.application();
					auto screen = app.findScreen(id);
					if(!screen)
					{
						logWarn("screen id:%d changed but isn't in device list", id);
						return;
					}
					screen->updateFrameRate(refreshRate);
					app.dispatchOnScreenChange(ctx, *screen, ScreenChange::frameRate);
				}
			},
			{
				"displayRemove", "(JI)V",
				(void*)
				+[](JNIEnv* env, jobject thiz, jlong nActivityAddr, jint id)
				{
					ApplicationContext ctx{(ANativeActivity*)nActivityAddr};
					auto &app = ctx.application();
					logMsg("screen id:%d removed", id);
					app.removeScreen(ctx, id, true);
				}
			}
		};
		env->RegisterNatives(displayListenerHelperCls, method, std::size(method));
		jSetListener = {env, displayListenerHelperCls, "setListener", "(Z)V"};
		jSetListener(env, displayListenerHelper, true);
		addOnExit([env, &displayListenerHelper = displayListenerHelper](ApplicationContext ctx, bool backgrounded)
		{
			ctx.application().removeSecondaryScreens();
			logMsg("unregistering display listener");
			jSetListener(env, displayListenerHelper, false);
			if(backgrounded)
			{
				ctx.addOnResume([env, &displayListenerHelper](ApplicationContext ctx, bool)
				{
					logMsg("registering display listener");
					jSetListener(env, displayListenerHelper, true);
					jEnumDisplays(env, ctx.baseActivityObject(), (jlong)ctx.aNativeActivityPtr());
					return false;
				}, SCREEN_ON_RESUME_PRIORITY);
			}
			return true;
		}, SCREEN_ON_EXIT_PRIORITY);
	}
	jEnumDisplays = {env, baseActivityClass, "enumDisplays", "(J)V"};
	jEnumDisplays(env, baseActivity, (jlong)nActivity);
}

AndroidScreen::AndroidScreen(ApplicationContext ctx, InitParams params)
{
	auto [env, aDisplay, metrics, id, refreshRate, presentationDeadline, rotation] = params;
	assert(aDisplay);
	assert(metrics);
	this->aDisplay = {env, aDisplay};
	bool isStraightRotation = true;
	if(id == 0)
	{
		id_ = 0;
		logMsg("init main display with starting rotation:%d", (int)rotation);
		ctx.application().setCurrentRotation(ctx, rotation);
		isStraightRotation = !isSideways(rotation);
	}
	else
	{
		id_ = id;
		logMsg("init display with id:%d", id_);
	}
	presentationDeadline_ = presentationDeadline;
	updateFrameRate(refreshRate);
	if(ctx.androidSDK() <= 10)
	{
		// corrections for devices known to report wrong refresh rates
		auto buildDevice = ctx.androidBuildDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7 && buildDevice == "R800at")
		{
			frameRate_ = 61.5;
			frameTime_ = fromHz<SteadyClockTime>(frameRate_);
		}
		else if(Config::MACHINE_IS_GENERIC_ARMV7 && buildDevice == "sholes")
		{
			frameRate_ = 60;
			frameTime_ = fromHz<SteadyClockTime>(frameRate_);
		}
		else
			reliableFrameRate = false;
	}
	ctx.application().emplaceFrameTimer(frameTimer, *static_cast<Screen*>(this));
	updateSupportedFrameRates(ctx, env);

	// DisplayMetrics
	jclass jDisplayMetricsCls = env->GetObjectClass(metrics);
	auto jDensity = env->GetFieldID(jDisplayMetricsCls, "density", "F");
	auto jScaledDensity = env->GetFieldID(jDisplayMetricsCls, "scaledDensity", "F");
	auto jWidthPixels = env->GetFieldID(jDisplayMetricsCls, "widthPixels", "I");
	auto jHeightPixels = env->GetFieldID(jDisplayMetricsCls, "heightPixels", "I");
	auto widthPixels = env->GetIntField(metrics, jWidthPixels);
	auto heightPixels = env->GetIntField(metrics, jHeightPixels);
	densityDPI_ = 160.*env->GetFloatField(metrics, jDensity);
	assert(densityDPI_);
	scaledDensityDPI_ = 160.*env->GetFloatField(metrics, jScaledDensity);
	assert(scaledDensityDPI_);
	logMsg("screen with size %dx%d, density DPI:%f, scaled density DPI:%f",
		widthPixels, heightPixels, (double)densityDPI_, (double)scaledDensityDPI_);
	if(Config::DEBUG_BUILD)
	{
		auto jXDPI = env->GetFieldID(jDisplayMetricsCls, "xdpi", "F");
		auto jYDPI = env->GetFieldID(jDisplayMetricsCls, "ydpi", "F");
		auto metricsXDPI = env->GetFloatField(metrics, jXDPI);
		auto metricsYDPI = env->GetFloatField(metrics, jYDPI);
		// DPI values are un-rotated from DisplayMetrics
		if(!isStraightRotation)
			std::swap(metricsXDPI, metricsYDPI);
		auto jDensityDPI = env->GetFieldID(jDisplayMetricsCls, "densityDpi", "I");
		logMsg("DPI:%fx%f, densityDPI:%d, refresh rate:%.2fHz",
			metricsXDPI, metricsYDPI, env->GetIntField(metrics, jDensityDPI),
			(double)frameRate_);
	}
	if(!isStraightRotation)
		std::swap(widthPixels, heightPixels);
	width_ = widthPixels;
	height_ = heightPixels;
}

void AndroidScreen::updateFrameRate(float rate)
{
	if(frameRate_ && rate != frameRate_)
	{
		logMsg("refresh rate updated to:%.2f on screen:%d", rate, id());
	}
	if(rate < 20.f || rate > 250.f) // sanity check in case device has a junk value
	{
		logWarn("ignoring unusual refresh rate:%f", rate);
		rate = 60;
		reliableFrameRate = false;
	}
	frameRate_ = rate;
	frameTime_ = fromHz<SteadyClockTime>(rate);
}

void AndroidScreen::updateSupportedFrameRates(ApplicationContext ctx, JNIEnv *env)
{
	if(ctx.androidSDK() < 21)
	{
		supportedFrameRates_ = {frameRate_};
		return;
	}
	JNI::InstMethod<jobject()> jGetSupportedRefreshRates{env, (jobject)aDisplay, "getSupportedRefreshRates", "()[F"};
	auto jRates = (jfloatArray)jGetSupportedRefreshRates(env, aDisplay);
	std::span<jfloat> rates{env->GetFloatArrayElements(jRates, 0), (size_t)env->GetArrayLength(jRates)};
	supportedFrameRates_.assign(rates.begin(), rates.end());
	if constexpr(Config::DEBUG_BUILD)
	{
		logDMsg("screen %d supports %zu rate(s):", id_, rates.size());
		for(auto r : rates) { logDMsg("%f", r); }
	}
	env->ReleaseFloatArrayElements(jRates, rates.data(), 0);
}

int Screen::width() const { return width_; }
int Screen::height() const { return height_; }
FrameRate Screen::frameRate() const { return frameRate_; }
SteadyClockTime Screen::frameTime() const { return frameTime_; }
SteadyClockTime Screen::presentationDeadline() const { return presentationDeadline_; }
bool Screen::frameRateIsReliable() const { return reliableFrameRate; }

void Screen::postFrameTimer()
{
	frameTimer.scheduleVSync();
}

void Screen::unpostFrameTimer()
{
	frameTimer.cancel();
}

void Screen::setVariableFrameTime(bool useVariableTime)
{
	if(!shouldUpdateFrameTimer(frameTimer, useVariableTime))
		return;
	application().emplaceFrameTimer(frameTimer, *static_cast<Screen*>(this), useVariableTime);
}

void Screen::setFrameInterval(int interval)
{
	// TODO
	//logMsg("setting frame interval %d", (int)interval);
	assert(interval >= 1);
}

bool Screen::supportsFrameInterval()
{
	return false;
}

bool Screen::supportsTimestamps() const
{
	return appContext().androidSDK() >= 16;
}

void Screen::setFrameRate(FrameRate rate)
{
	frameTimer.setFrameRate(rate ?: frameRate());
}

std::span<const FrameRate> Screen::supportedFrameRates() const
{
	return supportedFrameRates_;
}

}

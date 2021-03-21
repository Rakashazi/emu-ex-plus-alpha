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
#include <errno.h>
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../common/SimpleFrameTimer.hh"

namespace Base
{

static JavaInstMethod<jobject()> jGetSupportedRefreshRates{};
static JavaInstMethod<void(jboolean)> jSetListener{};
static JavaInstMethod<void(jlong)> jEnumDisplays{};

void initScreens(ApplicationContext app, JNIEnv *env, jobject activity, jclass activityCls)
{
	assert(!jEnumDisplays);
	if(app.androidSDK() >= 17)
	{
		JavaInstMethod<jobject(jlong)> jDisplayListenerHelper{env, activityCls, "displayListenerHelper", "(J)Lcom/imagine/DisplayListenerHelper;"};
		auto displayListenerHelper = jDisplayListenerHelper(env, activity, (jlong)app.aNativeActivityPtr());
		assert(displayListenerHelper);
		displayListenerHelper = env->NewGlobalRef(displayListenerHelper);
		auto displayListenerHelperCls = env->GetObjectClass(displayListenerHelper);
		JNINativeMethod method[]
		{
			{
				"displayAdd", "(JILandroid/view/Display;FLandroid/util/DisplayMetrics;)V",
				(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jobject, jfloat, jobject))
				([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jint id, jobject disp, jfloat refreshRate, jobject metrics)
				{
					ApplicationContext app{(ANativeActivity*)nActivityAddr};
					if(app.findScreen(id))
					{
						logMsg("screen id:%d already in device list", id);
						return;
					}
					app.addScreen(std::make_unique<Screen>(env, app, disp, id, refreshRate, SURFACE_ROTATION_0, metrics), true);
				})
			},
			{
				"displayChange", "(JIF)V",
				(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jfloat))
				([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jint id, jfloat refreshRate)
				{
					ApplicationContext app{(ANativeActivity*)nActivityAddr};
					auto screen = app.findScreen(id);
					if(!screen)
					{
						logWarn("screen id:%d changed but isn't in device list", id);
						return;
					}
					screen->updateRefreshRate(refreshRate);
				})
			},
			{
				"displayRemove", "(JI)V",
				(void*)(void (*)(JNIEnv*, jobject, jlong, jint))
				([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jint id)
				{
					ApplicationContext app{(ANativeActivity*)nActivityAddr};
					logMsg("screen id:%d removed", id);
					app.removeScreen(id, true);
				})
			}
		};
		env->RegisterNatives(displayListenerHelperCls, method, std::size(method));
		jSetListener = {env, displayListenerHelperCls, "setListener", "(Z)V"};
		jSetListener(env, displayListenerHelper, true);
		app.addOnExit([env, displayListenerHelper](ApplicationContext app, bool backgrounded)
		{
			app.removeSecondaryDisplays();
			logMsg("unregistering display listener");
			jSetListener(env, displayListenerHelper, false);
			if(backgrounded)
			{
				app.addOnResume([env, displayListenerHelper](ApplicationContext app, bool)
				{
					logMsg("registering display listener");
					jSetListener(env, displayListenerHelper, true);
					jEnumDisplays(env, app.baseActivityObject(), (jlong)app.aNativeActivityPtr());
					return false;
				}, Base::SCREEN_ON_RESUME_PRIORITY);
			}
			return true;
		}, Base::SCREEN_ON_EXIT_PRIORITY);
	}
	JNINativeMethod method[]
	{
		{
			"displayEnumerated", "(JLandroid/view/Display;IFILandroid/util/DisplayMetrics;)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jobject, jint, jfloat, jint, jobject))
			([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jobject disp, jint id, jfloat refreshRate, jint rotation, jobject metrics)
			{
				//auto it = IG::find_if(screen_, [=](const auto &e) { return e->id() == id; });
				ApplicationContext app{(ANativeActivity*)nActivityAddr};
				auto screen = app.findScreen(id);
				if(!screen)
				{
					app.addScreen(std::make_unique<Screen>(env, app, disp, id, refreshRate, (SurfaceRotation)rotation, metrics), false);
					return;
				}
				else
				{
					// already in list, update existing
					screen->updateRefreshRate(refreshRate);
				}
			})
		},
	};
	env->RegisterNatives(activityCls, method, std::size(method));
	jEnumDisplays = {env, activityCls, "enumDisplays", "(J)V"};
	jEnumDisplays(env, activity, (jlong)app.aNativeActivityPtr());
}

AndroidScreen::AndroidScreen(JNIEnv *env, ApplicationContext app, jobject aDisplay, int id, float refreshRate, SurfaceRotation rotation, jobject metrics)
{
	assumeExpr(aDisplay);
	assumeExpr(metrics);
	mainThreadJniEnv = env;
	this->aDisplay = env->NewGlobalRef(aDisplay);
	bool isStraightRotation = true;
	if(id == 0)
	{
		id_ = 0;
		logMsg("init main display with starting rotation:%d", (int)rotation);
		osRotation = rotation;
		isStraightRotation = surfaceRotationIsStraight(rotation);
	}
	else
	{
		id_ = id;
		logMsg("init display with id:%d", id_);
	}

	updateRefreshRate(refreshRate);
	if(app.androidSDK() <= 10)
	{
		// corrections for devices known to report wrong refresh rates
		auto buildDevice = app.androidBuildDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7 && string_equal(buildDevice.data(), "R800at"))
			refreshRate_ = 61.5;
		else if(Config::MACHINE_IS_GENERIC_ARMV7 && string_equal(buildDevice.data(), "sholes"))
			refreshRate_ = 60;
		else
			reliableRefreshRate = false;
	}

	// DisplayMetrics
	jclass jDisplayMetricsCls = env->GetObjectClass(metrics);
	auto jXDPI = env->GetFieldID(jDisplayMetricsCls, "xdpi", "F");
	auto jYDPI = env->GetFieldID(jDisplayMetricsCls, "ydpi", "F");
	auto jScaledDensity = env->GetFieldID(jDisplayMetricsCls, "scaledDensity", "F");
	auto jWidthPixels = env->GetFieldID(jDisplayMetricsCls, "widthPixels", "I");
	auto jHeightPixels = env->GetFieldID(jDisplayMetricsCls, "heightPixels", "I");
	auto metricsXDPI = env->GetFloatField(metrics, jXDPI);
	auto metricsYDPI = env->GetFloatField(metrics, jYDPI);
	auto widthPixels = env->GetIntField(metrics, jWidthPixels);
	auto heightPixels = env->GetIntField(metrics, jHeightPixels);
	densityDPI_ = 160.*env->GetFloatField(metrics, jScaledDensity);
	assert(densityDPI_);
	logMsg("screen with size %dx%d, DPI size %fx%f, scaled density DPI %f",
		widthPixels, heightPixels, (double)metricsXDPI, (double)metricsYDPI, (double)densityDPI_);
	if(Config::DEBUG_BUILD)
	{
		auto jDensity = env->GetFieldID(jDisplayMetricsCls, "density", "F");
		auto jDensityDPI = env->GetFieldID(jDisplayMetricsCls, "densityDpi", "I");
		logMsg("display density %f, densityDPI %d, %dx%d pixels, %.2fHz",
			(double)env->GetFloatField(metrics, jDensity), env->GetIntField(metrics, jDensityDPI),
			env->GetIntField(metrics, jWidthPixels), env->GetIntField(metrics, jHeightPixels),
			(double)refreshRate_);
	}
	// DPI values are un-rotated from DisplayMetrics
	xDPI = isStraightRotation ? metricsXDPI : metricsYDPI;
	yDPI = isStraightRotation ? metricsYDPI : metricsXDPI;
	width_ = isStraightRotation ? widthPixels : heightPixels;
	height_ = isStraightRotation ? heightPixels : widthPixels;
}

std::pair<float, float> AndroidScreen::dpi() const
{
	return {xDPI, yDPI};
}

float AndroidScreen::densityDPI() const
{
	return densityDPI_;
}

jobject AndroidScreen::displayObject() const
{
	return aDisplay;
}

int AndroidScreen::id() const
{
	return id_;
}

void AndroidScreen::updateRefreshRate(float refreshRate)
{
	if(refreshRate_ && refreshRate != refreshRate_)
	{
		logMsg("refresh rate updated to:%.2f on screen:%d", refreshRate, id());
	}
	if(refreshRate < 20.f || refreshRate > 250.f) // sanity check in case device has a junk value
	{
		logWarn("ignoring unusual refresh rate: %f", (double)refreshRate);
		refreshRate = 60;
		reliableRefreshRate = false;
	}
	refreshRate_ = refreshRate;
	frameTime_ = IG::FloatSeconds(1. / refreshRate);
}

bool AndroidScreen::operator ==(AndroidScreen const &rhs) const
{
	return id_ == rhs.id_;
}

AndroidScreen::operator bool() const
{
	return aDisplay;
}

AndroidScreen::~AndroidScreen()
{
	mainThreadJniEnv->DeleteGlobalRef(aDisplay);
}

int Screen::width()
{
	return width_;
}

int Screen::height()
{
	return height_;
}

double Screen::frameRate() const
{
	return refreshRate_;
}

IG::FloatSeconds Screen::frameTime() const
{
	return IG::FloatSeconds(1. / frameRate());
}

bool Screen::frameRateIsReliable() const
{
	return reliableRefreshRate;
}

void Screen::postFrame()
{
	if(!isActive)
	{
		//logMsg("can't post frame when app isn't running");
		return;
	}
	if(framePosted)
		return;
	//logMsg("posting frame");
	framePosted = true;
	frameTimer->scheduleVSync();
}

void Screen::unpostFrame()
{
	if(!framePosted)
		return;
	framePosted = false;
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

bool Screen::supportsTimestamps(ApplicationContext app)
{
	return app.androidSDK() >= 16;
}

void Screen::setFrameRate(double rate)
{
	// unsupported
}

std::vector<double> Screen::supportedFrameRates(ApplicationContext app)
{
	std::vector<double> rateVec;
	if(app.androidSDK() < 21)
	{
		rateVec.reserve(1);
		rateVec.emplace_back(frameRate());
	}
	auto env = app.mainThreadJniEnv();
	if(unlikely(!jGetSupportedRefreshRates))
	{
		jclass jDisplayCls = env->GetObjectClass(aDisplay);
		jGetSupportedRefreshRates.setup(env, jDisplayCls, "getSupportedRefreshRates", "()[F");
	}
	auto jRates = (jfloatArray)jGetSupportedRefreshRates(env, aDisplay);
	auto rate = env->GetFloatArrayElements(jRates, 0);
	auto rates = env->GetArrayLength(jRates);
	rateVec.reserve(rates);
	logMsg("screen %d supports %d rate(s):", id_, rates);
	iterateTimes(rates, i)
	{
		double r = rate[i];
		logMsg("%f", r);
		rateVec.emplace_back(r);
	}
	env->ReleaseFloatArrayElements(jRates, rate, 0);
	return rateVec;
}

}

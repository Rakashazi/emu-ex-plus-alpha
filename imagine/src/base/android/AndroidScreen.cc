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

#include <unistd.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <imagine/base/Screen.hh>
#include <imagine/base/Base.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../common/screenPrivate.hh"

namespace Base
{

static JavaInstMethod<jint()> jGetRotation{};
static JavaInstMethod<jfloat()> jGetRefreshRate{};
static JavaInstMethod<jobject()> jGetSupportedRefreshRates{};
static JavaInstMethod<jobject(jobject)> jGetMetrics{};
JavaInstMethod<jobject(jobject, jlong)> jPresentation{};
JavaInstMethod<jobject(jint)> jGetDisplay{};
#ifdef CONFIG_BASE_MULTI_SCREEN
static JavaInstMethod<jint()> jGetDisplayId{};
#endif

void initScreens(JNIEnv *env, jobject activity, jclass activityCls)
{
	{
		JavaInstMethod<jobject()> jDefaultDpy{env, activityCls, "defaultDpy", "()Landroid/view/Display;"};
		// DisplayMetrics obtained via getResources().getDisplayMetrics() so the scaledDensity field is correct
		JavaInstMethod<jobject()> jDisplayMetrics{env, activityCls, "displayMetrics", "()Landroid/util/DisplayMetrics;"};
		static Screen main;
		main.init(env, jDefaultDpy(env, activity), jDisplayMetrics(env, activity), true);
		Screen::addScreen(&main);
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	if(Base::androidSDK() >= 17)
	{
		jPresentation.setup(env, activityCls, "presentation", "(Landroid/view/Display;J)Lcom/imagine/PresentationHelper;");
		logMsg("setting up screen notifications");
		JavaInstMethod<jobject()> jDisplayListenerHelper{env, activityCls, "displayListenerHelper", "()Lcom/imagine/DisplayListenerHelper;"};
		auto displayListenerHelper = jDisplayListenerHelper(env, activity);
		assert(displayListenerHelper);
		auto displayListenerHelperCls = env->GetObjectClass(displayListenerHelper);
		JNINativeMethod method[] =
		{
			{
				"displayChange", "(II)V",
				(void*)(void (*)(JNIEnv*, jobject, jint, jint))
				([](JNIEnv* env, jobject thiz, jint id, jint change)
				{
					switch(change)
					{
						bcase 0:
							for(auto s : screen_)
							{
								if(s->id == id)
								{
									logMsg("screen %d already in device list", id);
									break;
								}
							}
							if(!screen_.isFull())
							{
								Screen *s = new Screen();
								s->init(env, jGetDisplay(env, thiz, id), nullptr, false);
								Screen::addScreen(s);
								if(Screen::onChange)
									Screen::onChange(*s, {Screen::Change::ADDED});
							}
						bcase 2:
							logMsg("screen %d removed", id);
							forEachInContainer(screen_, it)
							{
								Screen *removedScreen = *it;
								if(removedScreen->id == id)
								{
									it.erase();
									if(Screen::onChange)
										Screen::onChange(*removedScreen, {Screen::Change::REMOVED});
									removedScreen->deinit();
									delete removedScreen;
									break;
								}
							}
					}
				})
			}
		};
		env->RegisterNatives(displayListenerHelperCls, method, IG::size(method));

		// get the current presentation screens
		JavaInstMethod<jobject()> jGetPresentationDisplays{env, displayListenerHelperCls, "getPresentationDisplays", "()[Landroid/view/Display;"};
		jGetDisplay.setup(env, displayListenerHelperCls, "getDisplay", "(I)Landroid/view/Display;");
		auto jPDisplay = (jobjectArray)jGetPresentationDisplays(env, displayListenerHelper);
		uint pDisplays = env->GetArrayLength(jPDisplay);
		if(pDisplays)
		{
			if(pDisplays > screen_.freeSpace())
				pDisplays = screen_.freeSpace();
			logMsg("checking %d presentation display(s)", pDisplays);
			iterateTimes(pDisplays, i)
			{
				auto display = env->GetObjectArrayElement(jPDisplay, i);
				Screen *s = new Screen();
				s->init(env, display, nullptr, false);
				Screen::addScreen(s);
			}
		}
	}
	#endif
}

void AndroidScreen::init(JNIEnv *env, jobject aDisplay, jobject metrics, bool isMain)
{
	assert(aDisplay);
	this->aDisplay = env->NewGlobalRef(aDisplay);
	if(!jGetRotation)
	{
		jclass jDisplayCls = env->GetObjectClass(aDisplay);
		jGetRotation.setup(env, jDisplayCls, "getRotation", "()I");
		jGetRefreshRate.setup(env, jDisplayCls, "getRefreshRate", "()F");
		#ifdef CONFIG_BASE_MULTI_SCREEN
		jGetDisplayId.setup(env, jDisplayCls, "getDisplayId", "()I");
		#endif
		jGetMetrics.setup(env, Base::jBaseActivityCls, "getDisplayMetrics", "(Landroid/view/Display;)Landroid/util/DisplayMetrics;");
	}

	bool isStraightRotation = true;
	if(isMain)
	{
		auto orientation = (SurfaceRotation)jGetRotation(env, aDisplay);
		logMsg("starting orientation %d", orientation);
		osRotation = orientation;
		isStraightRotation = surfaceRotationIsStraight(orientation);
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	if(isMain)
	{
		id = 0;
	}
	else
	{
		id = jGetDisplayId(env, aDisplay);;
		logMsg("init display with id: %d", id);
	}
	#endif

	// refresh rate
	refreshRate_ = jGetRefreshRate(env, aDisplay);
	if(Base::androidSDK() <= 10)
	{
		// corrections for devices known to report wrong refresh rates
		auto buildDevice = Base::androidBuildDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7 && string_equal(buildDevice.data(), "R800at"))
			refreshRate_ = 61.5;
		else if(Config::MACHINE_IS_GENERIC_ARMV7 && string_equal(buildDevice.data(), "sholes"))
			refreshRate_ = 60;
		else
			reliableRefreshRate = false;
	}
	if(refreshRate_ < 20.f || refreshRate_ > 200.f) // sanity check in case device has a junk value
	{
		logWarn("ignoring unusual refresh rate: %f", (double)refreshRate_);
		refreshRate_ = 60;
		reliableRefreshRate = false;
	}

	// DisplayMetrics
	if(!metrics)
	{
		logMsg("getting metrics from display");
		metrics = jGetMetrics(env, Base::jBaseActivity, aDisplay);
		assert(metrics);
	}
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
	densityDPI = 160.*env->GetFloatField(metrics, jScaledDensity);
	assert(densityDPI);
	logMsg("screen with size %dx%d, DPI size %fx%f, scaled density DPI %f",
		widthPixels, heightPixels, (double)metricsXDPI, (double)metricsYDPI, (double)densityDPI);
	#ifndef NDEBUG
	{
		auto jDensity = env->GetFieldID(jDisplayMetricsCls, "density", "F");
		auto jDensityDPI = env->GetFieldID(jDisplayMetricsCls, "densityDpi", "I");
		logMsg("display density %f, densityDPI %d, %dx%d pixels, %.2fHz",
			(double)env->GetFloatField(metrics, jDensity), env->GetIntField(metrics, jDensityDPI),
			env->GetIntField(metrics, jWidthPixels), env->GetIntField(metrics, jHeightPixels),
			(double)refreshRate_);
	}
	#endif
	// DPI values are un-rotated from DisplayMetrics
	xDPI = isStraightRotation ? metricsXDPI : metricsYDPI;
	yDPI = isStraightRotation ? metricsYDPI : metricsXDPI;
	width_ = isStraightRotation ? widthPixels : heightPixels;
	height_ = isStraightRotation ? heightPixels : widthPixels;
}

void Screen::deinit()
{
	unpostFrame();
	jEnv()->DeleteGlobalRef(aDisplay);
	*this = {};
}

int Screen::width()
{
	return width_;
}

int Screen::height()
{
	return height_;
}

SurfaceRotation AndroidScreen::rotation(JNIEnv *env)
{
	return (SurfaceRotation)jGetRotation(env, aDisplay);
}

double Screen::frameRate()
{
	return refreshRate_;
}

double Screen::frameTime()
{
	return 1. / frameRate();
}

bool Screen::frameRateIsReliable()
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
	if(!inFrameHandler)
	{
		if(Base::androidSDK() < 16)
			currFrameTimestamp = IG::Time::now().nSecs();
		prevFrameTimestamp = 0;
	}
}

void Screen::unpostFrame()
{
	if(!framePosted)
		return;
	framePosted = false;
	if(!screensArePosted())
		frameTimer->cancel();
}

void Screen::setFrameInterval(uint interval)
{
	// TODO
	//logMsg("setting frame interval %d", (int)interval);
	assert(interval >= 1);
}

bool Screen::supportsFrameInterval()
{
	return false;
}

void Screen::setFrameRate(double rate)
{
	// unsupported
}

std::vector<double> Screen::supportedFrameRates()
{
	std::vector<double> rateVec;
	if(Base::androidSDK() < 21)
	{
		rateVec.reserve(1);
		rateVec.emplace_back(frameRate());
	}
	auto env = jEnv();
	if(unlikely(!jGetSupportedRefreshRates))
	{
		jclass jDisplayCls = env->GetObjectClass(aDisplay);
		jGetSupportedRefreshRates.setup(env, jDisplayCls, "getSupportedRefreshRates", "()[F");
	}
	auto jRates = (jfloatArray)jGetSupportedRefreshRates(env, aDisplay);
	auto rate = env->GetFloatArrayElements(jRates, 0);
	auto rates = env->GetArrayLength(jRates);
	rateVec.reserve(rates);
	logMsg("screen %d supports %d rate(s):", id, rates);
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

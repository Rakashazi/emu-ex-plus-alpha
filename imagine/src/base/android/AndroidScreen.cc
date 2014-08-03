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
#include "internal.hh"
#include "android.hh"
#include "ASurface.hh"
#include "../common/screenPrivate.hh"

namespace Base
{

static JavaInstMethod<jint> jGetRotation;
JavaInstMethod<jobject> jPresentation;
JavaInstMethod<jobject> jGetDisplay;

void initScreens(JNIEnv *jEnv, jobject activity, jclass activityCls)
{
	{
		JavaInstMethod<jobject> jDefaultDpy;
		jDefaultDpy.setup(jEnv, activityCls, "defaultDpy", "()Landroid/view/Display;");
		// DisplayMetrics obtained via getResources().getDisplayMetrics() so the scaledDensity field is correct
		JavaInstMethod<jobject> jDisplayMetrics;
		jDisplayMetrics.setup(jEnv, activityCls, "displayMetrics", "()Landroid/util/DisplayMetrics;");
		static Screen main;
		main.init(jEnv, jDefaultDpy(jEnv, activity), jDisplayMetrics(jEnv, activity), true);
		Screen::addScreen(&main);
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	if(Base::androidSDK() >= 17)
	{
		jPresentation.setup(jEnv, activityCls, "presentation", "(Landroid/view/Display;J)Lcom/imagine/PresentationHelper;");
		logMsg("setting up screen notifications");
		JavaInstMethod<jobject> jDisplayListenerHelper;
		jDisplayListenerHelper.setup(jEnv, activityCls, "displayListenerHelper", "()Lcom/imagine/DisplayListenerHelper;");
		auto displayListenerHelper = jDisplayListenerHelper(jEnv, activity);
		assert(displayListenerHelper);
		auto displayListenerHelperCls = jEnv->GetObjectClass(displayListenerHelper);
		JNINativeMethod method[] =
		{
			{
				"displayChange", "(II)V",
				(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jint devID, jint change))
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
		jEnv->RegisterNatives(displayListenerHelperCls, method, sizeofArray(method));

		// get the current presentation screens
		JavaInstMethod<jobject> jGetPresentationDisplays;
		jGetPresentationDisplays.setup(jEnv, displayListenerHelperCls, "getPresentationDisplays", "()[Landroid/view/Display;");
		jGetDisplay.setup(jEnv, displayListenerHelperCls, "getDisplay", "(I)Landroid/view/Display;");
		auto jPDisplay = (jobjectArray)jGetPresentationDisplays(jEnv, displayListenerHelper);
		uint pDisplays = jEnv->GetArrayLength(jPDisplay);
		if(pDisplays)
		{
			if(pDisplays > screen_.freeSpace())
				pDisplays = screen_.freeSpace();
			logMsg("checking %d presentation display(s)", pDisplays);
			iterateTimes(pDisplays, i)
			{
				auto display = jEnv->GetObjectArrayElement(jPDisplay, i);
				Screen *s = new Screen();
				s->init(jEnv, display, nullptr, false);
				Screen::addScreen(s);
			}
		}
		jEnv->DeleteLocalRef(jPDisplay);
	}
	#endif
}

void AndroidScreen::init(JNIEnv *jEnv, jobject aDisplay, jobject metrics, bool isMain)
{
	assert(aDisplay);
	this->aDisplay = jEnv->NewGlobalRef(aDisplay);

	if(!jGetRotation)
	{
		jclass jDisplayCls = jEnv->GetObjectClass(aDisplay);
		jGetRotation.setup(jEnv, jDisplayCls, "getRotation", "()I");
	}

	bool isStraightOrientation = true;
	if(isMain)
	{
		auto orientation = jGetRotation(jEnv, aDisplay);
		logMsg("starting orientation %d", orientation);
		osOrientation = orientation;
		isStraightOrientation = !ASurface::isSidewaysOrientation(orientation);
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	if(isMain)
	{
		id = 0;
	}
	else
	{
		jclass jDisplayCls = jEnv->GetObjectClass(aDisplay);
		JavaInstMethod<jint> jGetDisplayId;
		jGetDisplayId.setup(jEnv, jDisplayCls, "getDisplayId", "()I");
		id = jGetDisplayId(jEnv, aDisplay);;
		logMsg("init display with id: %d", id);
	}
	#endif

	// DisplayMetrics
	if(!metrics)
	{
		logMsg("getting metrics from display");
		JavaInstMethod<jobject> jGetMetrics;
		jGetMetrics.setup(jEnv, Base::jBaseActivityCls, "getDisplayMetrics", "(Landroid/view/Display;)Landroid/util/DisplayMetrics;");
		metrics = jGetMetrics(jEnv, Base::jBaseActivity, aDisplay);
		assert(metrics);
	}
	jclass jDisplayMetricsCls = jEnv->GetObjectClass(metrics);
	auto jXDPI = jEnv->GetFieldID(jDisplayMetricsCls, "xdpi", "F");
	auto jYDPI = jEnv->GetFieldID(jDisplayMetricsCls, "ydpi", "F");
	auto jScaledDensity = jEnv->GetFieldID(jDisplayMetricsCls, "scaledDensity", "F");
	auto jWidthPixels = jEnv->GetFieldID(jDisplayMetricsCls, "widthPixels", "I");
	auto jHeightPixels = jEnv->GetFieldID(jDisplayMetricsCls, "heightPixels", "I");
	auto metricsXDPI = jEnv->GetFloatField(metrics, jXDPI);
	auto metricsYDPI = jEnv->GetFloatField(metrics, jYDPI);
	auto widthPixels = jEnv->GetIntField(metrics, jWidthPixels);
	auto heightPixels = jEnv->GetIntField(metrics, jHeightPixels);
	densityDPI = 160.*jEnv->GetFloatField(metrics, jScaledDensity);
	assert(densityDPI);
	logMsg("screen with size %dx%d, DPI size %fx%f, scaled density DPI %f",
		widthPixels, heightPixels, (double)metricsXDPI, (double)metricsYDPI, (double)densityDPI);
	#ifndef NDEBUG
	{
		auto jDensity = jEnv->GetFieldID(jDisplayMetricsCls, "density", "F");
		auto jDensityDPI = jEnv->GetFieldID(jDisplayMetricsCls, "densityDpi", "I");
		logMsg("display density %f, densityDPI %d, %dx%d pixels",
			(double)jEnv->GetFloatField(metrics, jDensity), jEnv->GetIntField(metrics, jDensityDPI),
			jEnv->GetIntField(metrics, jWidthPixels), jEnv->GetIntField(metrics, jHeightPixels));
	}
	#endif
	// DPI values are un-rotated from DisplayMetrics
	xDPI = isStraightOrientation ? metricsXDPI : metricsYDPI;
	yDPI = isStraightOrientation ? metricsYDPI : metricsXDPI;
	width_ = isStraightOrientation ? widthPixels : heightPixels;
	height_ = isStraightOrientation ? heightPixels : widthPixels;
}

void Screen::deinit()
{
	unpostFrame();
	eEnv()->DeleteGlobalRef(aDisplay);
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

int AndroidScreen::aOrientation(JNIEnv *jEnv)
{
	return jGetRotation(jEnv, aDisplay);
}

uint Screen::refreshRate()
{
	if(!refreshRate_)
	{
		assert(aDisplay);
		JavaInstMethod<jfloat> jGetRefreshRate;
		auto jEnv = eEnv();
		jGetRefreshRate.setup(jEnv, jEnv->GetObjectClass(aDisplay), "getRefreshRate", "()F");
		refreshRate_ = jGetRefreshRate(jEnv, aDisplay);
		logMsg("refresh rate: %f", (double)refreshRate_);
	}
	return refreshRate_;
}

void Screen::postFrame()
{
	if(!appIsRunning())
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
			currFrameTime = TimeSys::now().toNs();
		prevFrameTime = 0;
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

}

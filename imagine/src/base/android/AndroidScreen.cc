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
#include <imagine/base/Screen.hh>
#include <imagine/base/Base.hh>
#include "androidBase.hh"
#include "private.hh"
#include "ASurface.hh"

namespace Base
{

extern bool hasChoreographer;
static JavaInstMethod<jint> jGetRotation;

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

void Screen::frameComplete()
{
	if(!hasChoreographer && frameIsPosted())
	{
		// update the frame time after a blocking double-buffered swap
		currFrameTime = TimeSys::now().toNs();
	}
}

void Screen::postFrame()
{
	if(!appIsRunning())
	{
		//logMsg("can't post frame when app isn't running");
		return;
	}
	else if(!framePosted)
	{
		framePosted = true;
		if(inFrameHandler)
		{
			//logMsg("posting frame while in frame handler");
		}
		else if(!framePostedEvent)
		{
			framePostedEvent = true;
			//logMsg("posting frame");
			if(jPostFrame.m)
				jPostFrame(eEnv(), frameHelper);
			else
			{
				uint64_t post = 1;
				auto ret = write(onFrameEventFd, &post, sizeof(post));
				assert(ret == sizeof(post));
			}
		}
	}
}

void Screen::unpostFrame()
{
	if(framePosted)
	{
		framePosted = false;
		currFrameTime = 0;
		if(inFrameHandler)
		{
			//logMsg("un-posting frame while in frame handler");
		}
		else if(framePostedEvent && !screensArePosted())
		{
			framePostedEvent = false;
			//logMsg("un-posting frame");
			if(jUnpostFrame.m)
				jUnpostFrame(eEnv(), frameHelper);
			else
			{
				uint64_t post;
				read(onFrameEventFd, &post, sizeof(post));
				onFrameEventIdle = 1; // force handler to idle since it could already be signaled by epoll
			}
		}
	}
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

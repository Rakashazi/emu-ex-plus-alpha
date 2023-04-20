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

#define LOGTAG "FrameTimer"
#include <imagine/base/Screen.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/variant.hh>
#include <imagine/base/SimpleFrameTimer.hh>
#include <imagine/logger/logger.h>
#include <android/choreographer.h>
#include <unistd.h>
#include <cerrno>

namespace IG
{

FrameTimer AndroidApplication::makeFrameTimer(Screen &screen)
{
	return visit(overloaded
	{
		[&](JavaChoreographer &c)
		{
			return FrameTimer{std::in_place_type<JavaChoreographerFrameTimer>, c};
		},
		[&](NativeChoreographer &c)
		{
			if(c)
				return FrameTimer{std::in_place_type<NativeChoreographerFrameTimer>, c};
			else // no choreographer
				return FrameTimer{std::in_place_type<SimpleFrameTimer>, screen};
		},
	}, choreographer);
}

void AndroidApplication::initChoreographer(JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
	if(androidSDK < 16)
	{
		return;
	}
	if(sizeof(long) < sizeof(int64_t) || androidSDK < 24)
	{
		// Always use on 32-bit systems due to NDK API issue
		// that prevents 64-bit timestamp resolution
		choreographer.emplace<JavaChoreographer>(*this, env, baseActivity, baseActivityClass);
	}
	else
	{
		choreographer.emplace<NativeChoreographer>(*this);
	}
}

static void updatePostedScreens(auto &choreographer, SteadyClockTimePoint timestamp, AndroidApplication &app)
{
	bool didUpdate{};
	app.flushSystemInputEvents();
	for(auto &s : app.screens())
	{
		if(s->isPosted())
		{
			didUpdate |= s->frameUpdate(timestamp);
		}
	}
	if(didUpdate)
	{
		choreographer.scheduleVSync();
	}
	else
	{
		//logMsg("stopping screen updates");
	}
}

JavaChoreographer::JavaChoreographer(AndroidApplication &app, JNIEnv *env, jobject baseActivity, jclass baseActivityClass):
	appPtr{&app}
{
	JNI::InstMethod<jobject(jlong)> jChoreographerHelper{env, baseActivityClass, "choreographerHelper", "(J)Lcom/imagine/ChoreographerHelper;"};
	frameHelper = {env, jChoreographerHelper(env, baseActivity, (jlong)this)};
	auto choreographerHelperCls = env->GetObjectClass(frameHelper);
	jPostFrame = {env, choreographerHelperCls, "postFrame", "()V"};
	JNINativeMethod method[]
	{
		{
			"onFrame", "(JJ)V",
			(void*)
			+[](JNIEnv* env, jobject thiz, jlong userData, jlong frameTimeNanos)
			{
				auto &inst = *((JavaChoreographer*)userData);
				inst.requested = false;
				updatePostedScreens(inst, SteadyClockTimePoint{Nanoseconds{frameTimeNanos}}, *inst.appPtr);
			}
		}
	};
	env->RegisterNatives(choreographerHelperCls, method, std::size(method));
	logMsg("using Java Choreographer");
}

void JavaChoreographer::scheduleVSync()
{
	assert(frameHelper);
	if(requested)
		return;
	requested = true;
	jPostFrame(frameHelper.jniEnv(), frameHelper);
}

NativeChoreographer::NativeChoreographer(AndroidApplication &app):
	appPtr{&app}
{
	AChoreographer* (*getInstance)(){};
	loadSymbol(getInstance, {}, "AChoreographer_getInstance");
	assert(getInstance);
	loadSymbol(postFrameCallback, {}, "AChoreographer_postFrameCallback");
	assert(postFrameCallback);
	choreographer = getInstance();
	assert(choreographer);
	logMsg("using native Choreographer");
}

void NativeChoreographer::scheduleVSync()
{
	if(requested)
		return;
	requested = true;
	postFrameCallback(choreographer,
		[](long frameTimeNanos, void* userData)
		{
			auto &inst = *((NativeChoreographer*)userData);
			inst.requested = false;
			updatePostedScreens(inst, SteadyClockTimePoint{Nanoseconds{frameTimeNanos}}, *inst.appPtr);
		}, this);
}

}

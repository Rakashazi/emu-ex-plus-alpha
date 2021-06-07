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
#include <imagine/base/SimpleFrameTimer.hh>
#include <imagine/logger/logger.h>
#include <android/choreographer.h>
#include <unistd.h>
#include <errno.h>

namespace Base
{

FrameTimer AndroidApplication::makeFrameTimer(Screen &screen)
{
	if(auto javaChoreographerPtr = std::get_if<JavaChoreographer>(&choreographer);
		javaChoreographerPtr)
	{
		return FrameTimer{std::in_place_type<JavaChoreographerFrameTimer>, *javaChoreographerPtr};
	}
	else
	{
		auto &nativeChoreographer = std::get<NativeChoreographer>(choreographer);
		if(nativeChoreographer)
		{
			return FrameTimer{std::in_place_type<NativeChoreographerFrameTimer>, nativeChoreographer};
		}
		else
		{
			return FrameTimer{std::in_place_type<SimpleFrameTimer>, screen};
		}
	}
}

void AndroidApplication::initChoreographer(const ScreenContainter &screens, JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
	if(androidSDK < 16)
	{
		return;
	}
	if(sizeof(long) < sizeof(int64_t) || androidSDK < 24)
	{
		// Always use on 32-bit systems due to NDK API issue
		// that prevents 64-bit timestamp resolution
		choreographer.emplace<JavaChoreographer>(screens, env, baseActivity, baseActivityClass);
	}
	else
	{
		choreographer.emplace<NativeChoreographer>(screens);
	}
}

static void updatePostedScreens(auto &choreographer, FrameTime timestamp, const ScreenContainter &screens)
{
	bool didUpdate{};
	for(auto &s : screens)
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

JavaChoreographer::JavaChoreographer(const ScreenContainter &screens, JNIEnv *env, jobject baseActivity, jclass baseActivityClass):
	screensPtr{&screens}
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
				updatePostedScreens(inst, FrameTime{frameTimeNanos}, *inst.screensPtr);
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

NativeChoreographer::NativeChoreographer(const ScreenContainter &screens):
	screensPtr{&screens}
{
	AChoreographer* (*getInstance)(){};
	Base::loadSymbol(getInstance, {}, "AChoreographer_getInstance");
	assert(getInstance);
	Base::loadSymbol(postFrameCallback, {}, "AChoreographer_postFrameCallback");
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
			updatePostedScreens(inst, FrameTime{frameTimeNanos}, *inst.screensPtr);
		}, this);
}

}

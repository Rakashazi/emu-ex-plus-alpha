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
#include <unistd.h>
#include <errno.h>
#include <imagine/base/Screen.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../common/SimpleFrameTimer.hh"
#include <android/choreographer.h>

namespace Base
{

class ChoreographerFrameTimer final : public FrameTimer
{
public:
	ChoreographerFrameTimer(ApplicationContext);
	~ChoreographerFrameTimer() final;
	void scheduleVSync() final;
	void cancel() final;
	void unsetRequested() { requested = false; }
	explicit operator bool() const { return frameHelper; }
	ApplicationContext appContext() const { return app; }

protected:
	ApplicationContext app{};
	JavaInstMethod<void()> jPostFrame{}, jUnpostFrame{};
	jobject frameHelper{};
	bool requested = false;
};

class AChoreographerFrameTimer final : public FrameTimer
{
public:
	AChoreographerFrameTimer(ApplicationContext);
	~AChoreographerFrameTimer() final {};
	void scheduleVSync() final;
	void cancel() final;
	void unsetRequested() { requested = false; }
	explicit operator bool() const { return choreographer; }
	ApplicationContext appContext() const { return app; }

protected:
	using PostFrameCallbackFunc = void (*)(AChoreographer* choreographer,
			AChoreographer_frameCallback callback, void* data);
	ApplicationContext app{};
	PostFrameCallbackFunc postFrameCallback{};
	AChoreographer *choreographer{};
	bool requested = false;
};

std::unique_ptr<FrameTimer> frameTimer{};

template <class T>
static void doOnFrame(T *timerObjPtr, int64_t frameTimeNanos)
{
	IG::Nanoseconds frameTime{frameTimeNanos};
	timerObjPtr->unsetRequested(); // Choreographer callbacks are one-shot
	auto app = timerObjPtr->appContext();
	iterateTimes(app.screens(), i)
	{
		auto s = app.screen(i);
		if(s->isPosted())
		{
			s->frameUpdate(frameTime);
		}
	}
}

ChoreographerFrameTimer::ChoreographerFrameTimer(ApplicationContext app):
	app{app}
{
	assert(app.androidSDK() >= 16);
	JNIEnv *env = app.mainThreadJniEnv();
	JavaInstMethod<jobject(jlong)> jNewChoreographerHelper{env, jBaseActivityCls, "newChoreographerHelper", "(J)Lcom/imagine/ChoreographerHelper;"};
	frameHelper = jNewChoreographerHelper(env, app.baseActivityObject(), (jlong)this);
	assert(frameHelper);
	frameHelper = env->NewGlobalRef(frameHelper);
	auto choreographerHelperCls = env->GetObjectClass(frameHelper);
	jPostFrame.setup(env, choreographerHelperCls, "postFrame", "()V");
	jUnpostFrame.setup(env, choreographerHelperCls, "unpostFrame", "()V");
	JNINativeMethod method[]
	{
		{
			"onFrame", "(JJ)Z",
			(void*)(jboolean (*)(JNIEnv*, jobject, jlong, jlong))
			([](JNIEnv* env, jobject thiz, jlong thisPtr, jlong frameTimeNanos)
			{
				doOnFrame((ChoreographerFrameTimer*)thisPtr, frameTimeNanos);
				return (jboolean)0;
			})
		}
	};
	env->RegisterNatives(choreographerHelperCls, method, std::size(method));
}

ChoreographerFrameTimer::~ChoreographerFrameTimer()
{
	if(frameHelper)
	{
		app.mainThreadJniEnv()->DeleteGlobalRef(frameHelper);
	}
}

void ChoreographerFrameTimer::scheduleVSync()
{
	assert(frameHelper);
	if(requested)
		return;
	requested = true;
	jPostFrame(app.mainThreadJniEnv(), frameHelper);
}

void ChoreographerFrameTimer::cancel()
{
	assert(frameHelper);
	if(!requested)
		return;
	requested = false;
	jUnpostFrame(app.mainThreadJniEnv(), frameHelper);
}

AChoreographerFrameTimer::AChoreographerFrameTimer(ApplicationContext app):
	app{app}
{
	assert(app.androidSDK() >= 24);
	AChoreographer* (*getInstance)(){};
	Base::loadSymbol(getInstance, {}, "AChoreographer_getInstance");
	assumeExpr(getInstance);
	Base::loadSymbol(postFrameCallback, {}, "AChoreographer_postFrameCallback");
	assumeExpr(postFrameCallback);
	choreographer = getInstance();
	assumeExpr(choreographer);
}

void AChoreographerFrameTimer::scheduleVSync()
{
	if(requested)
		return;
	requested = true;
	postFrameCallback(choreographer,
		[](long frameTimeNanos, void* data)
		{
			auto inst = (AChoreographerFrameTimer*)data;
			if(!inst->requested)
				return;
			doOnFrame(inst, frameTimeNanos);
		}, this);
}

void AChoreographerFrameTimer::cancel()
{
	requested = false;
}

void initFrameTimer(ApplicationContext app, Screen &screen)
{
	if(app.androidSDK() < 16)
	{
		// No OS frame timer
		frameTimer = std::make_unique<SimpleFrameTimer>(Base::EventLoop::forThread(), screen);
	}
	else if(sizeof(long) < sizeof(int64_t)
		|| app.androidSDK() < 24)
	{
		// Always use on 32-bit systems due to NDK API issue
		// that prevents 64-bit timestamp resolution
		logMsg("using Java Choreographer");
		frameTimer = std::make_unique<ChoreographerFrameTimer>(app);
	}
	else
	{
		logMsg("using native Choreographer");
		frameTimer = std::make_unique<AChoreographerFrameTimer>(app);
	}
}

}

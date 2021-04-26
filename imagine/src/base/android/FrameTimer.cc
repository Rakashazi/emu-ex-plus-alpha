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
#include <imagine/base/Application.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "android.hh"
#include "../common/SimpleFrameTimer.hh"
#include <android/choreographer.h>

namespace Base
{

class ChoreographerFrameTimer final : public FrameTimer
{
public:
	ChoreographerFrameTimer(AndroidApplication &, JNIEnv *, jobject baseActivity);
	~ChoreographerFrameTimer() final;
	void scheduleVSync() final;
	void cancel() final;
	void unsetRequested() { requested = false; }
	explicit operator bool() const { return frameHelper; }
	AndroidApplication &application() const { return *app; }

protected:
	AndroidApplication *app{};
	JNIEnv *jEnv{};
	JavaInstMethod<void()> jPostFrame{}, jUnpostFrame{};
	jobject frameHelper{};
	bool requested = false;
};

class AChoreographerFrameTimer final : public FrameTimer
{
public:
	AChoreographerFrameTimer(AndroidApplication &);
	~AChoreographerFrameTimer() final {};
	void scheduleVSync() final;
	void cancel() final;
	void unsetRequested() { requested = false; }
	explicit operator bool() const { return choreographer; }
	AndroidApplication &application() const { return *app; }

protected:
	using PostFrameCallbackFunc = void (*)(AChoreographer* choreographer,
			AChoreographer_frameCallback callback, void* data);
	AndroidApplication *app{};
	PostFrameCallbackFunc postFrameCallback{};
	AChoreographer *choreographer{};
	bool requested = false;
};

template <class T>
static void doOnFrame(T *timerObjPtr, int64_t frameTimeNanos)
{
	IG::Nanoseconds frameTime{frameTimeNanos};
	timerObjPtr->unsetRequested(); // Choreographer callbacks are one-shot
	auto &app = timerObjPtr->application();
	iterateTimes(app.screens(), i)
	{
		auto s = app.screen(i);
		if(s->isPosted())
		{
			s->frameUpdate(frameTime);
		}
	}
}

ChoreographerFrameTimer::ChoreographerFrameTimer(AndroidApplication &app, JNIEnv *env, jobject baseActivity):
	app{&app}, jEnv{env}
{
	JavaInstMethod<jobject(jlong)> jNewChoreographerHelper{env, app.baseActivityClass(), "newChoreographerHelper", "(J)Lcom/imagine/ChoreographerHelper;"};
	frameHelper = jNewChoreographerHelper(env, baseActivity, (jlong)this);
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
		jEnv->DeleteGlobalRef(frameHelper);
	}
}

void ChoreographerFrameTimer::scheduleVSync()
{
	assert(frameHelper);
	if(requested)
		return;
	requested = true;
	jPostFrame(jEnv, frameHelper);
}

void ChoreographerFrameTimer::cancel()
{
	assert(frameHelper);
	if(!requested)
		return;
	requested = false;
	jUnpostFrame(jEnv, frameHelper);
}

AChoreographerFrameTimer::AChoreographerFrameTimer(AndroidApplication &app):
	app{&app}
{
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

void AndroidApplication::initFrameTimer(JNIEnv *env, jobject baseActivity, int32_t androidSDK, Screen &screen)
{
	if(androidSDK < 16)
	{
		// No OS frame timer
		frameTimer_ = std::make_unique<SimpleFrameTimer>(Base::EventLoop::forThread(), screen);
	}
	else if(sizeof(long) < sizeof(int64_t) || androidSDK < 24)
	{
		// Always use on 32-bit systems due to NDK API issue
		// that prevents 64-bit timestamp resolution
		logMsg("using Java Choreographer");
		frameTimer_ = std::make_unique<ChoreographerFrameTimer>(*this, env, baseActivity);
	}
	else
	{
		logMsg("using native Choreographer");
		frameTimer_ = std::make_unique<AChoreographerFrameTimer>(*this);
	}
}

FrameTimer *AndroidApplication::frameTimer() const
{
	return frameTimer_.get();
}

}

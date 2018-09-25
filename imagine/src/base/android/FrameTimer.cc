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
#include <sys/eventfd.h>
#include <imagine/base/Screen.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../common/SimpleFrameTimer.hh"

namespace Base
{

class ChoreographerFrameTimer : public FrameTimer
{
public:
	JavaInstMethod<void()> jPostFrame{}, jUnpostFrame{};
	jobject frameHelper{};
	bool requested = false;

	bool init(JNIEnv *env, jobject activity);
	void scheduleVSync();
	void cancel();
};

std::unique_ptr<FrameTimer> frameTimer{};

bool ChoreographerFrameTimer::init(JNIEnv *env, jobject activity)
{
	if(Base::androidSDK() < 16)
	{
		return false;
	}
	//logMsg("using Choreographer for display updates");
	JavaInstMethod<jobject(jlong)> jNewChoreographerHelper{env, jBaseActivityCls, "newChoreographerHelper", "(J)Lcom/imagine/ChoreographerHelper;"};
	frameHelper = jNewChoreographerHelper(env, activity, (jlong)this);
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
				mainScreen().startDebugFrameStats(frameTimeNanos);
				auto choreographerFrameTimer = (ChoreographerFrameTimer*)thisPtr;
				choreographerFrameTimer->requested = false; // Choreographer callbacks are one-shot
				bool screenWasReallyPosted = false;
				iterateTimes(Screen::screens(), i)
				{
					auto s = Screen::screen(i);
					if(s->isPosted())
					{
						screenWasReallyPosted = true;
						s->frameUpdate(frameTimeNanos);
						s->prevFrameTimestamp = frameTimeNanos;
					}
				}
				assert(screenWasReallyPosted);
				mainScreen().endDebugFrameStats();
				return (jboolean)0;
			})
		}
	};
	env->RegisterNatives(choreographerHelperCls, method, IG::size(method));
	return true;
}

void ChoreographerFrameTimer::scheduleVSync()
{
	assert(frameHelper);
	if(requested)
		return;
	requested = true;
	jPostFrame(jEnvForThread(), frameHelper);
}

void ChoreographerFrameTimer::cancel()
{
	assert(frameHelper);
	if(!requested)
		return;
	requested = false;
	jUnpostFrame(jEnvForThread(), frameHelper);
}

void initFrameTimer(JNIEnv *env, jobject activity)
{
	if(Base::androidSDK() < 16)
	{
		auto timer = std::make_unique<SimpleFrameTimer>();
		timer->init(Base::EventLoop::forThread());
		frameTimer = std::move(timer);
	}
	else
	{
		auto timer = std::make_unique<ChoreographerFrameTimer>();
		timer->init(env, activity);
		frameTimer = std::move(timer);
	}
}


}

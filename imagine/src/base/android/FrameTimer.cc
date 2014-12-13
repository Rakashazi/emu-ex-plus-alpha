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
#include <imagine/base/GLContext.hh>
#include "internal.hh"
#include "android.hh"

namespace Base
{

class EventFDFrameTimer : public FrameTimer
{
public:
	int fd = -1;
	uint idle = 1;
	bool requested = false;

	bool init(JNIEnv *env, jobject activity);
	void scheduleVSync();
	void cancel();
};

class FrameworkFrameTimer : public FrameTimer
{
public:
	JavaInstMethod<void> jPostFrame, jUnpostFrame;
	jobject frameHelper = nullptr;
	bool requested = false;

	bool init(JNIEnv *env, jobject activity);
	void scheduleVSync();
	void cancel();
};

static EventFDFrameTimer eventFDFrameTimer;
static FrameworkFrameTimer frameworkFrameTimer;
FrameTimer *frameTimer = &frameworkFrameTimer;

bool EventFDFrameTimer::init(JNIEnv *env, jobject activity)
{
	if(fd >= 0)
		return true;
	fd = eventfd(0, 0);
	if(unlikely(fd == -1))
		return false;
	logMsg("eventfd %d for frame timer", fd);
	// this callback should behave as the "idle-handler" so input-related fds are processed in a timely manner
	int ret = ALooper_addFd(activityLooper(), fd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
		[](int fd, int, void *data)
		{
			auto &frameTimer = *((EventFDFrameTimer*)data);
			if(frameTimer.idle)
			{
				//logMsg("idled");
				frameTimer.idle--;
				return 1;
			}
			else
			{
				// "idle" every other call so other fds are processed
				// to avoid a frame of input lag
				frameTimer.idle = 1;
			}
			if(likely(inputQueue) && AInputQueue_hasEvents(inputQueue) == 1)
			{
				// some devices may delay reporting input events (stock rom on R800i for example),
				// check for any before rendering frame to avoid extra latency
				Input::processInput(inputQueue);
			}

			auto &screen = mainScreen();
			assert(screen.isPosted());
			assert(screen.currFrameTime);
			// force window draw so buffers swap and currFrameTime is updated after vsync
			deviceWindow()->setNeedsDraw(true);
			screen.frameUpdate(screen.currFrameTime);
			screen.prevFrameTime = screen.currFrameTime;
			GLContext::swapPresentedBuffers(*deviceWindow());
			if(screen.isPosted())
			{
				screen.currFrameTime = TimeSys::now().toNs();
			}
			else
			{
				frameTimer.cancel();
			}
			return 1;
		}, this);
	assert(ret == 1);
	return true;
}

void EventFDFrameTimer::scheduleVSync()
{
	assert(fd != -1);
	if(requested)
		return;
	requested = true;
	uint64_t post = 1;
	auto ret = write(fd, &post, sizeof(post));
	assert(ret == sizeof(post));
}

void EventFDFrameTimer::cancel()
{
	assert(fd != -1);
	if(!requested)
		return;
	requested = false;
	idle = 1; // force handler to idle since it could already be signaled by epoll
	uint64_t post;
	auto ret = read(fd, &post, sizeof(post));
	assert(ret == sizeof(post));
}

bool FrameworkFrameTimer::init(JNIEnv *env, jobject activity)
{
	if(Base::androidSDK() >= 16) // Choreographer
	{
		//logMsg("using Choreographer for display updates");
		JavaInstMethod<jobject> jNewChoreographerHelper;
		jNewChoreographerHelper.setup(env, jBaseActivityCls, "newChoreographerHelper", "()Lcom/imagine/ChoreographerHelper;");
		frameHelper = jNewChoreographerHelper(env, activity);
		assert(frameHelper);
		frameHelper = env->NewGlobalRef(frameHelper);
		auto choreographerHelperCls = env->GetObjectClass(frameHelper);
		jPostFrame.setup(env, choreographerHelperCls, "postFrame", "()V");
		jUnpostFrame.setup(env, choreographerHelperCls, "unpostFrame", "()V");
		JNINativeMethod method[] =
		{
			{
				"onFrame", "(J)Z",
				(void*)(jboolean JNICALL(*)(JNIEnv*, jobject, jlong))
				([](JNIEnv* env, jobject thiz, jlong frameTimeNanos)
				{
					mainScreen().startDebugFrameStats(frameTimeNanos);
					frameworkFrameTimer.requested = false; // Choreographer callbacks are one-shot
					bool screenWasReallyPosted = false;
					iterateTimes(Screen::screens(), i)
					{
						auto s = Screen::screen(i);
						if(s->isPosted())
						{
							screenWasReallyPosted = true;
							s->frameUpdate(frameTimeNanos);
							s->prevFrameTime = frameTimeNanos;
						}
					}
					assert(screenWasReallyPosted);
					mainScreen().endDebugFrameStats();
					return (jboolean)0;
				})
			}
		};
		env->RegisterNatives(choreographerHelperCls, method, sizeofArray(method));
	}
	else // MessageQueue.IdleHandler
	{
		logWarn("error creating eventfd: %d (%s), falling back to idle handler", errno, strerror(errno));
		JavaInstMethod<jobject> jNewIdleHelper;
		jNewIdleHelper.setup(env, jBaseActivityCls, "newIdleHelper", "()Lcom/imagine/BaseActivity$IdleHelper;");
		frameHelper = jNewIdleHelper(env, activity);
		assert(frameHelper);
		frameHelper = env->NewGlobalRef(frameHelper);
		auto idleHelperCls = env->GetObjectClass(frameHelper);
		jPostFrame.setup(env, idleHelperCls, "postFrame", "()V");
		jUnpostFrame.setup(env, idleHelperCls, "unpostFrame", "()V");
		JNINativeMethod method[]
		{
			{
				"onFrame", "()Z",
				(void*)(jboolean JNICALL(*)(JNIEnv*, jobject))
				([](JNIEnv* env, jobject thiz)
				{
					auto &screen = mainScreen();
					assert(screen.isPosted());
					// force window draw so buffers swap and currFrameTime is updated after vsync
					deviceWindow()->setNeedsDraw(true);
					screen.frameUpdate(screen.currFrameTime);
					screen.prevFrameTime = screen.currFrameTime;
					GLContext::swapPresentedBuffers(*deviceWindow());
					if(screen.isPosted())
					{
						screen.currFrameTime = TimeSys::now().toNs();
						return (jboolean)true;
					}
					else
					{
						frameworkFrameTimer.requested = false;
						return (jboolean)false;
					}
				})
			}
		};
		env->RegisterNatives(idleHelperCls, method, sizeofArray(method));
	}
	return true;
}

void FrameworkFrameTimer::scheduleVSync()
{
	assert(frameHelper);
	if(requested)
		return;
	requested = true;
	jPostFrame(jEnv(), frameHelper);
}

void FrameworkFrameTimer::cancel()
{
	assert(frameHelper);
	if(!requested)
		return;
	requested = false;
	jUnpostFrame(jEnv(), frameHelper);
}

void initFrameTimer(JNIEnv *env, jobject activity)
{
	if(Base::androidSDK() < 16)
	{
		// use eventfd if OS supports it
		if(eventFDFrameTimer.init(env, activity))
		{
			frameTimer = &eventFDFrameTimer;
			return;
		}
		// fallback to MessageQueue.IdleHandler
		frameworkFrameTimer.init(env, activity);
	}
	else
	{
		// use Choreographer
		frameworkFrameTimer.init(env, activity);
	}
}


}

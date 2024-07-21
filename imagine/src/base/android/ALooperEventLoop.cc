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

#include <imagine/base/EventLoop.hh>
#include <imagine/util/format.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>
#include <unistd.h>

namespace IG
{

constexpr SystemLogger log{"EventLoop"};
extern pthread_key_t jEnvPThreadKey;

static int eventCallback(int fd, int events, void *data)
{
	auto &info = *((ALooperFDEventSourceInfo*)data);
	bool keep = info.callback(fd, events);
	if(!keep)
	{
		info.looper = {};
	}
	return keep;
}

ALooperFDEventSource::ALooperFDEventSource(MaybeUniqueFileDescriptor fd, FDEventSourceDesc, PollEventDelegate del):
	info{fd.get() != -1 ? std::make_unique<ALooperFDEventSourceInfo>(del) : std::unique_ptr<ALooperFDEventSourceInfo>{}},
	fd_{std::move(fd)} {}

ALooperFDEventSource::ALooperFDEventSource(ALooperFDEventSource &&o) noexcept
{
	*this = std::move(o);
}

ALooperFDEventSource &ALooperFDEventSource::operator=(ALooperFDEventSource &&o) noexcept
{
	deinit();
	info = std::move(o.info);
	fd_ = std::move(o.fd_);
	return *this;
}

ALooperFDEventSource::~ALooperFDEventSource()
{
	deinit();
}

bool FDEventSource::attach(EventLoop loop, PollEventFlags events)
{
	if(fd_.get() == -1) [[unlikely]]
	{
		log.error("trying to attach without valid fd");
		return false;
	}
	detach();
	log.info("adding fd:{} to looper:{} ({})", fd_.get(), (void*)loop.nativeObject(), debugLabel());
	if(!loop)
		loop = EventLoop::forThread();
	assumeExpr(info);
	info->looper = loop.nativeObject();
	if(auto res = ALooper_addFd(loop.nativeObject(), fd_, ALOOPER_POLL_CALLBACK, events, eventCallback, info.get());
		res != 1)
	{
		return false;
	}
	return true;
}

void FDEventSource::detach()
{
	if(!hasEventLoop())
		return;
	log.info("removing fd:{} from looper ({})", fd_.get(), debugLabel());
	ALooper_removeFd(info->looper, fd_);
	info->looper = {};
}

void FDEventSource::setEvents(PollEventFlags events)
{
	if(!hasEventLoop())
	{
		log.error("trying to set events without event loop");
		return;
	}
	ALooper_addFd(info->looper, fd_, ALOOPER_POLL_CALLBACK, events, eventCallback, info.get());
}

void FDEventSource::dispatchEvents(PollEventFlags events)
{
	eventCallback(fd(), events, info.get());
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	assumeExpr(info);
	info->callback = callback;
}

bool FDEventSource::hasEventLoop() const
{
	return info && info->looper;
}

int FDEventSource::fd() const
{
	return fd_;
}

void ALooperFDEventSource::deinit()
{
	static_cast<FDEventSource*>(this)->detach();
}

EventLoop EventLoop::forThread()
{
	return {ALooper_forThread()};
}

EventLoop EventLoop::makeForThread()
{
	return {ALooper_prepare(0)};
}

static const char *aLooperPollResultStr(int res)
{
	switch(res)
	{
		case ALOOPER_POLL_CALLBACK: return "Callback";
		case ALOOPER_POLL_ERROR: return "Error";
		case ALOOPER_POLL_TIMEOUT: return "Timeout";
		case ALOOPER_POLL_WAKE: return "Wake";
	}
	return "Unknown";
}

struct JavaLooperContext
{
	JNIEnv* env{};
	jclass cls{};
	jobject looper{};
};

static JavaLooperContext javaLooperContext()
{
	auto env = static_cast<JNIEnv*>(pthread_getspecific(jEnvPThreadKey));
	if(!env)
		return {};
	auto looperClass = env->FindClass("android/os/Looper");
	assert(looperClass);
	JNI::ClassMethod<jobject()> myLooper{env, looperClass, "myLooper", "()Landroid/os/Looper;"};
	return {env, looperClass, myLooper(env, looperClass)};
}

void EventLoop::run(const bool& condition)
{
	if(auto jLooperCtx = javaLooperContext(); jLooperCtx.looper)
	{
		JNI::ClassMethod<void()> loop{jLooperCtx.env, jLooperCtx.cls, "loop", "()V"};
		while(condition)
		{
			loop(jLooperCtx.env, jLooperCtx.cls);
		}
	}
	else
	{
		while(condition)
		{
			int res = ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
			if(res != ALOOPER_POLL_CALLBACK && res != ALOOPER_POLL_WAKE)
				log.error("ALooper_pollOnce returned:{}", aLooperPollResultStr(res));
		}
	}
}

void EventLoop::stop()
{
	if(auto jLooperCtx = javaLooperContext(); jLooperCtx.looper)
	{
		JNI::InstMethod<void()> quit{jLooperCtx.env, jLooperCtx.cls, "quit", "()V"};
		quit(jLooperCtx.env, jLooperCtx.looper);
	}
	else
	{
		ALooper_wake(looper);
	}
}

EventLoop::operator bool() const
{
	return looper;
}

}

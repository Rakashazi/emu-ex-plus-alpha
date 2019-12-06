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

#define LOGTAG "TimerFD"
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <imagine/base/Timer.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>

#if __has_include(<sys/timerfd.h>) && (!defined ANDROID || __ANDROID_API__ >= 19)
#include <sys/timerfd.h>
#else
#include <time.h>
#include <sys/syscall.h>
#include <linux/fcntl.h>

#ifndef TFD_NONBLOCK
#define TFD_NONBLOCK O_NONBLOCK
#endif

#ifndef TFD_CLOEXEC
#define TFD_CLOEXEC O_CLOEXEC
#endif

static int timerfd_create(clockid_t clock_id, int flags)
{
	return syscall(__NR_timerfd_create, clock_id, flags);
}

static int timerfd_settime(int ufd, int flags,
					const struct itimerspec *utmr,
					struct itimerspec *otmr)
{
	return syscall(__NR_timerfd_settime, ufd, flags, utmr, otmr);
}
#endif

namespace Base
{

int TimerFD::fd() const
{
	return fdSrc.fd();
}

bool TimerFD::arm(timespec time, timespec repeatInterval, EventLoop loop, bool shouldReuseResources)
{
	reuseResources = shouldReuseResources;
	bool rearm = false;
	if(fd() == -1)
	{
		int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
		if(fd == -1)
		{
			logErr("error creating timerfd");
			return false;
		}
		if(!loop)
			loop = EventLoop::forThread();
		//logMsg("made timerfd: %d", fd);
		fdSrc = {fd, loop,
			[this](int fd, int events)
			{
				timerFired();
				return true;
			}};
	}
	else
	{
		rearm = true;
	}

	logMsg("%s %sfd:%d to run in %lds & %ldns, repeat every %lds & %ldns",
		rearm ? "re-arming" : "creating", reuseResources ? "reusable " : "",
		fd(), (long)time.tv_sec, (long)time.tv_nsec,
		(long)repeatInterval.tv_sec, (long)repeatInterval.tv_nsec);
	if(repeatInterval.tv_sec || repeatInterval.tv_nsec)
	{
		repeating = true;
	}
	struct itimerspec newTime{repeatInterval, time};
	if(timerfd_settime(fd(), 0, &newTime, nullptr) != 0)
	{
		logErr("error in timerfd_settime: %s", strerror(errno));
		return false;
	}
	armed = true;
	return true;
}

void TimerFD::deinit()
{
	if(fd() == -1)
		return;
	logMsg("closing fd:%d", fd());
	fdSrc.closeFD();
	armed = false;
}

void Timer::deinit()
{
	TimerFD::deinit();
}

void TimerFD::timerFired()
{
	//logMsg("callback ready for fd:%d", fd);
	if(unlikely(!armed))
	{
		logMsg("disarmed after fd became ready");
		return;
	}
	uint64_t timesFired;
	int bytes = ::read(fd(), &timesFired, 8);
	armed = repeating; // disarm timer if non-repeating, can be re-armed in callback()
	callback();
	if(!armed && !reuseResources)
		deinit();
}

void Timer::callbackAfterNSec(CallbackDelegate callback, int ns, int repeatNs, EventLoop loop, Flags flags)
{
	this->callback = callback;
	int seconds = ns / 1000000000;
	long leftoverNs = ns % 1000000000;
	int repeatSeconds = repeatNs / 1000000000;
	long repeatLeftoverNs = repeatNs % 1000000000;
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, loop, flags & HINT_REUSE))
	{
		logErr("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::callbackAfterMSec(CallbackDelegate callback, int ms, int repeatMs, EventLoop loop, Flags flags)
{
	this->callback = callback;
	int seconds = ms / 1000;
	int leftoverMs = ms % 1000;
	long leftoverNs = leftoverMs * 1000000;
	int repeatSeconds = repeatMs / 1000;
	int repeatLeftoverMs = repeatMs % 1000;
	long repeatLeftoverNs = repeatLeftoverMs * 1000000;
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, loop, flags & HINT_REUSE))
	{
		logErr("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::callbackAfterSec(CallbackDelegate callback, int s, int repeatS, EventLoop loop, Flags flags)
{
	this->callback = callback;
	if(!arm({s, 0}, {repeatS, 0}, loop, flags & HINT_REUSE))
	{
		logErr("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::callbackAfterNSec(CallbackDelegate callback, int ns, EventLoop loop)
{
	callbackAfterNSec(callback, ns, 0, loop, HINT_NONE);
}

void Timer::callbackAfterMSec(CallbackDelegate callback, int ms, EventLoop loop)
{
	callbackAfterMSec(callback, ms, 0, loop, HINT_NONE);
}

void Timer::callbackAfterSec(CallbackDelegate callback, int s, EventLoop loop)
{
	callbackAfterSec(callback, s, 0, loop, HINT_NONE);
}

void Timer::cancel()
{
	if(reuseResources)
	{
		if(armed)
		{
			// disarm timer
			assert(fd());
			logMsg("disarming fd:%d", fd());
			struct itimerspec newTime{{0}};
			timerfd_settime(fd(), 0, &newTime, nullptr);
			armed = false;
		}
	}
	else
		deinit();
}

}

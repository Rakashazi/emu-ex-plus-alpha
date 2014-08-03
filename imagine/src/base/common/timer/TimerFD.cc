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
#include <imagine/base/Timer.hh>
#include <imagine/base/EventLoopFileSource.hh>
#include <imagine/logger/logger.h>

#if defined __ANDROID__
#include "../../android/android.hh"
// No sys/timerfd.h on Android, need to use syscall
#include <time.h>
#include <sys/syscall.h>

static int timerfd_create(clockid_t __clock_id, int __flags)
{
	return syscall(__NR_timerfd_create, __clock_id, __flags);
}

static int timerfd_settime(int __ufd, int __flags,
					const struct itimerspec *__utmr,
					struct itimerspec *__otmr)
{
	return syscall(__NR_timerfd_settime, __ufd, __flags, __utmr, __otmr);
}
#else
#include <sys/timerfd.h>
#endif

namespace Base
{

bool TimerFD::arm(timespec time, timespec repeatInterval, bool shouldReuseResources)
{
	reuseResources = shouldReuseResources;
	bool rearm = false;
	if(fd == -1)
	{
		fd = timerfd_create(CLOCK_MONOTONIC, 0);
		if(fd == -1)
		{
			logErr("error creating timerfd");
			return false;
		}
		//logMsg("created timerfd: %d", fd);
		#if defined __ANDROID__
		auto ret = ALooper_addFd(Base::activityLooper(), fd, ALOOPER_POLL_CALLBACK, Base::POLLEV_IN,
			[](int fd, int events, void* data)
			{
				auto &inst = *((TimerFD*)data);
				inst.timerFired();
				return 1;
			}, this);
		assert(ret == 1);
		#else
		fdSrc.init(fd,
			[this](int fd, int events)
			{
				timerFired();
				return 1;
			});
		#endif
	}
	else
	{
		rearm = true;
	}

	logMsg("%s %stimerfd %d to run in %ld second(s) and %ld ns", rearm ? "re-arming" : "creating", reuseResources ? "reusable " : "",
		fd, (long)time.tv_sec, (long)time.tv_nsec);
	if(repeatInterval.tv_sec || repeatInterval.tv_nsec)
	{
		repeating = true;
		logMsg("repeating every %ld second(s) and %ld ns", (long)repeatInterval.tv_sec, (long)repeatInterval.tv_nsec);
	}
	struct itimerspec newTime { repeatInterval, time };
	if(timerfd_settime(fd, 0, &newTime, nullptr) != 0)
	{
		logErr("error in timerfd_settime: %s", strerror(errno));
		return false;
	}
	armed = true;
	return true;
}

void TimerFD::deinit()
{
	if(fd >= 0)
	{
		logMsg("closing timerfd %d", fd);
		#if defined __ANDROID__
		ALooper_removeFd(Base::activityLooper(), fd);
		#else
		fdSrc.deinit();
		#endif
		close(fd);
		fd = -1;
		armed = false;
	}
}

void Timer::deinit()
{
	TimerFD::deinit();
}

void TimerFD::timerFired()
{
	logMsg("running callback, fd %d", fd);
	armed = repeating; // disarm timer if non-repeating, can be re-armed in callback()
	callback();
	if(!armed && !reuseResources)
		deinit();
	else if(fd >= 0)
	{
		uint64_t timesFired;
		int bytes = ::read(fd, &timesFired, 8);
		assert(bytes != -1);
	}
}

void Timer::callbackAfterNSec(CallbackDelegate callback, int ns, int repeatNs, Flags flags)
{
	var_selfs(callback);
	int seconds = ns / 1000000000;
	long leftoverNs = ns % 1000000000;
	int repeatSeconds = repeatNs / 1000000000;
	long repeatLeftoverNs = repeatNs % 1000000000;
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, flags & HINT_REUSE))
	{
		bug_exit("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::callbackAfterMSec(CallbackDelegate callback, int ms, int repeatMs, Flags flags)
{
	var_selfs(callback);
	int seconds = ms / 1000;
	int leftoverMs = ms % 1000;
	long leftoverNs = leftoverMs * 1000000;
	int repeatSeconds = repeatMs / 1000;
	int repeatLeftoverMs = repeatMs % 1000;
	long repeatLeftoverNs = repeatLeftoverMs * 1000000;
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, flags & HINT_REUSE))
	{
		bug_exit("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::callbackAfterSec(CallbackDelegate callback, int s, int repeatS, Flags flags)
{
	var_selfs(callback);
	if(!arm({s, 0}, {repeatS, 0}, flags & HINT_REUSE))
	{
		bug_exit("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::callbackAfterNSec(CallbackDelegate callback, int ns)
{
	callbackAfterNSec(callback, ns, 0, HINT_NONE);
}

void Timer::callbackAfterMSec(CallbackDelegate callback, int ms)
{
	callbackAfterMSec(callback, ms, 0, HINT_NONE);
}

void Timer::callbackAfterSec(CallbackDelegate callback, int s)
{
	callbackAfterSec(callback, s, 0, HINT_NONE);
}

void Timer::cancel()
{
	if(reuseResources)
	{
		if(armed)
		{
			// disarm timer
			assert(fd);
			logMsg("disarming timerfd: %d", fd);
			struct itimerspec newTime{{0}};
			timerfd_settime(fd, 0, &newTime, nullptr);
			armed = false;
		}
	}
	else
		deinit();
}

}

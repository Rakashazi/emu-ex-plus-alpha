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
#include <imagine/base/Timer.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#if __has_include(<sys/timerfd.h>) && (!defined __ANDROID__ || ANDROID_MIN_API >= 19)
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

enum
{
	TFD_TIMER_ABSTIME = 1 << 0,
};

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

static int timerfd_gettime(int ufd,
					struct itimerspec *otmr)
{
	return syscall(__NR_timerfd_gettime, ufd, otmr);
}
#endif

namespace IG
{

TimerFD::TimerFD(const char *debugLabel, CallbackDelegate c):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	callback_{std::make_unique<CallbackDelegate>(c)},
	fdSrc{label(), UniqueFileDescriptor{timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)}}
{
	if(fdSrc.fd() == -1)
	{
		logErr("error creating timerfd");
	}
}

bool TimerFD::arm(timespec time, timespec repeatInterval, int flags, EventLoop loop)
{
	if(!fdSrc.hasEventLoop())
	{
		if(!loop)
			loop = EventLoop::forThread();
		fdSrc.attach(loop,
			[callback = callback_.get()](int fd, int)
			{
				//logMsg("callback ready for fd:%d", fd);
				uint64_t timesFired;
				auto ret = ::read(fd, &timesFired, 8);
				if(ret == -1)
				{
					if(Config::DEBUG_BUILD && errno != EAGAIN)
						logErr("error reading timerfd in callback");
					return false;
				}
				bool keepTimer = (*callback)();
				return keepTimer;
			});
	}
	struct itimerspec newTime{repeatInterval, time};
	if(timerfd_settime(fdSrc.fd(), flags, &newTime, nullptr) != 0)
	{
		logErr("error in timerfd_settime: %s (%s)", strerror(errno), label());
		return false;
	}
	return true;
}

void Timer::run(Time time, Time repeatTime, bool isAbsTime, EventLoop loop, CallbackDelegate callback)
{
	if(callback)
		setCallback(callback);
	time_t seconds = time.count() / 1000000000l;
	long leftoverNs = time.count() % 1000000000l;
	time_t repeatSeconds = repeatTime.count() / 1000000000l;
	long repeatLeftoverNs = repeatTime.count() % 1000000000l;
	if(Config::DEBUG_BUILD)
	{
		IG::FloatSeconds relTime = isAbsTime ? time - IG::steadyClockTimestamp() : time;
		logMsg("arming fd:%d (%s) to run in:%.4fs repeats:%.4fs",
			fdSrc.fd(), label(), relTime.count(), IG::FloatSeconds(repeatTime).count());
	}
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, isAbsTime ? TFD_TIMER_ABSTIME : 0, loop))
	{
		logErr("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::cancel()
{
	fdSrc.detach();
	struct itimerspec newTime{};
	timerfd_settime(fdSrc.fd(), 0, &newTime, nullptr);
}

void Timer::setCallback(CallbackDelegate callback)
{
	*callback_ = callback;
}

void Timer::dispatchEarly()
{
	cancel();
	(*callback_)();
}

bool Timer::isArmed()
{
	return fdSrc.hasEventLoop();
}

Timer::operator bool() const
{
	return fdSrc.fd() != -1;
}

const char *TimerFD::label()
{
	return debugLabel;
}

}

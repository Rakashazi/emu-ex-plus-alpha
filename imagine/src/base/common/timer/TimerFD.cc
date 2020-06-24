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
#include <errno.h>
#include <cstring>

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

static int timerfd_gettime(int ufd,
					struct itimerspec *otmr)
{
	return syscall(__NR_timerfd_gettime, ufd, otmr);
}
#endif

namespace Base
{

TimerFD::TimerFD(const char *debugLabel, CallbackDelegate c):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	fdSrc{label(), timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)},
	callback_{std::make_unique<CallbackDelegate>(c)}
{
	if(fdSrc.fd() == -1)
	{
		logErr("error creating timerfd");
	}
}

TimerFD::TimerFD(TimerFD &&o)
{
	*this = std::move(o);
}

TimerFD &TimerFD::operator=(TimerFD &&o)
{
	deinit();
	fdSrc = std::move(o.fdSrc);
	callback_ = std::move(o.callback_);
	debugLabel = o.debugLabel;
	return *this;
}

TimerFD::~TimerFD()
{
	deinit();
}

bool TimerFD::arm(timespec time, timespec repeatInterval, EventLoop loop)
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
				int ret = ::read(fd, &timesFired, 8);
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
	logMsg("arming fd:%d (%s) to run in %lds & %ldns, repeat every %lds & %ldns (%s)",
		fdSrc.fd(), label(), (long)time.tv_sec, (long)time.tv_nsec,
		(long)repeatInterval.tv_sec, (long)repeatInterval.tv_nsec, label());
	struct itimerspec newTime{repeatInterval, time};
	if(timerfd_settime(fdSrc.fd(), 0, &newTime, nullptr) != 0)
	{
		logErr("error in timerfd_settime: %s (%s)", strerror(errno), label());
		return false;
	}
	return true;
}

void TimerFD::deinit()
{
	if(fdSrc.fd() == -1)
		return;
	logMsg("closing fd:%d (%s)", fdSrc.fd(), label());
	fdSrc.closeFD();
}

void Timer::run(Time time, Time repeatTime, EventLoop loop, CallbackDelegate callback)
{
	if(callback)
		setCallback(callback);
	int seconds = time.count() / 1000000000;
	long leftoverNs = time.count() % 1000000000;
	int repeatSeconds = repeatTime.count() / 1000000000;
	long repeatLeftoverNs = repeatTime.count() % 1000000000;
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, loop))
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

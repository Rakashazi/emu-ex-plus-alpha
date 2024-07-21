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

#include <imagine/base/Timer.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/format.hh>
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

constexpr SystemLogger log{"Timer"};

static void cancelTimer(int fd)
{
	struct itimerspec newTime{};
	timerfd_settime(fd, 0, &newTime, nullptr);
}

TimerFD::TimerFD(TimerDesc desc, CallbackDelegate del):
	callback_{std::make_unique<CallbackDelegate>(del)},
	fdSrc
	{
		UniqueFileDescriptor{timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)},
		{.debugLabel = desc.debugLabel, .eventLoop = desc.eventLoop},
		[callback = callback_.get()](int fd, int)
		{
			//log.debug("callback ready for fd:{}", fd);
			uint64_t timesFired;
			auto ret = ::read(fd, &timesFired, 8);
			if(ret == -1)
			{
				if(Config::DEBUG_BUILD && errno != EAGAIN)
					log.error("error reading timerfd in callback");
				return true;
			}
			if(!(*callback)())
				cancelTimer(fd);
			return true;
		}
	} {}

bool TimerFD::arm(timespec time, timespec repeatInterval, int flags)
{
	struct itimerspec newTime{repeatInterval, time};
	if(timerfd_settime(fdSrc.fd(), flags, &newTime, nullptr) != 0)
	{
		log.error("error in timerfd_settime:{} ({})", strerror(errno), fdSrc.debugLabel());
		return false;
	}
	return true;
}

void Timer::run(Time time, Time repeatTime, bool isAbsTime, CallbackDelegate callback)
{
	if(callback)
		setCallback(callback);
	time_t seconds = time.count() / 1000000000l;
	long leftoverNs = time.count() % 1000000000l;
	time_t repeatSeconds = repeatTime.count() / 1000000000l;
	long repeatLeftoverNs = repeatTime.count() % 1000000000l;
	if(Config::DEBUG_BUILD)
	{
		FloatSeconds relTime = isAbsTime ? time - SteadyClock::now().time_since_epoch() : time;
		log.info("arming fd:{} ({}) to run in:{}s repeats:{}s",
			fdSrc.fd(), fdSrc.debugLabel(), relTime.count(), FloatSeconds(repeatTime).count());
	}
	if(!arm({seconds, leftoverNs}, {repeatSeconds, repeatLeftoverNs}, isAbsTime ? TFD_TIMER_ABSTIME : 0))
	{
		log.error("failed to setup timer, OS resources may be low or bad parameters present");
	}
}

void Timer::cancel()
{
	cancelTimer(fdSrc.fd());
}

void Timer::setCallback(CallbackDelegate callback)
{
	*callback_ = callback;
}

void Timer::setEventLoop(EventLoop loop)
{
	cancel();
	fdSrc.attach(loop);
}

void Timer::dispatchEarly()
{
	cancel();
	(*callback_)();
}

bool Timer::isArmed()
{
	struct itimerspec currTime{};
	timerfd_gettime(fdSrc.fd(), &currTime);
	return currTime.it_value.tv_nsec || currTime.it_value.tv_sec;
}

Timer::operator bool() const
{
	return fdSrc.fd() != -1;
}

}

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

#include <imagine/base/CustomEvent.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/errno.h>
#define USE_EVENTFD
#else
// kqueue
#include <sys/event.h>
static constexpr uintptr_t CUSTOM_IDENT = 1;
#endif

namespace IG
{

constexpr SystemLogger log{"CustomEvent"};

static IG::UniqueFileDescriptor makeEventFD()
{
#ifdef USE_EVENTFD
	return eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
#else
	int fd = kqueue();
	if(fd == -1)
		return -1;
	struct kevent kev;
	EV_SET(&kev, CUSTOM_IDENT, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
	kevent(fd, &kev, 1, nullptr, 0, nullptr);
	return fd;
#endif
}

static void notifyEventFD(int fd, [[maybe_unused]] const char *debugLabel)
{
#ifdef USE_EVENTFD
	eventfd_t counter = 1;
	auto ret = write(fd, &counter, sizeof(counter));
	if(ret == -1)
	{
		log.error("error writing eventfd:{} ({})", fd, debugLabel);
	}
#else
	struct kevent kev;
	EV_SET(&kev, CUSTOM_IDENT, EVFILT_USER, EV_ENABLE, NOTE_TRIGGER, 0, nullptr);
	kevent(fd, &kev, 1, nullptr, 0, nullptr);
#endif
}

static void cancelEventFD(int fd, [[maybe_unused]] const char *debugLabel)
{
#ifdef USE_EVENTFD
	eventfd_t counter;
	auto ret = read(fd, &counter, sizeof(counter));
	if(ret == -1)
	{
		if(Config::DEBUG_BUILD && errno != EAGAIN)
			log.error("error reading eventfd:{} ({})", fd, debugLabel);
	}
#else
	struct kevent kev;
	EV_SET(&kev, CUSTOM_IDENT, EVFILT_USER, EV_DISABLE, 0, 0, nullptr);
	kevent(fd, &kev, 1, nullptr, 0, nullptr);
#endif
}

FDCustomEvent::FDCustomEvent(FDEventSourceDesc desc, PollEventDelegate del):
	fdSrc{makeEventFD(), {.debugLabel = desc.debugLabel, .eventLoop = desc.eventLoop}, del}
{
	if(fdSrc.fd() == -1)
	{
		log.error("error creating fd ({})", desc.debugLabel);
	}
}

void CustomEvent::detach()
{
	fdSrc.detach();
}

void CustomEvent::notify()
{
	notifyEventFD(fdSrc.fd(), debugLabel());
}

void CustomEvent::cancel()
{
	cancelEventFD(fdSrc.fd(), debugLabel());
}

bool CustomEvent::isAttached() const
{
	return fdSrc.hasEventLoop();
}

CustomEvent::operator bool() const
{
	return fdSrc.fd() != -1;
}

bool FDCustomEvent::shouldPerformCallback(int fd)
{
	#ifdef USE_EVENTFD
	eventfd_t counter;
	auto ret = read(fd, &counter, sizeof(counter));
	if(ret == -1)
	{
		if(Config::DEBUG_BUILD && errno != EAGAIN)
			log.error("error reading eventfd:{} in callback", fd);
		return false;
	}
	#else
	struct timespec timeout{};
	struct kevent kev;
	int ret = kevent(fd, nullptr, 0, &kev, 1, &timeout);
	if(ret < 1)
	{
		if(ret == -1)
			log.error("error in kevent() in callback");
		return false;
	}
	#endif
	return true;
}

}

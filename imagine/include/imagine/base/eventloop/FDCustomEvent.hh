#pragma once

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
#include <atomic>

namespace Base
{

class FDCustomEvent
{
protected:
	FDEventSource fdSrc{};
	CustomEventDelegate callback{};
	std::atomic_bool cancelled = false;
	#ifndef NDEBUG
	const char *debugLabel{};
	#endif

	const char *label();

public:
	#ifdef NDEBUG
	FDCustomEvent();
	FDCustomEvent(const char *debugLabel): FDCustomEvent() {}
	#else
	FDCustomEvent() : FDCustomEvent{nullptr} {}
	FDCustomEvent(const char *debugLabel);
	#endif

	bool operator ==(FDCustomEvent const& rhs) const
	{
		return fdSrc.fd() == rhs.fdSrc.fd();
	}

	explicit operator bool() const
	{
		return fdSrc.fd() != -1;
	}
};

using CustomEventImpl = FDCustomEvent;

}

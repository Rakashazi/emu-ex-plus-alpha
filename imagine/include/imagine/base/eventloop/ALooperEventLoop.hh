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

#include <imagine/base/baseDefs.hh>
#include <imagine/util/used.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <android/looper.h>
#include <memory>
#include <utility>

namespace IG
{

constexpr int pollEventInput = ALOOPER_EVENT_INPUT, pollEventOutput = ALOOPER_EVENT_OUTPUT,
	pollEventError = ALOOPER_EVENT_ERROR, pollEventHangUp = ALOOPER_EVENT_HANGUP;

struct ALooperFDEventSourceInfo
{
	PollEventDelegate callback{};
	ALooper* looper{};
};

class ALooperFDEventSource
{
public:
	ALooperFDEventSource(MaybeUniqueFileDescriptor, FDEventSourceDesc, PollEventDelegate);
	ALooperFDEventSource(ALooperFDEventSource&&) noexcept;
	ALooperFDEventSource &operator=(ALooperFDEventSource&&) noexcept;
	~ALooperFDEventSource();

protected:
	std::unique_ptr<ALooperFDEventSourceInfo> info;
	MaybeUniqueFileDescriptor fd_;

	void deinit();
};

using FDEventSourceImpl = ALooperFDEventSource;

class ALooperEventLoop
{
public:
	constexpr ALooperEventLoop() = default;
	constexpr ALooperEventLoop(ALooper* looper): looper{looper} {}
	ALooper* nativeObject() const { return looper; }

protected:
	ALooper* looper{};
};

using EventLoopImpl = ALooperEventLoop;

}

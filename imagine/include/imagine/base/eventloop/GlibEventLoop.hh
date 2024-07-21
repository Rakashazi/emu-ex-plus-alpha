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
#include <glib.h>
#include <memory>
#include <utility>

namespace IG
{

constexpr int pollEventInput = G_IO_IN, pollEventOutput = G_IO_OUT,
	pollEventError = G_IO_ERR, pollEventHangUp = G_IO_HUP;

struct PollEventGSource : public GSource
{
	PollEventDelegate callback{};
};

void destroyGSource(GSource *);

struct GSourceDeleter
{
	void operator()(GSource *src) const
	{
		destroyGSource(src);
	}
};
using UniqueGSource = std::unique_ptr<GSource, GSourceDeleter>;

class GlibFDEventSource
{
public:
	GlibFDEventSource(MaybeUniqueFileDescriptor, FDEventSourceDesc, PollEventDelegate);

protected:
	UniqueGSource source{};
	gpointer tag{};
	MaybeUniqueFileDescriptor fd_{};

	static PollEventDelegate& getDelegate(GSource*);
	static UniqueGSource makeSource(PollEventDelegate);
};

using FDEventSourceImpl = GlibFDEventSource;

class GlibEventLoop
{
public:
	constexpr GlibEventLoop() = default;
	constexpr GlibEventLoop(GMainContext *ctx): mainContext{ctx} {}
	GMainContext *nativeObject() const { return mainContext; }

protected:
	GMainContext *mainContext{};
};

using EventLoopImpl = GlibEventLoop;

}

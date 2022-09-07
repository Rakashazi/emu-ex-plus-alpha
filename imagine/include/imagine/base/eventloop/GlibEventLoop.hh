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

#include <imagine/base/eventLoopDefs.hh>
#include <imagine/util/used.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <glib.h>
#include <memory>

namespace IG
{

static const int POLLEV_IN = G_IO_IN, POLLEV_OUT = G_IO_OUT, POLLEV_ERR = G_IO_ERR, POLLEV_HUP = G_IO_HUP;

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
	constexpr GlibFDEventSource() = default;
	GlibFDEventSource(MaybeUniqueFileDescriptor fd) : GlibFDEventSource{nullptr, std::move(fd)} {}
	GlibFDEventSource(const char *debugLabel, MaybeUniqueFileDescriptor fd);

protected:
	IG_UseMemberIf(Config::DEBUG_BUILD, const char *, debugLabel){};
	UniqueGSource fdSource{};
	gpointer tag{};
	MaybeUniqueFileDescriptor fd_{};
	IG_UseMemberIfOrConstant(Config::DEBUG_BUILD, bool, true, usingGlibSource){};

	const char *label() const;
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

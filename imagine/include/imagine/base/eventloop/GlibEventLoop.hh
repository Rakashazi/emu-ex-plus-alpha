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
#include <imagine/util/typeTraits.hh>
#include <glib.h>

namespace Base
{

static const int POLLEV_IN = G_IO_IN, POLLEV_OUT = G_IO_OUT, POLLEV_ERR = G_IO_ERR, POLLEV_HUP = G_IO_HUP;

struct GSource2 : public GSource
{
	PollEventDelegate callback{};
};

class GlibFDEventSource
{
public:
	constexpr GlibFDEventSource() {}
	GlibFDEventSource(int fd) : GlibFDEventSource{nullptr, fd} {}
	GlibFDEventSource(const char *debugLabel, int fd);
	GlibFDEventSource(GlibFDEventSource &&o);
	GlibFDEventSource &operator=(GlibFDEventSource &&o);
	~GlibFDEventSource();

protected:
	[[no_unique_address]] IG::UseTypeIf<Config::DEBUG_BUILD, const char *> debugLabel{};
	GSource2 *source{};
	gpointer tag{};
	int fd_ = -1;

	bool makeAndAttachSource(GSourceFuncs *fdSourceFuncs,
		PollEventDelegate callback_, GIOCondition events, GMainContext *ctx);
	void deinit();
	const char *label();
};

using FDEventSourceImpl = GlibFDEventSource;

class GlibEventLoop
{
public:
	constexpr GlibEventLoop() {}
	constexpr GlibEventLoop(GMainContext *ctx): mainContext{ctx} {}
	GMainContext *nativeObject() const { return mainContext; }

protected:
	GMainContext *mainContext{};
};

using EventLoopImpl = GlibEventLoop;

}

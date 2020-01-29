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

#include <glib.h>
#include <imagine/base/eventLoopDefs.hh>

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
	#ifdef NDEBUG
	GlibFDEventSource(int fd);
	GlibFDEventSource(const char *debugLabel, int fd): GlibFDEventSource(fd) {}
	#else
	GlibFDEventSource(int fd) : GlibFDEventSource{nullptr, fd} {}
	GlibFDEventSource(const char *debugLabel, int fd);
	#endif
	GlibFDEventSource(GlibFDEventSource &&o);
	GlibFDEventSource &operator=(GlibFDEventSource &&o);
	~GlibFDEventSource();
	bool makeAndAttachSource(GSourceFuncs *fdSourceFuncs,
		PollEventDelegate callback_, GIOCondition events, GMainContext *ctx);

protected:
	GSource2 *source{};
	gpointer tag{};
	int fd_ = -1;
	#ifndef NDEBUG
	const char *debugLabel{};
	#endif

	const char *label();
	void deinit();
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

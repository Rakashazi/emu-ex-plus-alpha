#pragma once

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
};

using FDEventSourceImpl = GlibFDEventSource;

class GlibEventLoop
{
public:
	constexpr GlibEventLoop() {}
	constexpr GlibEventLoop(GMainContext *ctx): mainContext{ctx} {}
	GMainContext *nativeObject() { return mainContext; }

protected:
	GMainContext *mainContext{};
};

using EventLoopImpl = GlibEventLoop;

}

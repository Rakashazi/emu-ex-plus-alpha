#pragma once

#include <glib.h>
#include <imagine/base/eventLoopDefs.hh>

namespace Base
{

static const int POLLEV_IN = G_IO_IN, POLLEV_OUT = G_IO_OUT, POLLEV_ERR = G_IO_ERR, POLLEV_HUP = G_IO_HUP;

struct GlibEventLoopFileSource
{
	PollEventDelegate callback;
	GSource *source = nullptr;
	gpointer tag = nullptr;
	int fd_ = -1;

	constexpr GlibEventLoopFileSource() {}
};

using EventLoopFileSourceImpl = GlibEventLoopFileSource;

}

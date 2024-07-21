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

#include <imagine/config/defs.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/io/PosixIO.hh>
#include <imagine/util/used.hh>
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
#include <array>

namespace IG
{

class Pipe
{
public:
	Pipe(int preferredSize = 0): Pipe(nullptr, preferredSize) {}
	Pipe(const char *debugLabel, int preferredSize = 0);
	PosixIO &source();
	PosixIO &sink();
	void attach(EventLoop loop);
	void detach();
	bool hasData();
	void dispatchSourceEvents();
	void setPreferredSize(int size);
	void setReadNonBlocking(bool on);
	bool isReadNonBlocking() const;
	explicit operator bool() const;
	const char* debugLabel() const { return fdSrc.debugLabel(); }

	void attach(auto &&f)
	{
		attach({}, IG_forward(f));
	}

	void attach(EventLoop loop, Callable<bool, PosixIO&> auto &&f)
	{
		fdSrc.setCallback(PollEventDelegate
			{
				[=](int fd, int)
				{
					PosixIO io{fd};
					bool keep = f(io);
					io.releaseFd().release();
					return keep;
				}
			});
		attach(loop);
	}

protected:
	std::array<PosixIO, 2> io;
	FDEventSource fdSrc;
};

}

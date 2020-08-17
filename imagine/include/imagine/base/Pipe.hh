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
#include <imagine/util/typeTraits.hh>
#include <array>
#include <optional>
#include <utility>

namespace Base
{

class Pipe
{
public:
	Pipe(uint32_t preferredSize = 0): Pipe(nullptr, preferredSize) {}
	Pipe(const char *debugLabel, uint32_t preferredSize = 0);
	Pipe(Pipe &&o);
	Pipe &operator=(Pipe &&o);
	~Pipe();
	PosixIO &source();
	PosixIO &sink();
	void attach(EventLoop loop, PollEventDelegate del);
	void detach();
	bool hasData();
	void dispatchSourceEvents();
	void setPreferredSize(int size);
	void setReadNonBlocking(bool on);
	bool isReadNonBlocking() const;
	explicit operator bool() const;

	template<class Func>
	void attach(Func func)
	{
		attach({}, std::forward<Func>(func));
	}

	template<class Func>
	void attach(EventLoop loop, Func func)
	{
		attach(loop,
			PollEventDelegate
			{
				[=](int fd, int)
				{
					PosixIO io{fd};
					auto keep = func(io);
					io.releaseFD();
					return keep;
				}
			});
	}

protected:
	[[no_unique_address]] IG::UseTypeIf<Config::DEBUG_BUILD, const char *> debugLabel{};
	std::array<PosixIO, 2> io{-1, -1};
	FDEventSource fdSrc{};

	void deinit();
	const char *label() const;
};

}

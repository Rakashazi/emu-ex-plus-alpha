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
#include <imagine/util/DelegateFunc.hh>
#include <imagine/base/EventLoop.hh>
#include <array>
#include <optional>

namespace Base
{

class Pipe
{
public:
	using Delegate = DelegateFunc<int (Pipe &pipe)>;

	Pipe();
	~Pipe();
	Pipe(Pipe &&o);
	Pipe &operator=(Pipe &&o);
	void addToEventLoop(EventLoop loop, Delegate del);
	void removeFromEventLoop();
	bool read(void *data, uint size);
	bool tryRead(void *data, uint size);
	bool write(const void *data, uint size);
	bool hasData();
	explicit operator bool() const;

	template <class T>
	std::optional<T> read()
	{
		T obj;
		bool success = read(&obj, sizeof(T));
		if(success)
			return {obj};
		else
			return {};
	}

	template <class T>
	T readNoErr()
	{
		T obj{};
		read(&obj, sizeof(T));
		return obj;
	}

	template <class T>
	bool write(T &&obj)
	{
		return write(&obj, sizeof(obj));
	}

private:
	std::array<int, 2> msgPipe{-1, -1};
	FDEventSource fdSrc{};
	Delegate del{};

	void moveObject(Pipe &o);
	void deinit();
};

}

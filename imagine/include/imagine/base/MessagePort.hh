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
#include <imagine/base/Pipe.hh>

namespace Base
{

template<class MSG_TYPE>
class PipeMessagePort
{
	static_assert(sizeof(MSG_TYPE) < PIPE_BUF, "size of message too big for atomic writes");

public:
	class Messages
	{
	public:
		Messages(Pipe &pipe): pipe{pipe} {}

		MSG_TYPE get()
		{
			auto msg = pipe.read<MSG_TYPE>();
			if(msg)
				return msg.value();
			else
				return {};
		}

		template <class T>
		T getExtraData()
		{
			return pipe.readNoErr<T>();
		}

		template <class T>
		bool getExtraData(T *obj, uint size)
		{
			return pipe.read(obj, size);
		}

	protected:
		Pipe &pipe;
	};

	PipeMessagePort(uint capacity = 8):
		pipe{(uint)sizeof(MSG_TYPE) * capacity}
	{
		pipe.setReadNonBlocking(true);
	}

	template<class FUNC>
	void addToEventLoop(EventLoop loop, FUNC del)
	{
		pipe.addToEventLoop(loop,
			[=](Base::Pipe &pipe) -> int
			{
				Messages msg{pipe};
				return del(msg);
			});
	}

	void removeFromEventLoop()
	{
		pipe.removeFromEventLoop();
	}

	bool send(MSG_TYPE &&msg)
	{
		return pipe.write(msg);
	}

	template <class T>
	bool sendExtraData(T &&obj)
	{
		static_assert(sizeof(T) < PIPE_BUF, "size of data too big for atomic writes");
		return pipe.write(obj);
	}

	template <class T>
	bool sendExtraData(T *obj, uint size)
	{
		assumeExpr(size < PIPE_BUF);
		return pipe.write(obj, size);
	}

	template<class FUNC>
	void run(FUNC del)
	{
		Messages msgs{pipe};
		for(auto msg = msgs.get(); del(msg); msg = msgs.get()) {}
	}

	explicit operator bool() const { return (bool)pipe; }

protected:
	Pipe pipe;
};

template<class MSG_TYPE>
using MessagePort = PipeMessagePort<MSG_TYPE>;

}

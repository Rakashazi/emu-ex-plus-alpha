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
#include <type_traits>

namespace Base
{

template<class MsgType>
class PipeMessagePort
{
	static_assert(sizeof(MsgType) < PIPE_BUF, "size of message too big for atomic writes");

public:
	class Messages
	{
	public:
		Messages(Pipe &pipe): pipe{pipe} {}

		MsgType get()
		{
			auto msg = pipe.read<MsgType>();
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
		bool getExtraData(T *obj, size_t size)
		{
			return pipe.read(obj, size);
		}

	protected:
		Pipe &pipe;
	};

	PipeMessagePort(const char *debugLabel = nullptr, uint32_t capacity = 8):
		pipe{debugLabel, (uint32_t)sizeof(MsgType) * capacity}
	{
		pipe.setReadNonBlocking(true);
	}

	template<class Func>
	void addToEventLoop(EventLoop loop, Func &&func)
	{
		pipe.addToEventLoop(loop,
			[=](Base::Pipe &pipe) -> int
			{
				Messages msg{pipe};
				constexpr auto returnsVoid = std::is_same_v<void, decltype(func(msg))>;
				if constexpr(returnsVoid)
				{
					func(msg);
					return 1;
				}
				else
				{
					return func(msg);
				}
			});
	}

	void removeFromEventLoop()
	{
		pipe.removeFromEventLoop();
	}

	bool send(MsgType msg)
	{
		return pipe.write(msg);
	}

	template <class T>
	bool sendWithExtraData(MsgType msg, T obj)
	{
		static_assert(sizeof(MsgType) + sizeof(T) < PIPE_BUF, "size of data too big for atomic writes");
		const auto bufferSize = sizeof(MsgType) + sizeof(T);
		char buffer[bufferSize];
		memcpy(buffer, &msg, sizeof(MsgType));
		memcpy(buffer + sizeof(MsgType), &obj, sizeof(T));
		return pipe.write(buffer, bufferSize);
	}

	template <class T>
	bool sendWithExtraData(MsgType msg, T *obj, uint32_t size)
	{
		assumeExpr(sizeof(MsgType) + size < PIPE_BUF);
		const auto bufferSize = sizeof(MsgType) + size;
		char buffer[bufferSize];
		memcpy(buffer, &msg, sizeof(MsgType));
		memcpy(buffer + sizeof(MsgType), obj, size);
		return pipe.write(buffer, bufferSize);
	}

	void clear()
	{
		Messages msg{pipe};
		while(msg.get()) {}
	}

	explicit operator bool() const { return (bool)pipe; }

protected:
	Pipe pipe;
};

template<class MsgType>
using MessagePort = PipeMessagePort<MsgType>;

}

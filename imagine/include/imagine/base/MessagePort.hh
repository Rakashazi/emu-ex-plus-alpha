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
#include <imagine/util/typeTraits.hh>

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
		bool getExtraData(T *obj, size_t size)
		{
			return pipe.read(obj, size);
		}

	protected:
		Pipe &pipe;
	};

	PipeMessagePort(const char *debugLabel = nullptr, uint32_t capacity = 8):
		pipe{debugLabel, (uint32_t)sizeof(MSG_TYPE) * capacity}
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
				constexpr auto returnsVoid = std::is_same<void, decltype(del(msg))>::value;
				if constexpr(returnsVoid)
				{
					del(msg);
					return 1;
				}
				else
				{
					return del(msg);
				}
			});
	}

	void removeFromEventLoop()
	{
		pipe.removeFromEventLoop();
	}

	bool send(MSG_TYPE msg)
	{
		return pipe.write(msg);
	}

	template <class T>
	bool sendWithExtraData(MSG_TYPE msg, T obj)
	{
		static_assert(sizeof(MSG_TYPE) + sizeof(T) < PIPE_BUF, "size of data too big for atomic writes");
		const auto bufferSize = sizeof(MSG_TYPE) + sizeof(T);
		char buffer[bufferSize];
		memcpy(buffer, &msg, sizeof(MSG_TYPE));
		memcpy(buffer + sizeof(MSG_TYPE), &obj, sizeof(T));
		return pipe.write(buffer, bufferSize);
	}

	template <class T>
	bool sendWithExtraData(MSG_TYPE msg, T *obj, uint32_t size)
	{
		assumeExpr(sizeof(MSG_TYPE) + size < PIPE_BUF);
		const auto bufferSize = sizeof(MSG_TYPE) + size;
		char buffer[bufferSize];
		memcpy(buffer, &msg, sizeof(MSG_TYPE));
		memcpy(buffer + sizeof(MSG_TYPE), obj, size);
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

template<class MSG_TYPE>
using MessagePort = PipeMessagePort<MSG_TYPE>;

}

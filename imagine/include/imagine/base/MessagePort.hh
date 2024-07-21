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
#include <imagine/thread/Semaphore.hh>
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
#include <cstring>
#include <span>

namespace IG
{

template <class MsgType>
concept ReplySemaphoreSettableMessage =
	requires (MsgType msg, std::binary_semaphore *sem){ msg.setReplySemaphore(sem); };

enum class MessageReplyMode
{
	none, wait
};

template<class MsgType>
class PipeMessages
{
public:
	struct Sentinel {};

	class Iterator
	{
	public:
		constexpr Iterator(PosixIO &io): io{&io}
		{
			this->operator++();
		}

		Iterator operator++()
		{
			if(!io) [[unlikely]]
				return *this;
			if(io->read(msg).bytes != sizeof(MsgType))
			{
				// end of messages
				io = nullptr;
			}
			return *this;
		}

		bool operator==(Sentinel) const
		{
			return !io;
		}

		const MsgType &operator*() const
		{
			return msg;
		}

	private:
		PosixIO *io{};
		MsgType msg;
	};

	constexpr PipeMessages(PosixIO &io): io{io} {}
	auto begin() const { return Iterator{io}; }
	auto end() const { return Sentinel{}; }

	template <class T>
	T getExtraData()
	{
		return io.get<T>();
	}

	template <class T>
	auto readExtraData(std::span<T> span)
	{
		return io.read(span);
	}

protected:
	PosixIO &io;
};

template<class MsgType>
class PipeMessagePort
{
public:
	using Messages = PipeMessages<MsgType>;
	static constexpr size_t MSG_SIZE = sizeof(MsgType);
	static_assert(MSG_SIZE < PIPE_BUF, "size of message too big for atomic writes");

	PipeMessagePort(const char *debugLabel = nullptr, int capacity = 0):
		pipe{debugLabel, (int)MSG_SIZE * capacity} {}

	void attach(auto &&f)
	{
		attach(EventLoop::forThread(), IG_forward(f));
	}

	void attach(EventLoop loop, Callable<void, Messages> auto &&f)
	{
		pipe.setReadNonBlocking(true);
		pipe.attach(loop,
			[=](auto &io) -> bool
			{
				Messages msg{io};
				f(msg);
				return true;
			});
	}

	void attach(EventLoop loop, Callable<bool, Messages> auto &&f)
	{
		pipe.setReadNonBlocking(true);
		pipe.attach(loop,
			[=](auto &io) -> bool
			{
				Messages msg{io};
				return f(msg);
			});
	}

	void detach()
	{
		pipe.detach();
	}

	bool send(MsgType msg)
	{
		return pipe.sink().put(msg) != -1;
	}

	bool send(MsgType msg, MessageReplyMode mode)
	{
		if(mode == MessageReplyMode::wait)
		{
			std::binary_semaphore replySemaphore{0};
			return send(msg, &replySemaphore);
		}
		else
		{
			return send(msg);
		}
	}

	bool send(ReplySemaphoreSettableMessage auto msg, std::binary_semaphore *semPtr)
	{
		if(semPtr)
		{
			msg.setReplySemaphore(semPtr);
			if(pipe.sink().put(msg) == -1) [[unlikely]]
			{
				return false;
			}
			semPtr->acquire();
			return true;
		}
		else
		{
			return send(msg);
		}
	}

	bool sendWithExtraData(MsgType msg, auto &&obj)
	{
		static_assert(MSG_SIZE + sizeof(obj) < PIPE_BUF, "size of data too big for atomic writes");
		return sendWithExtraData(msg, std::span<const std::remove_reference_t<decltype(obj)>>{&obj, 1});
	}

	template <class T>
	bool sendWithExtraData(MsgType msg, std::span<const T> span)
	{
		if(span.empty())
			return send(msg);
		assert(span.size_bytes() < PIPE_BUF);
		OutVector buffs[2]{std::span<const MsgType>{&msg, 1}, span};
		return pipe.sink().writeVector(buffs) != -1;
	}

	MsgType getMessage() { return pipe.source().get<MsgType>(); }

	void clear()
	{
		while(getMessage()) {}
	}

	void dispatchMessages()
	{
		pipe.dispatchSourceEvents();
	}

	Messages messages() { return Messages{pipe.source()}; }

	explicit operator bool() const { return (bool)pipe; }

protected:
	Pipe pipe;
};

template<class MsgType>
using MessagePort = PipeMessagePort<MsgType>;

template<class MsgType>
using Messages = PipeMessages<MsgType>;

}

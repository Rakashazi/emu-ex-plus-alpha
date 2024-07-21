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

#include <imagine/vmem/memory.hh>
#include <imagine/util/math.hh>
#include <imagine/util/algorithm.h>
#include <cstdint>
#include <cassert>
#include <atomic>
#include <span>
#include <optional>
#include <type_traits>
#include <bit>
#include <array>

namespace IG
{

struct RingBufferConf
{
	bool mirrored{};
	size_t fixedSize{};
};

struct RingBufferRWFlags
{
	bool blocking:1{};
	size_t flushSize{};
};

struct RingBufferIdxPair
{
	size_t read{}, write{};
};

template<class T, RingBufferConf conf = RingBufferConf{}>
class RingBuffer
{
public:
	using IdxPair = RingBufferIdxPair;
	using RWFlags = RingBufferRWFlags;
	static constexpr bool isFixedSize = conf.fixedSize;

	struct RWSpan : public std::span<T>
	{
		constexpr RWSpan(std::span<T> span, IdxPair idxs):
			std::span<T>{span}, idxs{idxs} {}

		IdxPair idxs{};
	};

	constexpr RingBuffer() = default;

	RingBuffer(size_t size) requires(!isFixedSize):
		buff{allocBuffer(size)} {}

	void clear()
	{
		writeIdx.store(0, std::memory_order_relaxed);
		readIdx.store(0, std::memory_order_relaxed);
	}

	[[nodiscard]]
	size_t freeSpace(IdxPair idxs) const { return capacity() - size(idxs);}

	[[nodiscard]]
	size_t freeSpace() const { return capacity() - size();}

	[[nodiscard]]
	size_t size(IdxPair idxs) const
	{
		return wrapSize(idxs.write - idxs.read);
	}

	[[nodiscard]]
	size_t size() const { return size(loadIdxs()); }

	[[nodiscard]]
	size_t capacity() const { return conf.mirrored ? bufferCapacity() / 2 : bufferCapacity(); }

	[[nodiscard]]
	bool empty(IdxPair idxs) const { return !size(idxs); }

	[[nodiscard]]
	bool empty() const { return !size(); }

	[[nodiscard]]
	bool full(IdxPair idxs) const { return size(idxs) == capacity(); }

	[[nodiscard]]
	bool full() const { return size() == capacity(); }

	[[nodiscard]]
	RWSpan beginRead(size_t s, RWFlags flags = {})
	{
		IdxPair idxs{.read = readIdx.load(std::memory_order_relaxed), .write = writeIdx.load(std::memory_order_acquire)};
		if(flags.blocking && empty(idxs))
		{
			writeIdx.wait(idxs.write, std::memory_order_acquire);
			idxs.write = writeIdx.load(std::memory_order_acquire);
		}
		s = std::min(s, size(idxs));
		std::span<T> span{&buff[wrapIdx(idxs.read)], s};
		assertAddrRange(span);
		return {span, idxs};
	}

	void endRead(RWSpan span)
	{
		readIdx.store(span.idxs.read + span.size(), std::memory_order_release);
	}

	size_t read(std::span<T> buff, RWFlags flags = {})
	{
		auto span = beginRead(buff.size(), flags);
		copy_n(span.data(), span.size(), buff.data());
		endRead(span);
		return span.size();
	}

	[[nodiscard]]
	std::optional<T> tryPop()
	{
		T tmp;
		if(!read({&tmp, 1}))
			return {};
		return std::optional<T>{tmp};
	}

	T pop(RWFlags flags = {})
	{
		T tmp;
		if(read({&tmp, 1}, flags))
			return tmp;
		return {};
	}

	void notifyRead()
	{
		readIdx.notify_all();
	}

	[[nodiscard]]
	RWSpan beginWrite(size_t s, RWFlags flags = {})
	{
		IdxPair idxs{.read = readIdx.load(std::memory_order_acquire), .write = writeIdx.load(std::memory_order_relaxed)};
		if(flags.blocking && !freeSpace(idxs))
		{
			notifyWrite();
			readIdx.wait(idxs.read, std::memory_order_acquire);
			idxs.read = readIdx.load(std::memory_order_acquire);
		}
		s = std::min(s, freeSpace(idxs));
		std::span<T> span{&buff[wrapIdx(idxs.write)], s};
		assertAddrRange(span);
		return {span, idxs};
	}

	void endWrite(RWSpan span)
	{
		writeIdx.store(span.idxs.write + span.size(), std::memory_order_release);
	}

	size_t write(std::span<const T> buff, RWFlags flags = {})
	{
		auto span = beginWrite(buff.size(), flags);
		copy_n(buff.data(), span.size(), span.data());
		endWrite(span);
		if(flags.flushSize && (size() + span.size() >= flags.flushSize))
			notifyWrite();
		return span.size();
	}

	bool push(const T& val, RWFlags flags = {})
	{
		return write({&val, 1}, flags);
	}

	void notifyWrite()
	{
		writeIdx.notify_all();
	}

	size_t setMinCapacity(size_t newCapacity) requires(!isFixedSize)
	{
		newCapacity = roundUpPowOf2(newCapacity);
		if(newCapacity == capacity())
			return newCapacity;
		size_t oldSize = size();
		auto newBuff = allocBuffer(newCapacity);
		if(oldSize && newCapacity > oldSize) // maintain buffer content if increasing size
		{
			copy_n(buff.get(), oldSize, newBuff.get());
		}
		else // clear buffer if truncating content
		{
			clear();
		}
		buff = std::move(newBuff);
		return newCapacity;
	}

	void reset() requires(!isFixedSize)
	{
		resetVPtr(buff);
		clear();
	}

	void waitForSize(size_t s)
	{
		if(size() == s)
			return;
		notifyWrite();
		IdxPair idxs = loadIdxs();
		while(size(idxs) != s)
		{
			readIdx.wait(idxs.read, std::memory_order_acquire);
			idxs = loadIdxs();
		}
	}

private:
	static constexpr size_t idxAlign = 64; // default of std::hardware_destructive_interference_size
	static_assert(conf.fixedSize == 0 || std::has_single_bit(conf.fixedSize));
	static_assert(conf.fixedSize == 0 || (conf.fixedSize > 0 && !conf.mirrored));
	std::conditional_t<isFixedSize, std::array<T, conf.fixedSize>, UniqueVPtr<T>> buff;
	alignas(idxAlign) std::atomic_size_t writeIdx{};
	alignas(idxAlign) std::atomic_size_t readIdx{};

	static UniqueVPtr<T> allocBuffer(size_t size) requires(!isFixedSize)
	{
		size = roundUpPowOf2(size);
		return conf.mirrored ? makeUniqueMirroredVPtr<T>(size) : makeUniqueVPtr<T>(size);
	}

	size_t wrapSize(size_t s) const
	{
		return s & (capacity() | (capacity() - 1));
	}

	size_t wrapIdx(size_t s) const
	{
		return s & (capacity() - 1);
	}

	IdxPair loadIdxs() const
	{
		return {.read = readIdx.load(std::memory_order_acquire), .write = writeIdx.load(std::memory_order_acquire)};
	}

	size_t bufferCapacity() const requires(!isFixedSize)  { return buff.get_deleter().size; }
	size_t bufferCapacity() const requires isFixedSize { return buff.size(); }

	void assertAddrRange([[maybe_unused]] std::span<T> span) const
	{
		if(!conf.mirrored)
		{
			// check that a multi-element read/write doesn't go past the buffer
			assert(&span[span.size()] <= &buff[bufferCapacity()]);
		}
	}
};

}

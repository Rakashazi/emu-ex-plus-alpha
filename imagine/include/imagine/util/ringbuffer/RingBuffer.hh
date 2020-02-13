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

#include <cstdint>
#include <atomic>

namespace IG
{

class RingBuffer
{
public:
	using SizeType = uint32_t;

	constexpr RingBuffer() {}
	RingBuffer(SizeType size);
	RingBuffer(RingBuffer &&o);
	RingBuffer &operator=(RingBuffer &&o);
	~RingBuffer();
	void clear();
	SizeType freeSpace() const;
	SizeType size() const;
	SizeType capacity() const;
	void setMinCapacity(SizeType capacity);
	SizeType write(const void *buff, SizeType size);
	char *writeAddr() const;
	void commitWrite(SizeType size);
	SizeType read(void *buff, SizeType size);
	char *readAddr() const;
	void commitRead(SizeType size);
	explicit operator bool() const;

protected:
	char *buff{};
	char *start{}, *end{};
	std::atomic<SizeType> written{};
	SizeType buffSize{};

	void init(SizeType size);
	void deinit();
	char *advanceAddr(char *ptr, SizeType size) const;
	char *wrapPtr(char *ptr) const;
};

}

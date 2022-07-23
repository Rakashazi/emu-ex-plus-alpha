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

#include <imagine/io/ioDefs.hh>
#include <imagine/util/memory/Buffer.hh>
#include <imagine/util/concepts.hh>
#include <memory>
#include <utility>

namespace IG
{

class IOBuffer : public ByteBuffer
{
public:
	using ByteBuffer::ByteBuffer;
	using Flags = uint8_t;

	static constexpr Flags MAPPED_FILE_BIT = bit(0);

	constexpr IOBuffer(std::span<uint8_t> span, Flags flags, DeleterFunc deleter = [](const uint8_t*, size_t){}):
		ByteBuffer{span, deleter}, flags{flags} {}

	constexpr bool isMappedFile() const { return flags & MAPPED_FILE_BIT; }

protected:
	Flags flags{};
};

template <class IO>
class IOUtils
{
public:
	using AccessHint = IOAccessHint;
	using Advice = IOAdvice;
	using BufferMode = IOBufferMode;
	using SeekMode = IOSeekMode;
	using OpenFlags = FileOpenFlags;

	off_t seekS(off_t offset);
	off_t seekE(off_t offset);
	off_t seekC(off_t offset);
	bool rewind();
	off_t tell();

	ssize_t send(auto &output, off_t *srcOffset, size_t bytes)
	{
		if(srcOffset)
		{
			seekS(*srcOffset);
		}
		ssize_t bytesToWrite = bytes;
		ssize_t totalBytesWritten = 0;
		while(bytesToWrite)
		{
			std::array<char, 4096> buff;
			ssize_t bytes = std::min((ssize_t)sizeof(buff), bytesToWrite);
			ssize_t bytesRead = static_cast<IO*>(this)->read(buff.data(), bytes);
			if(bytesRead == 0)
				break;
			if(bytesRead == -1)
			{
				return -1;
			}
			ssize_t bytesWritten = output.write(buff.data(), bytes);
			if(bytesWritten == -1)
			{
				return -1;
			}
			totalBytesWritten += bytesWritten;
			bytesToWrite -= bytes;
		}
		return totalBytesWritten;
	}

	IOBuffer buffer(BufferMode mode = BufferMode::DIRECT);

	template <class T, bool useOffset = false>
	T getImpl(off_t offset = 0)
	{
		if constexpr(std::is_same_v<T, bool>)
		{
			// special case to convert value to a valid bool
			return getImpl<uint8_t, useOffset>(offset);
		}
		else
		{
			T obj;
			ssize_t size;
			if constexpr(useOffset)
				size = static_cast<IO*>(this)->readAtPos(&obj, sizeof(T), offset);
			else
				size = static_cast<IO*>(this)->read(&obj, sizeof(T));
			if(size < (ssize_t)sizeof(T)) [[unlikely]]
				return {};
			return obj;
		}
	}

	template <class T>
	T get()
	{
		return getImpl<T>();
	}

	template <class T>
	T get(off_t offset)
	{
		return getImpl<T, true>(offset);
	}

	ssize_t readAtPosGeneric(void *buff, size_t bytes, off_t offset);

	ssize_t readSized(ResizableContainer auto &c, size_t maxBytes)
	{
		if(c.max_size() < maxBytes)
			return -1;
		c.resize(maxBytes);
		auto bytesRead = static_cast<IO*>(this)->read(c.data(), maxBytes);
		if(bytesRead == -1) [[unlikely]]
			return -1;
		c.resize(bytesRead);
		return bytesRead;
	}

	ssize_t write(NotPointerDecayable auto &&obj)
	{
		return static_cast<IO*>(this)->write(&obj, sizeof(decltype(obj)));
	}

	FILE *toFileStream(const char *opentype);
};

}

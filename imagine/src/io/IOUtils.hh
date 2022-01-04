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

#include <imagine/io/IO.hh>
#include <array>

namespace IG
{

template <class IO>
off_t IOUtils<IO>::seekS(off_t offset)
{
	return static_cast<IO*>(this)->seek(offset, IODefs::SeekMode::SET);
}

template <class IO>
off_t IOUtils<IO>::seekE(off_t offset)
{
	return static_cast<IO*>(this)->seek(offset, IODefs::SeekMode::END);
}

template <class IO>
off_t IOUtils<IO>::seekC(off_t offset)
{
	return static_cast<IO*>(this)->seek(offset, IODefs::SeekMode::CUR);
}

template <class IO>
bool IOUtils<IO>::rewind()
{
	return seekS(0) != -1;
}

template <class IO>
off_t IOUtils<IO>::tell()
{
	return static_cast<IO*>(this)->seekC(0);
}

template <class IO>
ssize_t IOUtils<IO>::send(IO &output, off_t *srcOffset, size_t bytes)
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

static IG::ByteBuffer makeBufferCopy(auto &io)
{
	auto size = io.size();
	auto buff = std::make_unique<uint8_t[]>(size);
	if(io.read(buff.get(), size) != (ssize_t)size)
	{
		return {};
	}
	return {std::move(buff), size};
}

template <class IO>
IG::ByteBuffer IOUtils<IO>::buffer(IODefs::BufferMode mode)
{
	auto &io = *static_cast<IO*>(this);
	if(mode == ::IG::IO::BufferMode::RELEASE)
	{
		if constexpr(requires {io.releaseBuffer();})
		{
			auto buff = io.releaseBuffer();
			if(buff)
				return buff;
		}
	}
	else // mode == IO::BufferMode::DIRECT
	{
		auto map = io.map();
		if(map.data())
			return {map};
	}
	return makeBufferCopy(io);
}

}

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

#include <imagine/io/IOUtils.hh>

namespace IG
{

template <class IO>
off_t IOUtils<IO>::seek(off_t offset)
{
	return static_cast<IO*>(this)->seek(offset, IOSeekMode::Set);
}

template <class IO>
bool IOUtils<IO>::rewind()
{
	return seek(0) != -1;
}

template <class IO>
off_t IOUtils<IO>::tell()
{
	return static_cast<IO*>(this)->seek(0, IOSeekMode::Cur);
}

static IOBuffer makeBufferCopy(auto &io)
{
	auto size = io.size();
	auto buff = std::make_unique<uint8_t[]>(size);
	if(io.read(buff.get(), size, 0) != (ssize_t)size)
	{
		return {};
	}
	return {std::move(buff), size};
}

template <class IO>
IOBuffer IOUtils<IO>::buffer(IOBufferMode mode)
{
	auto &io = *static_cast<IO*>(this);
	if(mode == IOBufferMode::Release)
	{
		if constexpr(requires {io.releaseBuffer();})
		{
			auto buff = io.releaseBuffer();
			if(buff)
				return buff;
		}
	}
	else // mode == IOBufferMode::DIRECT
	{
		if constexpr(requires {io.map();})
		{
			auto map = io.map();
			if(map.data())
				return {map, {.mappedFile = true}};
		}
	}
	return makeBufferCopy(io);
}

template <class IO>
ssize_t IOUtils<IO>::readAtPosGeneric(void *buff, size_t bytes, off_t offset)
{
	auto &io = *static_cast<IO*>(this);
	auto savedOffset = io.tell();
	io.seek(offset);
	auto bytesRead = io.read(buff, bytes);
	io.seek(savedOffset);
	return bytesRead;
}

template <class IO>
ssize_t IOUtils<IO>::writeAtPosGeneric(const void *buff, size_t bytes, off_t offset)
{
	auto &io = *static_cast<IO*>(this);
	auto savedOffset = io.tell();
	io.seek(offset);
	auto bytesWritten = io.write(buff, bytes);
	io.seek(savedOffset);
	return bytesWritten;
}

template <class IO>
FILE *IOUtils<IO>::toFileStream([[maybe_unused]] const char* opentype)
{
	auto &io = *static_cast<IO*>(this);
	if(!io)
	{
		return nullptr;
	}
	auto ioPtr = std::make_unique<IO>(std::move(io));
	#if defined __ANDROID__ || __APPLE__
	auto f = funopen(ioPtr.release(),
		[](void *cookie, char *buf, int size)
		{
			auto &io = *(IO*)cookie;
			return int(io.read(buf, size));
		},
		[](void *cookie, const char *buf, int size)
		{
			auto &io = *(IO*)cookie;
			return int(io.write(buf, size));
		},
		[](void *cookie, fpos_t offset, int whence)
		{
			auto &io = *(IO*)cookie;
			return fpos_t(io.seek(offset, (IOSeekMode)whence));
		},
		[](void *cookie)
		{
			delete (IO*)cookie;
			return 0;
		});
	#else
	cookie_io_functions_t funcs
	{
		.read =
			[](void *cookie, char *buf, size_t size)
			{
				auto &io = *(IO*)cookie;
				return io.read(buf, size);
			},
		.write =
			[](void *cookie, const char *buf, size_t size)
			{
				auto &io = *(IO*)cookie;
				auto bytesWritten = io.write(buf, size);
				if(bytesWritten == -1)
				{
					bytesWritten = 0; // needs to return 0 for error
				}
				return bytesWritten;
			},
		.seek =
			[](void *cookie, off64_t *position, int whence)
			{
				auto &io = *(IO*)cookie;
				auto newPos = io.seek(*position, (IOSeekMode)whence);
				if(newPos == -1)
				{
					return -1;
				}
				*position = newPos;
				return 0;
			},
		.close =
			[](void *cookie)
			{
				delete (IO*)cookie;
				return 0;
			}
	};
	auto f = fopencookie(ioPtr.release(), opentype, funcs);
	#endif
	assert(f);
	return f;
}

template <class IO>
ssize_t IOUtils<IO>::genericWriteVector(std::span<const OutVector> buffs, std::optional<off_t> offset)
{
	auto &io = *static_cast<IO*>(this);
	ssize_t totalSize{};
	for(auto buff : buffs)
	{
		auto written = io.write(buff.data(), buff.size(), offset);
		if(written == -1)
			return -1;
		totalSize += written;
		if(offset)
			*offset += written;
	}
	return totalSize;
}

inline auto transformOffsetToAbsolute(IOSeekMode mode, auto offset, auto startPos, auto endPos, auto currentPos)
{
	switch(mode)
	{
		case IOSeekMode::Set: return offset + startPos;
		case IOSeekMode::End: return offset + endPos;
		case IOSeekMode::Cur: return offset + currentPos;
	}
	bug_unreachable("IOSeekMode == %d", (int)mode);
}

}

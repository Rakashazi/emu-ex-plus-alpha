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
#include <imagine/util/memory/UniqueFileStream.hh>

namespace IG
{

template <class IO>
class FileStream
{
public:
	constexpr FileStream() = default;

	FileStream(IO io, const char *opentype):
		io{std::move(io)}
	{
		#if defined __ANDROID__ || __APPLE__
		f = UniqueFileStream{funopen(this,
			[](void *cookie, char *buf, int size)
			{
				auto &s = *(FileStream*)cookie;
				return (int)s.io.read(buf, size);
			},
			[](void *cookie, const char *buf, int size)
			{
				auto &s = *(FileStream*)cookie;
				return (int)s.io.write(buf, size);
			},
			[](void *cookie, fpos_t offset, int whence)
			{
				auto &s = *(FileStream*)cookie;
				return (fpos_t)s.io.seek(offset, (IOSeekMode)whence);
			},
			nullptr)};
		#else
		cookie_io_functions_t funcs
		{
			.read =
				[](void *cookie, char *buf, size_t size)
				{
					auto &s = *(FileStream*)cookie;
					return (ssize_t)s.io.read(buf, size);
				},
			.write =
				[](void *cookie, const char *buf, size_t size)
				{
					auto &s = *(FileStream*)cookie;
					auto bytesWritten = s.io.write(buf, size);
					if(bytesWritten == -1)
					{
						bytesWritten = 0; // needs to return 0 for error
					}
					return (ssize_t)bytesWritten;
				},
			.seek =
				[](void *cookie, off64_t *position, int whence)
				{
					auto &s = *(FileStream*)cookie;
					auto newPos = s.io.seek(*position, (IOSeekMode)whence);
					if(newPos == -1)
					{
						return -1;
					}
					*position = newPos;
					return 0;
				}
		};
		f = UniqueFileStream{fopencookie(this, opentype, funcs)};
		#endif
		assert(f);
	}

	FILE *filePtr() const
	{
		return f.get();
	}

	operator IO&()
	{
		return io;
	}

	explicit operator bool() const
	{
		return (bool)f;
	}

	IO release()
	{
		return std::move(io);
	}

protected:
	UniqueFileStream f{};
	IO io{};
};

}

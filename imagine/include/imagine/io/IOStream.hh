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
#include <cstdio>
#include <cerrno>

template <class IO>
class IOStream
{
public:
	constexpr IOStream() {}

	IOStream(IO io, const char *opentype)
	{
		this->io = std::move(io);
		#if defined __ANDROID__ || __APPLE__
		f = funopen(this,
			[](void *cookie, char *buf, int size)
			{
				auto &s = *(IOStream*)cookie;
				return (int)s.io.read(buf, size);
			},
			[](void *cookie, const char *buf, int size)
			{
				auto &s = *(IOStream*)cookie;
				return (int)s.io.write(buf, size);
			},
			[](void *cookie, fpos_t offset, int whence)
			{
				auto &s = *(IOStream*)cookie;
				if(s.io.seek(offset, (IODefs::SeekMode)whence) != OK)
				{
					return (fpos_t)-1;
				}
				return (fpos_t)s.io.tell();
			},
			[](void *cookie)
			{
				auto &s = *(IOStream*)cookie;
				s.io.close();
				s.f = nullptr;
				return 0;
			});
		#else
		cookie_io_functions_t funcs{};
		funcs.read =
			[](void *cookie, char *buf, size_t size)
			{
				auto &s = *(IOStream*)cookie;
				return (ssize_t)s.io.read(buf, size);
			};
		funcs.write =
			[](void *cookie, const char *buf, size_t size)
			{
				auto &s = *(IOStream*)cookie;
				auto bytesWritten = s.io.write(buf, size);
				if(bytesWritten == -1)
				{
					bytesWritten = 0; // needs to return 0 for error
				}
				return (ssize_t)bytesWritten;
			};
		funcs.seek =
			[](void *cookie, off64_t *position, int whence)
			{
				auto &s = *(IOStream*)cookie;
				if(s.io.seek(*position, (IODefs::SeekMode)whence) != OK)
				{
					return -1;
				}
				*position = s.io.tell();
				return 0;
			};
		funcs.close =
			[](void *cookie)
			{
				auto &s = *(IOStream*)cookie;
				s.io.close();
				s.f = nullptr;
				return 0;
			};
		f = fopencookie(this, opentype, funcs);
		#endif
		assert(f);
	}

	~IOStream()
	{
		if(f)
		{
			fclose(f);
		}
	}

	IOStream(IOStream &&o)
	{
		io = std::move(o.io);
		f = o.f;
		o.f = nullptr;
	}

	IOStream &operator=(IOStream &&o)
	{
		if(f)
		{
			fclose(f);
		}
		io = std::move(o.io);
		f = o.f;
		o.f = nullptr;
		return *this;
	}

	operator FILE*()
	{
		return f;
	}

	explicit operator bool()
	{
		return f;
	}

protected:
	IO io;
	FILE *f{};
};

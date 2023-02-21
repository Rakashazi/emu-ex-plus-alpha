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
#include <cstdio>

// Functions to wrap basic stdio functionality

namespace IG
{

inline int fgetc(Readable auto &io)
{
	char byte;
	return io.read(&byte, 1) == 1 ? byte : EOF;
}

inline size_t fread(void *ptr, size_t size, size_t nmemb, Readable auto &io)
{
	auto bytesRead = io.read(ptr, (size * nmemb));
	return bytesRead > 0 ? bytesRead / size : 0;
}

inline size_t fwrite(const void* ptr, size_t size, size_t nmemb, Writable auto &io)
{
	auto bytesWritten = io.write(ptr, (size * nmemb));
	return bytesWritten > 0 ? bytesWritten / size : 0;
}

inline long ftell(Seekable auto &io)
{
	return io.tell();
}

inline int fseek(Seekable auto &io, long offset, int whence)
{
	return io.seek(offset, IOSeekMode(whence)) == -1 ? -1 : 0;
}

inline int fclose(auto &stream)
{
	return stream = {};
}

}

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

template <class IO>
ssize_t IOUtils<IO>::read(void *buff, size_t bytes) { return ((IO*)this)->read(buff, bytes, nullptr); }

template <class IO>
ssize_t IOUtils<IO>::readAtPos(void *buff, size_t bytes, off_t offset) { return ((IO*)this)->readAtPos(buff, bytes, offset, nullptr); }

template <class IO>
ssize_t IOUtils<IO>::write(const void *buff, size_t bytes) { return ((IO*)this)->write(buff, bytes, nullptr); }

template <class IO>
off_t IOUtils<IO>::seek(off_t offset, IODefs::SeekMode mode) { return ((IO*)this)->seek(offset, mode, nullptr); }

template <class IO>
off_t IOUtils<IO>::seekS(off_t offset, std::error_code *ecOut) { return ((IO*)this)->seek(offset, SEEK_SET, ecOut); }

template <class IO>
off_t IOUtils<IO>::seekE(off_t offset, std::error_code *ecOut) { return ((IO*)this)->seek(offset, SEEK_END, ecOut); }

template <class IO>
off_t IOUtils<IO>::seekC(off_t offset, std::error_code *ecOut) { return ((IO*)this)->seek(offset, SEEK_CUR, ecOut); }

template <class IO>
off_t IOUtils<IO>::seekS(off_t offset) { return ((IO*)this)->seek(offset, SEEK_SET); }

template <class IO>
off_t IOUtils<IO>::seekE(off_t offset) { return ((IO*)this)->seek(offset, SEEK_END); }

template <class IO>
off_t IOUtils<IO>::seekC(off_t offset) { return ((IO*)this)->seek(offset, SEEK_CUR); }

template <class IO>
off_t IOUtils<IO>::tell(std::error_code *ecOut)
{
	return ((IO*)this)->seekC(0, ecOut);
}

template <class IO>
off_t IOUtils<IO>::tell() { return ((IO*)this)->tell(nullptr); }

template <class IO>
std::error_code IOUtils<IO>::readAll(void *buff, size_t bytes)
{
	std::error_code ec{};
	auto bytesRead = ((IO*)this)->read(buff, bytes, &ec);
	if(bytesRead != (ssize_t)bytes)
		return !ec ? std::error_code{EINVAL, std::system_category()} : ec;
	return {};
}

template <class IO>
std::error_code IOUtils<IO>::writeAll(void *buff, size_t bytes)
{
	std::error_code ec{};
	auto bytesWritten = ((IO*)this)->write(buff, bytes, &ec);
	if(bytesWritten != (ssize_t)bytes)
		return !ec ? std::error_code{EINVAL, std::system_category()} : ec;
	return {};
}

template <class IO>
std::error_code IOUtils<IO>::writeToIO(IO &io)
{
	seekS(0);
	ssize_t bytesToWrite = ((IO*)this)->size();
	while(bytesToWrite)
	{
		char buff[4096];
		ssize_t bytes = std::min((ssize_t)sizeof(buff), bytesToWrite);
		auto ec = readAll(buff, bytes);
		if(ec)
		{
			return ec;
		}
		if(io.write(buff, bytes) != bytes)
		{
			return {EINVAL, std::system_category()};
		}
		bytesToWrite -= bytes;
	}
	return {};
}

template <class IO>
IG::ConstBufferView IOUtils<IO>::constBufferView()
{
	auto size = ((IO*)this)->size();
	auto mmapData = ((IO*)this)->mmapConst();
	if(mmapData)
	{
		return IG::ConstBufferView(mmapData, size, [](const char*){});
	}
	else
	{
		seekS(0);
		auto buff = new char[size];
		if(((IO*)this)->read(buff, size) != (ssize_t)size)
		{
			return {};
		}
		return IG::ConstBufferView(buff, size, [](const char *ptr){ delete[] ptr; });
	}
}

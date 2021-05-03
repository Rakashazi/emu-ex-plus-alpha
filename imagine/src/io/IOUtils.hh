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

template <class IO>
ssize_t IOUtils<IO>::read(void *buff, size_t bytes)
{
	return static_cast<IO*>(this)->read(buff, bytes, nullptr);
}

template <class IO>
ssize_t IOUtils<IO>::readAtPos(void *buff, size_t bytes, off_t offset)
{
	return static_cast<IO*>(this)->readAtPos(buff, bytes, offset, nullptr);
}

template <class IO>
ssize_t IOUtils<IO>::write(const void *buff, size_t bytes)
{
	return static_cast<IO*>(this)->write(buff, bytes, nullptr);
}

template <class IO>
off_t IOUtils<IO>::seek(off_t offset, IODefs::SeekMode mode)
{
	return static_cast<IO*>(this)->seek(offset, mode, nullptr);
}

template <class IO>
off_t IOUtils<IO>::seekS(off_t offset, std::error_code *ecOut)
{
	return static_cast<IO*>(this)->seek(offset, SEEK_SET, ecOut);
}

template <class IO>
off_t IOUtils<IO>::seekE(off_t offset, std::error_code *ecOut)
{
	return static_cast<IO*>(this)->seek(offset, SEEK_END, ecOut);
}

template <class IO>
off_t IOUtils<IO>::seekC(off_t offset, std::error_code *ecOut)
{
	return static_cast<IO*>(this)->seek(offset, SEEK_CUR, ecOut);
}

template <class IO>
bool IOUtils<IO>::rewind()
{
	return seekS(0) != -1;
}

template <class IO>
off_t IOUtils<IO>::tell(std::error_code *ecOut)
{
	return static_cast<IO*>(this)->seekC(0, ecOut);
}

template <class IO>
ssize_t IOUtils<IO>::send(IO &output, off_t *srcOffset, size_t bytes, std::error_code *ecOut)
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
			if(ecOut)
				*ecOut = {EIO, std::system_category()};
			return -1;
		}
		ssize_t bytesWritten = output.write(buff.data(), bytes);
		if(bytesWritten == -1)
		{
			if(ecOut)
				*ecOut = {EIO, std::system_category()};
			return -1;
		}
		totalBytesWritten += bytesWritten;
		bytesToWrite -= bytes;
	}
	return totalBytesWritten;
}

template <class IO>
IG::ConstByteBufferView IOUtils<IO>::constBufferView()
{
	auto size = static_cast<IO*>(this)->size();
	auto mmapData = static_cast<IO*>(this)->mmapConst();
	if(mmapData)
	{
		return IG::ConstByteBufferView{mmapData, size, [](const uint8_t*){}};
	}
	else
	{
		seekS(0);
		auto buff = new uint8_t[size];
		if(static_cast<IO*>(this)->read(buff, size) != (ssize_t)size)
		{
			delete[] buff;
			return {};
		}
		return IG::ConstByteBufferView{buff, size, [](const uint8_t *ptr){ delete[] ptr; }};
	}
}

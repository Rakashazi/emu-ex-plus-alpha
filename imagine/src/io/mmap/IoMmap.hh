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

#pragma once

#include <engine-globals.h>

#include <io/Io.hh>

class IoMmap : public Io
{
public:
	void init(const uchar * buffer, size_t size);

	size_t readUpTo(void* buffer, size_t numBytes);
	const uchar *mmapConst() { return data; };
	size_t fwrite(const void* ptr, size_t size, size_t nmemb);
	int fgetc();
	CallResult tell(ulong* offset);
	CallResult seekU(ulong offset, uint mode);
	CallResult seekS(long offset, uint mode);
	void truncate(ulong offset) { logErr("truncate not supported"); }
	ulong size() ATTRS(pure);
	void sync() { }
	int eof() ATTRS(pure);
protected:
	const uchar *data = nullptr;
	const uchar *currPos = nullptr;
	size_t iSize = 0;

	const uchar *endofBuffer();
};

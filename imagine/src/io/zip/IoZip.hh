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

#include <mem/interface.h>
#include <io/Io.hh>
#include <unzip.h>

class IoZip : public Io
{
public:
	static Io *open(const char *path, const char *pathInZip);
	~IoZip() { close(); }

	size_t readUpTo(void* buffer, size_t numBytes);
	size_t fwrite(const void* ptr, size_t size, size_t nmemb);
	CallResult tell(ulong* offset);
	CallResult seekU(ulong offset, uint mode);
	CallResult seekS(long offset, uint mode);
	void truncate(ulong offset);
	void close();
	ulong size();
	void sync();
	int eof();

private:
	unzFile zip = nullptr;
	Io *zipIo = nullptr;
	ulong uncompSize = 0;
	bool openZipFile(const char *path);
	bool openFileInZip();
	void resetFileInZip();
};

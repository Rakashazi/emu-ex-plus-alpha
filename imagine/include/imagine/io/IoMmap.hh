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

#include <imagine/engine-globals.h>
#include <imagine/io/Io.hh>

class IoMmap : public Io
{
public:
	void init(const void* buffer, size_t size);
	ssize_t readUpTo(void *buffer, size_t numBytes) override;
	const char *mmapConst() override { return data; };
	size_t fwrite(const void *buffer, size_t size, size_t nmemb) override;
	CallResult tell(ulong &offset) override;
	CallResult seek(long offset, uint mode) override;
	void truncate(ulong offset) override;
	ulong size() override;
	void sync() override {}
	int eof() override;

protected:
	const char *data = nullptr;
	const char *currPos = nullptr;
	size_t iSize = 0;

	const char *endofBuffer();
};

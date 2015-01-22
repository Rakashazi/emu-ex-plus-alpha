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
#include <imagine/io/IO.hh>

class MapIO : public IO
{
public:
	using IOUtils::read;
	using IOUtils::readAtPos;
	using IOUtils::write;
	using IOUtils::tell;

	constexpr MapIO() {}
	ssize_t read(void *buff, size_t bytes, CallResult *resultOut) override;
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut) override;
	const char *mmapConst() override;
	ssize_t write(const void *buff, size_t bytes, CallResult *resultOut) override;
	off_t tell(CallResult *resultOut) override;
	CallResult seek(off_t offset, SeekMode mode) override;
	size_t size() override;
	bool eof() override;
	explicit operator bool() override;
	#if defined __linux__ || defined __APPLE__
	void advise(off_t offset, size_t bytes, Advice advice) override;
	#endif

protected:
	const char *data{};
	const char *currPos{};
	size_t dataSize = 0;

	void setData(const void* buff, size_t size);
	void resetData();
	const char *dataEnd();
	ssize_t readAtAddr(void* buff, size_t bytes, const char *readPos, CallResult *resultOut);
};

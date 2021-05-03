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

#include <imagine/config/defs.hh>
#include <imagine/io/IO.hh>

class MapIO : public IO
{
public:
	using IO::read;
	using IO::readAtPos;
	using IO::write;
	using IO::seek;
	using IO::seekS;
	using IO::seekE;
	using IO::seekC;
	using IO::tell;
	using IO::send;
	using IO::constBufferView;
	using IO::get;

	constexpr MapIO() {}
	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut) override;
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut) override;
	const uint8_t *mmapConst() override;
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut) override;
	off_t seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut) override;
	size_t size() override;
	bool eof() override;
	explicit operator bool() const override;
	#if defined __linux__ || defined __APPLE__
	void advise(off_t offset, size_t bytes, Advice advice) override;
	#endif

protected:
	const uint8_t *data{};
	const uint8_t *currPos{};
	size_t dataSize = 0;

	void setData(const void* buff, size_t size);
	void resetData();
	const uint8_t *dataEnd();
	ssize_t readAtAddr(void* buff, size_t bytes, const uint8_t *readPos, std::error_code *ecOut);
};

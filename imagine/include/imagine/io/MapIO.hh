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

#include <imagine/io/IOUtils.hh>

namespace IG
{

class MapIO final : public IOUtils<MapIO>
{
public:
	using IOUtilsBase = IOUtils<MapIO>;
	using IOUtilsBase::write;
	using IOUtilsBase::seekS;
	using IOUtilsBase::seekE;
	using IOUtilsBase::seekC;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;

	constexpr MapIO() = default;
	MapIO(IOBuffer);
	explicit MapIO(Readable auto &&io): MapIO{io.buffer(BufferMode::RELEASE)} {}
	explicit MapIO(Readable auto &io): MapIO{io.buffer(BufferMode::DIRECT)} {}
	ssize_t read(void *buff, size_t bytes);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset);
	std::span<uint8_t> map();
	ssize_t write(const void *buff, size_t bytes);
	off_t seek(off_t offset, SeekMode mode);
	size_t size();
	bool eof();
	explicit operator bool() const;
	void advise(off_t offset, size_t bytes, Advice advice);
	IOBuffer releaseBuffer();

protected:
	uint8_t *currPos{};
	IOBuffer buff{};

	uint8_t *data() const;
	uint8_t *dataEnd() const;
	ssize_t readAtAddr(void* buff, size_t bytes, const uint8_t *readPos);
};

}

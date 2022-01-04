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

namespace IG
{

class MapIO final : public IO
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
	using IO::buffer;
	using IO::get;

	constexpr MapIO() = default;
	MapIO(IG::ByteBuffer buff);
	MapIO(IG::derived_from<IO> auto &&io): MapIO{io.buffer(IO::BufferMode::RELEASE)} {}
	MapIO(IG::derived_from<IO> auto &io): MapIO{io.buffer(IO::BufferMode::DIRECT)} {}
	ssize_t read(void *buff, size_t bytes) final;
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset) final;
	std::span<uint8_t> map() final;
	ssize_t write(const void *buff, size_t bytes) final;
	off_t seek(off_t offset, IO::SeekMode mode) final;
	size_t size() final;
	bool eof() final;
	explicit operator bool() const final;
	#if defined __linux__ || defined __APPLE__
	void advise(off_t offset, size_t bytes, Advice advice) final;
	#endif
	IG::ByteBuffer releaseBuffer();

protected:
	uint8_t *currPos{};
	IG::ByteBuffer buff{};

	uint8_t *data() const;
	uint8_t *dataEnd() const;
	ssize_t readAtAddr(void* buff, size_t bytes, const uint8_t *readPos);
};

}

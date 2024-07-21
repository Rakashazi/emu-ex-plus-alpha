#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/IO.hh>
#include <imagine/io/IOUtils-impl.hh>
#include <optional>

namespace EmuEx
{

using namespace IG;

class OutSizeTracker : public IOUtils<OutSizeTracker>
{
public:
	using IOUtilsBase = IOUtils<OutSizeTracker>;
	using IOUtilsBase::read;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::toFileStream;

	OutSizeTracker(size_t *sizePtr): sizePtr{sizePtr} {}
	ssize_t read(void*, size_t, [[maybe_unused]] std::optional<off_t> offset = {}) { return 0; }

	off_t seek(off_t offset, SeekMode mode)
	{
		size_t newPos = transformOffsetToAbsolute(mode, offset, 0, off_t(size()), off_t(currPos));
		if(newPos > size())
			return -1;
		currPos = newPos;
		return newPos;
	}

	size_t size() const { return *sizePtr; }
	std::span<uint8_t> map() { return {}; }
	explicit operator bool() const { return sizePtr; }

	ssize_t write(const void*, size_t bytes, [[maybe_unused]] std::optional<off_t> offset = {})
	{
		currPos += bytes;
		if(currPos > *sizePtr)
			*sizePtr = currPos;
		return bytes;
	}

private:
	size_t currPos{};
	size_t *sizePtr{};
};

}

namespace IG
{
template class IOUtils<EmuEx::OutSizeTracker>;
}

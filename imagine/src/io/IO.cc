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
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>
#include "IOUtils.hh"

namespace IG
{

template class IOUtils<IO>;

ssize_t IO::read(void *buff, size_t bytes) { return visit([&](auto &io){ return io.read(buff, bytes); }, *this); }
ssize_t IO::write(const void *buff, size_t bytes) { return visit([&](auto &io){ return io.write(buff, bytes); }, *this); }
off_t IO::seek(off_t offset, IOSeekMode mode) { return visit([&](auto &io){ return io.seek(offset, mode); }, *this); }
size_t IO::size() { return visit([&](auto &io){ return io.size(); }, *this); }
bool IO::eof() { return visit([&](auto &io){ return io.eof(); }, *this); }
IO::operator bool() const { return visit([&](auto &io){ return (bool)io; }, *this); }

std::span<uint8_t> IO::map()
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.map();})
			return io.map();
		else
			return std::span<uint8_t>{};
	}, *this);
};

bool IO::truncate(off_t offset)
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.truncate(offset);})
			return io.truncate(offset);
		else
			return false;
	}, *this);
};

void IO::sync()
{
	visit([&](auto &io)
	{
		if constexpr(requires {io.sync();})
			io.sync();
	}, *this);
}

void IO::advise(off_t offset, size_t bytes, Advice advice)
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.advise(offset, bytes, advice);})
			return io.advise(offset, bytes, advice);
	}, *this);
}

ssize_t IO::readAtPos(void *buff, size_t bytes, off_t offset)
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.readAtPos(buff, bytes, offset);})
			return io.readAtPos(buff, bytes, offset);
		else
			return readAtPosGeneric(buff, bytes, offset);
	}, *this);
}

}

namespace IG::FileUtils
{

ssize_t writeToPath(CStringView path, std::span<const unsigned char> src)
{
	auto f = FileIO{path, FILE_OPEN_NEW | FILE_TEST_BIT};
	return f.write(src.data(), src.size());
}

ssize_t writeToPath(CStringView path, IO &io)
{
	auto f = FileIO{path, FILE_OPEN_NEW | FILE_TEST_BIT};
	return io.send(f, nullptr, io.size());
}

ssize_t readFromPath(CStringView path, std::span<unsigned char> dest, IO::AccessHint accessHint)
{
	FileIO f{path, accessHint, FILE_TEST_BIT};
	return f.read(dest.data(), dest.size());
}

IOBuffer bufferFromPath(CStringView path, FileOpenFlags openFlags, size_t sizeLimit)
{
	FileIO file{path, IOAccessHint::ALL, openFlags};
	if(!file)
		return {};
	if(file.size() > sizeLimit)
	{
		if(openFlags & FILE_TEST_BIT)
			return {};
		else
			throw std::runtime_error(fmt::format("{} exceeds {} byte limit", path.data(), sizeLimit));
	}
	return file.buffer(IOBufferMode::RELEASE);
}

}

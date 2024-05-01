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
#include <imagine/io/IOUtils-impl.hh>

namespace IG
{

template class IOUtils<IO>;

ssize_t IO::read(void *buff, size_t bytes, std::optional<off_t> offset)
{
	return visit([&](auto &io){ return io.read(buff, bytes, offset); });
}

ssize_t IO::write(const void *buff, size_t bytes, std::optional<off_t> offset)
{
	return visit([&](auto &io){ return io.write(buff, bytes, offset);	});
}

off_t IO::seek(off_t offset, IOSeekMode mode) { return visit([&](auto &io){ return io.seek(offset, mode); }); }
size_t IO::size() { return visit([&](auto &io){ return io.size(); }); }
bool IO::eof() { return visit([&](auto &io){ return io.eof(); }); }
IO::operator bool() const { return visit([&](auto &io){ return (bool)io; }); }

std::span<uint8_t> IO::map()
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.map();})
			return io.map();
		else
			return std::span<uint8_t>{};
	});
};

ssize_t IO::writeVector(std::span<const OutVector> buffs, std::optional<off_t> offset)
{
	return visit([&](auto &io) -> ssize_t
	{
		if constexpr(requires {io.writeVector(buffs, offset);})
			return io.writeVector(buffs, offset);
		else
			return io.genericWriteVector(buffs, offset);
	});
}

bool IO::truncate(off_t offset)
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.truncate(offset);})
			return io.truncate(offset);
		else
			return false;
	});
};

void IO::sync()
{
	visit([&](auto &io)
	{
		if constexpr(requires {io.sync();})
			io.sync();
	});
}

void IO::advise(off_t offset, size_t bytes, Advice advice)
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.advise(offset, bytes, advice);})
			return io.advise(offset, bytes, advice);
	});
}

}

namespace IG::FileUtils
{

ssize_t writeToPath(CStringView path, std::span<const unsigned char> src)
{
	auto f = FileIO{path, OpenFlags::testNewFile()};
	return f.write(src).bytes;
}

ssize_t writeToPath(CStringView path, IO &io)
{
	auto f = FileIO{path, OpenFlags::testNewFile()};
	return io.send(f, nullptr, io.size());
}

ssize_t readFromPath(CStringView path, std::span<unsigned char> dest, IO::AccessHint accessHint)
{
	FileIO f{path, {.test = true, .accessHint = accessHint}};
	return f.read(dest).bytes;
}

IOBuffer bufferFromPath(CStringView path, OpenFlags openFlags, size_t sizeLimit)
{
	openFlags.accessHint = IOAccessHint::All;
	FileIO file{path, openFlags};
	if(!file)
		return {};
	if(file.size() > sizeLimit)
	{
		if(openFlags.test)
			return {};
		else
			throw std::runtime_error(std::format("{} exceeds {} byte limit", path, sizeLimit));
	}
	return file.buffer(IOBufferMode::Release);
}

}

/*  This file is part of EmuFramework.

	EmuFramework is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EmuFramework is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <mednafen/types.h>
#include <mednafen/FileStream.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/mednafen.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/utility.h>
#include <system_error>

namespace EmuEx
{
IG::ApplicationContext gAppContext();
}

namespace Mednafen
{

std::pair<IG::OpenFlags, uint8_t> modeToAttribs(uint32 mode)
{
	using namespace IG;
	switch(mode)
	{
		default:
			throw MDFN_Error(0, _("Unknown FileStream mode."));

		case FileStream::MODE_READ:
			return {{.read = true, .accessHint = IOAccessHint::Sequential}, Stream::ATTRIBUTE_READABLE};

		case FileStream::MODE_READ_WRITE:
			return {OpenFlags::createFile(), Stream::ATTRIBUTE_READABLE | Stream::ATTRIBUTE_WRITEABLE};

		case FileStream::MODE_WRITE:
			return {OpenFlags::newFile(), Stream::ATTRIBUTE_WRITEABLE};

		case FileStream::MODE_WRITE_INPLACE:
			return {{.write = true, .create = true}, Stream::ATTRIBUTE_WRITEABLE};

		case FileStream::MODE_WRITE_SAFE:
			return {{.write = true}, Stream::ATTRIBUTE_WRITEABLE};
	}
}

FileStream::FileStream(const std::string& path, const uint32 mode, const int do_lock, [[maybe_unused]] const uint32 buffer_size)
try:
	io{EmuEx::gAppContext().openFileUri(path, modeToAttribs(mode).first)},
 attribs{modeToAttribs(mode).second}
{
 assert(!do_lock);
}
catch(std::system_error &err)
{
	ErrnoHolder ene(err.code().value());
	throw MDFN_Error(ene.Errno(), _("Error opening file \"%s\": %s"), path.c_str(), ene.StrError());
}

FileStream::FileStream(std::span<uint8_t> buff):
	io{IG::MapIO{buff}} {}

FileStream::~FileStream() {}

uint64 FileStream::attributes(void)
{
 return ATTRIBUTE_SEEKABLE | attribs;
}

uint8 *FileStream::map(void) noexcept
{
 return io.map().data();
}

uint64 FileStream::map_size(void) noexcept
{
 return io.size();
}

void FileStream::unmap(void) noexcept {}

uint64 FileStream::read(void *data, uint64 count, bool error_on_eos)
{
	auto bytes = io.read(data, count);
	if(bytes != (ssize_t)count)
	{
		ErrnoHolder ene(errno);

		if(bytes == -1)
			throw(MDFN_Error(ene.Errno(), _("Error reading from opened file")));

		if(error_on_eos)
			throw(MDFN_Error(0, _("Unexpected EOF while reading from opened file")));
	}
	return bytes;
}

uint64 FileStream::readAtPos(void *data, uint64 count, uint64 pos)
{
	return io.read(data, count, pos);
}

void FileStream::write(const void *data, uint64 count)
{
	auto bytes = io.write(data, count);
	if(bytes == -1)
	{
		ErrnoHolder ene(errno);

		throw(MDFN_Error(ene.Errno(), _("Error writing to opened file")));
	}
}

void FileStream::truncate(uint64 length)
{
	io.truncate(length);
}

void FileStream::seek(int64 offset, int whence)
{
	io.seek(offset, (IG::IOSeekMode)whence);
}

void FileStream::flush(void) {}

uint64 FileStream::tell(void)
{
 auto offset = io.tell();
 if(offset == -1)
 {
  ErrnoHolder ene(errno);

  throw(MDFN_Error(ene.Errno(), _("Error getting position in opened file")));
 }
 return offset;
}

uint64 FileStream::size(void)
{
	return io.size();
}

void FileStream::close(void)
{
	io = {};
}

void FileStream::advise(off_t offset, size_t bytes, IG::IOAdvice advice)
{
	io.advise(offset, bytes, advice);
}

int FileStream::get_char()
{
	char c;
	io.read(&c, 1);
	return c;
}

uint64 MemoryStream::readAtPos(void *data, uint64 count, uint64 pos)
{
	auto prevPos = tell();
	seek(pos, SEEK_SET);
	auto bytes = read(data, count, false);
	seek(prevPos, SEEK_SET);
	return bytes;
}

void MemoryStream::setSize(size_t size)
{
	assert(size <= data_buffer_alloced);
	data_buffer_size = size;
}

uint64 Stream::readAtPos(void*, [[maybe_unused]] uint64 count, [[maybe_unused]] uint64 pos)
{
	bug_unreachable("Stream::readAtPos not implemented");
	return 0;
}

void Stream::advise(off_t, size_t, IG::IOAdvice) {}

}

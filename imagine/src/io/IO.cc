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
#include <imagine/io/api/stdio.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include "IOUtils.hh"

template class IOUtils<IO>;
template class IOUtils<GenericIO>;

std::span<uint8_t> IO::map() { return {}; };

std::error_code IO::truncate(off_t offset) { return {ENOSYS, std::system_category()}; };

void IO::sync() {}

void IO::advise(off_t offset, size_t bytes, Advice advice) {}

ssize_t IO::readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut)
{
	auto savedOffset = tell();
	seekS(offset);
	auto bytesRead = read(buff, bytes, ecOut);
	seekS(savedOffset);
	return bytesRead;
}

GenericIO::GenericIO(std::unique_ptr<IO> io): io{std::move(io)} {}

GenericIO::operator IO*()
{
	return io.get();
}

GenericIO::operator IO&()
{
	return *io;
}

IO *GenericIO::release()
{
	return io.release();
}

FILE *GenericIO::moveToFileStream(const char *opentype)
{
	if(!*io)
	{
		return nullptr;
	}
	#if defined __ANDROID__ || __APPLE__
	auto f = funopen(release(),
		[](void *cookie, char *buf, int size)
		{
			auto &io = *(IO*)cookie;
			return (int)io.read(buf, size);
		},
		[](void *cookie, const char *buf, int size)
		{
			auto &io = *(IO*)cookie;
			return (int)io.write(buf, size);
		},
		[](void *cookie, fpos_t offset, int whence)
		{
			auto &io = *(IO*)cookie;
			return (fpos_t)io.seek(offset, (IODefs::SeekMode)whence);
		},
		[](void *cookie)
		{
			delete (IO*)cookie;
			return 0;
		});
	#else
	cookie_io_functions_t funcs
	{
		.read =
			[](void *cookie, char *buf, size_t size)
			{
				auto &io = *(IO*)cookie;
				return (ssize_t)io.read(buf, size);
			},
		.write =
			[](void *cookie, const char *buf, size_t size)
			{
				auto &io = *(IO*)cookie;
				auto bytesWritten = io.write(buf, size);
				if(bytesWritten == -1)
				{
					bytesWritten = 0; // needs to return 0 for error
				}
				return (ssize_t)bytesWritten;
			},
		.seek =
			[](void *cookie, off64_t *position, int whence)
			{
				auto &io = *(IO*)cookie;
				auto newPos = io.seek(*position, (IODefs::SeekMode)whence);
				if(newPos == -1)
				{
					return -1;
				}
				*position = newPos;
				return 0;
			},
		.close =
			[](void *cookie)
			{
				delete (IO*)cookie;
				return 0;
			}
	};
	auto f = fopencookie(release(), opentype, funcs);
	#endif
	assert(f);
	return f;
}

ssize_t GenericIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	if(!io)
	{
		if(ecOut)
			*ecOut = {EBADF, std::system_category()};
		return -1;
	}
	return io->read(buff, bytes, ecOut);
}

ssize_t GenericIO::readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut)
{
	if(!io)
	{
		if(ecOut)
			*ecOut = {EBADF, std::system_category()};
		return -1;
	}
	return io->readAtPos(buff, bytes, offset, ecOut);
}

std::span<uint8_t> GenericIO::map()
{
	if(io)
		return io->map();
	else
		return {};
}

ssize_t GenericIO::write(const void *buff, size_t bytes, std::error_code *ecOut)
{
	if(!io)
	{
		if(ecOut)
			*ecOut = {EBADF, std::system_category()};
		return -1;
	}
	return io->write(buff, bytes, ecOut);
}

std::error_code GenericIO::truncate(off_t offset)
{
	return io ? io->truncate(offset) : std::error_code{EBADF, std::system_category()};
}

off_t GenericIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	if(!io)
	{
		if(ecOut)
			*ecOut = {EBADF, std::system_category()};
		return -1;
	}
	return io->seek(offset, mode, ecOut);
}

void GenericIO::sync()
{
	if(io)
		io->sync();
}

size_t GenericIO::size()
{
	return io ? io->size() : 0;
}

bool GenericIO::eof()
{
	return io ? io->eof() : false;
}

void GenericIO::advise(off_t offset, size_t bytes, IO::Advice advice)
{
	if(io)
		io->advise(offset, bytes, advice);
}

GenericIO::operator bool() const
{
	return io && *io;
}

namespace FileUtils
{

ssize_t writeToPath(IG::CStringView path, std::span<const unsigned char> src)
{
	auto f = FileIO::create(path, IO::OPEN_TEST);
	return f.write(src.data(), src.size());
}

ssize_t writeToPath(IG::CStringView path, IO &io)
{
	auto f = FileIO::create(path, IO::OPEN_TEST);
	return io.send(f, nullptr, io.size());
}

ssize_t readFromPath(IG::CStringView path, std::span<unsigned char> dest, IO::AccessHint accessHint)
{
	FileIO f{path, accessHint, IO::OPEN_TEST};
	return f.read(dest.data(), dest.size());
}

IG::ByteBuffer bufferFromPath(IG::CStringView path, unsigned openFlags, size_t sizeLimit)
{
	FileIO file{path, IO::AccessHint::ALL, openFlags};
	if(!file)
		return {};
	if(file.size() > sizeLimit)
	{
		if(openFlags & IO::OPEN_TEST)
			return {};
		else
			throw std::runtime_error(fmt::format("{} exceeds {} byte limit", path.data(), sizeLimit));
	}
	return file.buffer(IODefs::BufferMode::RELEASE);
}

}

int fgetc(IO &io)
{
	char byte;
	return io.read(&byte, 1) == 1 ? byte : EOF;
}

size_t fread(void *ptr, size_t size, size_t nmemb, IO &io)
{
	auto bytesRead = io.read(ptr, (size * nmemb));
	return bytesRead > 0 ? bytesRead / size : 0;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, IO &io)
{
	auto bytesWritten = io.write(ptr, (size * nmemb));
	return bytesWritten > 0 ? bytesWritten / size : 0;
}

long ftell(IO &io)
{
	return io.tell();
}

int fseek(IO &io, long offset, int whence)
{
	return io.seek(offset, (IODefs::SeekMode)whence) == -1 ? -1 : 0;
}

int fclose(IO &stream)
{
	return 0;
}

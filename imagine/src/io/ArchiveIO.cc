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

#define LOGTAG "ArchIO"
#include <imagine/io/ArchiveIO.hh>
#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include <imagine/io/IOUtils-impl.hh>
#include <archive.h>
#include <archive_entry.h>
#include <format>

namespace IG
{

template class IOUtils<ArchiveIO>;

static FS::file_type makeEntryType(int type)
{
	using namespace IG::FS;
	switch(type)
	{
		case AE_IFREG: return file_type::regular;
		case AE_IFDIR: return file_type::directory;
		case AE_IFLNK: return file_type::symlink;
		case AE_IFBLK: return file_type::block;
		case AE_IFCHR: return file_type::character;
		case AE_IFIFO: return file_type::fifo;
		case AE_IFSOCK: return file_type::socket;
	}
	return file_type::unknown;
}

constexpr size_t bufferedIoSize = 32 * 1024;

struct ArchiveControlBlock
{
	ArchiveControlBlock(IO io): io{std::move(io)} {}
	IO io;
};

struct ArchiveControlBlockWithBuffer : public ArchiveControlBlock
{
	using ArchiveControlBlock::ArchiveControlBlock;
	std::array<uint8_t, bufferedIoSize> buff;
};

static void setReadSupport(struct archive *arch)
{
	archive_read_support_format_7zip(arch);
	archive_read_support_format_rar(arch);
	archive_read_support_format_zip(arch);
}

ArchiveIO::ArchiveIO() = default;

ArchiveIO::~ArchiveIO() = default;

ArchiveIO::ArchiveIO(ArchiveIO&&) noexcept = default;

ArchiveIO &ArchiveIO::operator=(ArchiveIO &&) noexcept = default;

ArchiveIO::ArchiveIO(CStringView path)
{
	init(FileIO{path, {.accessHint = IOAccessHint::Sequential}});
}

ArchiveIO::ArchiveIO(FileIO io) { init(std::move(io)); }

ArchiveIO::ArchiveIO(IO io)
{
	visit(overloaded
	{
		[&](ArchiveIO &&io) { *this = std::move(io); },
		[&](auto &&io) { init(std::move(io)); }
	}, std::move(io));
}

void ArchiveIO::init(IO io)
{
	UniqueArchive newArch{archive_read_new()};
	setReadSupport(newArch.get());
	auto seekFunc =
		[](struct archive *, void *data, int64_t offset, int whence) -> int64_t
		{
			//logMsg("seek %lld %d", (long long)offset, whence);
			auto &a = *((ArchiveControlBlock*)data);
			auto newPos = a.io.seek(offset, (IOSeekMode)whence);
			if(newPos == -1)
			{
				logErr("error seeking to %llu", (long long)newPos);
				return ARCHIVE_FATAL;
			}
			return newPos;
		};
	auto skipFunc =
		[](struct archive *, void *data, int64_t request) -> int64_t
		{
			//logMsg("skip %lld", (long long)request);
			auto &a = *((ArchiveControlBlock*)data);
			if(auto bytes = a.io.seek(request, IOSeekMode::Cur);
				bytes != -1)
				return bytes;
			else
				return ARCHIVE_FAILED;
		};
	archive_read_set_seek_callback(newArch.get(), seekFunc);
	int openRes = ARCHIVE_FATAL;
	std::unique_ptr<ArchiveControlBlock> newCtrlBlock{};
	if(auto fileMap = io.map();
		fileMap.data())
	{
		//logMsg("source IO supports mapping");
		auto readFunc =
			[](struct archive *, void *data, const void **buffOut) -> ssize_t
			{
				auto &a = *((ArchiveControlBlock*)data);
				// return the entire buffer after the file position
				auto pos = a.io.tell();
				ssize_t bytesRead = a.io.size() - pos;
				if(bytesRead)
				{
					*buffOut = a.io.map().data() + pos;
					a.io.seek(bytesRead, IOSeekMode::Cur);
				}
				//logMsg("read %lld bytes @ offset:%llu", (long long)bytesRead, (long long)pos);
				return bytesRead;
			};
		newCtrlBlock = std::make_unique<ArchiveControlBlock>(std::move(io));
		openRes = archive_read_open2(newArch.get(),
			newCtrlBlock.get(), nullptr, readFunc, skipFunc, nullptr);
	}
	else
	{
		auto readFunc =
			[](struct archive *, void *data, const void **buffOut) -> ssize_t
			{
				auto &a = *((ArchiveControlBlockWithBuffer*)data);
				auto buffPtr = a.buff.data();
				auto bytesRead = a.io.read(buffPtr, a.buff.size());
				*buffOut = buffPtr;
				return bytesRead;
			};
		newCtrlBlock = std::make_unique<ArchiveControlBlockWithBuffer>(std::move(io));
		openRes = archive_read_open2(newArch.get(),
			newCtrlBlock.get(), nullptr, readFunc, skipFunc, nullptr);
	}
	if(openRes != ARCHIVE_OK)
	{
		auto errString = archive_error_string(newArch.get());
		if(Config::DEBUG_BUILD)
			logErr("error opening archive:%s", errString);
		throw std::runtime_error{std::format("Error opening archive: {}", errString)};
	}
	logMsg("opened archive:%p", newArch.get());
	arch = std::move(newArch);
	ctrlBlock = std::move(newCtrlBlock);
	readNextEntry(); // go to first entry
}

void ArchiveIO::freeArchive(struct archive *arch)
{
	if(!arch)
		return;
	logMsg("freeing archive:%p", arch);
	archive_read_free(arch);
}

std::string_view ArchiveIO::name() const
{
	assumeExpr(ptr);
	auto name = archive_entry_pathname(ptr);
	return name ? name : "";
}

FS::file_type ArchiveIO::type() const
{
	assumeExpr(ptr);
	return makeEntryType(archive_entry_filetype(ptr));
}

size_t ArchiveIO::size()
{
	assumeExpr(ptr);
	return archive_entry_size(ptr);
}

uint32_t ArchiveIO::crc32() const
{
	assumeExpr(ptr);
	return archive_entry_crc32(ptr);
}

bool ArchiveIO::readNextEntry()
{
	if(!arch) [[unlikely]]
		return false;
	ptr = {};
	struct archive_entry *entryPtr{};
	auto ret = archive_read_next_header(arch.get(), &entryPtr);
	if(ret == ARCHIVE_EOF)
	{
		logMsg("reached archive end");
		return false;
	}
	else if(ret <= ARCHIVE_FAILED)
	{
		if(Config::DEBUG_BUILD)
			logErr("error reading archive entry:%s", archive_error_string(arch.get()));
		return false;
	}
	else if(ret != ARCHIVE_OK)
	{
		if(Config::DEBUG_BUILD)
			logWarn("warning reading archive entry:%s", archive_error_string(arch.get()));
	}
	ptr = entryPtr;
	return true;
}

bool ArchiveIO::hasEntry() const
{
	return arch && ptr;
}

void ArchiveIO::rewind()
{
	if(!arch) [[unlikely]]
		return;
	logMsg("rewinding archive:%p", arch.get());
	// take the existing IO, rewind, and re-use it
	auto io = std::move(ctrlBlock->io);
	io.rewind();
	arch = {};
	ctrlBlock = {};
	init(std::move(io));
}

ssize_t ArchiveIO::read(void *buff, size_t bytes, std::optional<off_t> offset)
{
	if(!*this) [[unlikely]]
	{
		return -1;
	}
	if(offset)
	{
		return readAtPosGeneric(buff, bytes, *offset);
	}
	else
	{
		int bytesRead = archive_read_data(archive(), buff, bytes);
		if(bytesRead < 0)
		{
			bytesRead = -1;
		}
		return bytesRead;
	}
}

ssize_t ArchiveIO::write(const void*, size_t, std::optional<off_t>)
{
	return -1;
}

off_t ArchiveIO::seek(off_t offset, IOSeekMode mode)
{
	if(!*this) [[unlikely]]
	{
		return -1;
	}
	long newPos = archive_seek_data(archive(), offset, (int)mode);
	if(newPos < 0)
	{
		if(offset == 0 && mode == IOSeekMode::Cur) // archive not seekable, report position as 0
		{
			return 0;
		}
		logErr("seek to offset %lld failed", (long long)offset);
		return -1;
	}
	return newPos;
}

bool ArchiveIO::eof()
{
	return tell() == (off_t)size();
}

ArchiveIO::operator bool() const
{
	return (bool)archive();
}

}

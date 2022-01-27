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
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include <archive.h>
#include <archive_entry.h>

namespace IG
{

static FS::file_type makeEntryType(int type)
{
	using namespace FS;
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

static constexpr size_t bufferedIoSize = 32 * 1024;

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

ArchiveEntry::ArchiveEntry(IG::CStringView path)
{
	init(FileIO{path, IO::AccessHint::SEQUENTIAL});
}

ArchiveEntry::ArchiveEntry(GenericIO io)
{
	init(std::move(io));
}

void ArchiveEntry::init(GenericIO io)
{
	UniqueArchive newArch{archive_read_new()};
	setReadSupport(newArch.get());
	auto seekFunc =
		[](struct archive *, void *data, int64_t offset, int whence) -> int64_t
		{
			//logMsg("seek %lld %d", (long long)offset, whence);
			auto &a = *((ArchiveControlBlock*)data);
			auto newPos = a.io.seek(offset, (IODefs::SeekMode)whence);
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
			if(auto bytes = a.io.seekC(request);
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
					a.io.seekC(bytesRead);
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
		throw std::runtime_error{fmt::format("Error opening archive: {}", errString)};
	}
	logMsg("opened archive:%p", newArch.get());
	arch = std::move(newArch);
	ctrlBlock = std::move(newCtrlBlock);
	readNextEntry(); // go to first entry
}

void ArchiveEntry::freeArchive(struct archive *arch)
{
	if(!arch)
		return;
	logMsg("freeing archive:%p", arch);
	archive_read_free(arch);
}

std::string_view ArchiveEntry::name() const
{
	assumeExpr(ptr);
	auto name = archive_entry_pathname(ptr);
	return name ? name : "";
}

FS::file_type ArchiveEntry::type() const
{
	assumeExpr(ptr);
	return makeEntryType(archive_entry_filetype(ptr));
}

size_t ArchiveEntry::size() const
{
	assumeExpr(ptr);
	return archive_entry_size(ptr);
}

uint32_t ArchiveEntry::crc32() const
{
	assumeExpr(ptr);
	return archive_entry_crc32(ptr);
}

ArchiveIO ArchiveEntry::moveIO()
{
	return ArchiveIO{std::move(*this)};
}

void ArchiveEntry::moveIO(ArchiveIO io)
{
	*this = io.releaseArchive();
}

bool ArchiveEntry::readNextEntry()
{
	if(!arch) [[unlikely]]
		return false;
	auto ret = archive_read_next_header(arch.get(), &ptr);
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
	return true;
}

bool ArchiveEntry::hasEntry() const
{
	return ptr;
}

void ArchiveEntry::rewind()
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

ArchiveIO::ArchiveIO(ArchiveEntry entry):
	entry{std::move(entry)}
{}

ArchiveEntry ArchiveIO::releaseArchive()
{
	return std::move(entry);
}

std::string_view ArchiveIO::name() const
{
	return entry.name();
}

ssize_t ArchiveIO::read(void *buff, size_t bytes)
{
	if(!*this) [[unlikely]]
	{
		return -1;
	}
	int bytesRead = archive_read_data(entry.archive(), buff, bytes);
	if(bytesRead < 0)
	{
		bytesRead = -1;
	}
	return bytesRead;
}

ssize_t ArchiveIO::write(const void* buff, size_t bytes)
{
	return -1;
}

off_t ArchiveIO::seek(off_t offset, IO::SeekMode mode)
{
	if(!*this) [[unlikely]]
	{
		return -1;
	}
	long newPos = archive_seek_data(entry.archive(), offset, (int)mode);
	if(newPos < 0)
	{
		logErr("seek to offset %lld failed", (long long)offset);
		return -1;
	}
	return newPos;
}

size_t ArchiveIO::size()
{
	return entry.size();
}

bool ArchiveIO::eof()
{
	return tell() == (off_t)entry.size();
}

ArchiveIO::operator bool() const
{
	return (bool)entry.archive();
}

}

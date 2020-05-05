#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/io/IO.hh>
#include <imagine/io/BufferMapIO.hh>
#include <imagine/fs/FSDefs.hh>
#include <memory>

struct archive;
struct archive_entry;
struct ArchiveGenericIO;
class ArchiveIO;

class ArchiveEntry
{
public:
	std::shared_ptr<struct archive> arch{};
	struct archive_entry *ptr{};
	ArchiveGenericIO *genericIO{};

	const char *name() const;
	FS::file_type type() const;
	size_t size() const;
	uint32_t crc32() const;
	ArchiveIO moveIO();
	void moveIO(ArchiveIO io);
};

class ArchiveIO final : public IO
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

	constexpr ArchiveIO() {}
	ArchiveIO(ArchiveEntry entry);
	ArchiveIO(ArchiveIO &&o);
	ArchiveIO &operator=(ArchiveIO &&o);
	~ArchiveIO() final;
	GenericIO makeGeneric();
	ArchiveEntry releaseArchive();
	const char *name();
	BufferMapIO moveToMapIO();

	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut) final;
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut) final;
	off_t seek(off_t offset, SeekMode mode, std::error_code *ecOut) final;
	void close() final;
	size_t size() final;
	bool eof() final;
	explicit operator bool() const final;

private:
	ArchiveEntry entry{};
};

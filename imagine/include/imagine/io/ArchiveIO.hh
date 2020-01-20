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
	uint32 crc32() const;
	ArchiveIO moveIO();
	void moveIO(ArchiveIO io);
};

class ArchiveIO : public IO
{
public:
	using IOUtils::read;
	using IOUtils::write;
	using IOUtils::seek;

	ArchiveIO() {}
	ArchiveIO(ArchiveEntry entry):
		entry{entry}
	{}
	~ArchiveIO() final;
	ArchiveIO(ArchiveIO &&o);
	ArchiveIO &operator=(ArchiveIO &&o);
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
	explicit operator bool() final;

private:
	ArchiveEntry entry{};

	// no copying outside of class
	ArchiveIO(const ArchiveIO &) = default;
	ArchiveIO &operator=(const ArchiveIO &) = default;
};

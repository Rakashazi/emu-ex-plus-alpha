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

#include <imagine/io/IOUtils.hh>
#include <imagine/util/string/CStringView.hh>
#include <memory>

struct archive;
struct archive_entry;

namespace IG::FS
{
enum class file_type : int8_t;
}

namespace IG
{

class ArchiveIO;
class IO;

// data used by libarchive callbacks allocated in its own memory block
struct ArchiveControlBlock;

class ArchiveEntry
{
public:
	ArchiveEntry();
	~ArchiveEntry();
	ArchiveEntry(CStringView path);
	explicit ArchiveEntry(IO);
	ArchiveEntry(ArchiveEntry&&) noexcept;
	ArchiveEntry &operator=(ArchiveEntry&&) noexcept;
	std::string_view name() const;
	FS::file_type type() const;
	size_t size() const;
	uint32_t crc32() const;
	ArchiveIO moveIO();
	void moveIO(ArchiveIO io);
	bool readNextEntry();
	bool hasEntry() const;
	void rewind();
	struct archive* archive() const { return arch.get(); }

protected:
	struct ArchiveDeleter
	{
		void operator()(struct archive *ptr) const
		{
			freeArchive(ptr);
		}
	};
	using UniqueArchive = std::unique_ptr<struct archive, ArchiveDeleter>;

	UniqueArchive arch{};
	struct archive_entry *ptr{};
	std::unique_ptr<ArchiveControlBlock> ctrlBlock{};

	void init(IO);
	static void freeArchive(struct archive *);
};

class ArchiveIO final : public IOUtils<ArchiveIO>
{
public:
	using IOUtilsBase = IOUtils<ArchiveIO>;
	using IOUtilsBase::write;
	using IOUtilsBase::seekS;
	using IOUtilsBase::seekE;
	using IOUtilsBase::seekC;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;

	constexpr ArchiveIO() = default;
	ArchiveIO(ArchiveEntry entry);
	ArchiveEntry releaseArchive();
	std::string_view name() const;
	ssize_t read(void *buff, size_t bytes);
	ssize_t write(const void *buff, size_t bytes);
	off_t seek(off_t offset, SeekMode mode);
	size_t size();
	bool eof();
	explicit operator bool() const;

protected:
	ArchiveEntry entry{};
};

}

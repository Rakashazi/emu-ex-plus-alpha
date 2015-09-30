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

#include <imagine/engine-globals.h>
#include <imagine/io/IO.hh>
#include <array>

struct archive;
struct archive_entry;

class ArchiveIO : public IO
{
public:
	using IOUtils::read;
	using IOUtils::write;
	using IOUtils::seek;

	ArchiveIO() {}
	ArchiveIO(std::shared_ptr<struct archive> arch, struct archive_entry *entry):
		arch{std::move(arch)}, entry{entry}
	{}
	~ArchiveIO() override;
	ArchiveIO(ArchiveIO &&o);
	ArchiveIO &operator=(ArchiveIO &&o);
	operator GenericIO();
	std::shared_ptr<struct archive> releaseArchive();
	const char *name();

	ssize_t read(void *buff, size_t bytes, CallResult *resultOut) override;
	ssize_t write(const void *buff, size_t bytes, CallResult *resultOut) override;
	off_t seek(off_t offset, SeekMode mode, CallResult *resultOut) override;
	void close() override;
	size_t size() override;
	bool eof() override;
	explicit operator bool() override;

private:
	std::shared_ptr<struct archive> arch{};
	struct archive_entry *entry{};

	// no copying outside of class
	ArchiveIO(const ArchiveIO &) = default;
	ArchiveIO &operator=(const ArchiveIO &) = default;
};

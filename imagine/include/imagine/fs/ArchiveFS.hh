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

#include <memory>
#include <imagine/engine-globals.h>
#include <imagine/util/operators.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/io/ArchiveIO.hh>

struct archive;
struct archive_entry;

namespace FS
{

class ArchiveEntry
{
public:
	std::shared_ptr<struct archive> arch{};
	struct archive_entry *entry{};

	const char *name() const;
	file_type type() const;
	ArchiveIO moveIO();
	void moveIO(ArchiveIO io);
};

class ArchiveIterator :
	public std::iterator<std::input_iterator_tag, ArchiveEntry>,
	public NotEquals<ArchiveIterator>
{
public:
	ArchiveEntry archEntry{};

	ArchiveIterator() {}
	ArchiveIterator(PathString path): ArchiveIterator{path.data()} {}
	ArchiveIterator(const char *path);
	ArchiveIterator(PathString path, CallResult &result): ArchiveIterator{path.data(), result} {}
	ArchiveIterator(const char *path, CallResult &result);
	ArchiveIterator(GenericIO io);
	ArchiveIterator(GenericIO io, CallResult &result);
	~ArchiveIterator();
	ArchiveEntry& operator*();
	ArchiveEntry* operator->();
	void operator++();
	bool operator==(ArchiveIterator const &rhs) const;

private:
	void init(const char *path, CallResult &result);
	void init(GenericIO io, CallResult &result);
};

static const ArchiveIterator &begin(const ArchiveIterator &iter)
{
	return iter;
}

static ArchiveIterator end(const ArchiveIterator &)
{
	return {};
}

ArchiveIO fileFromArchive(const char *archivePath, const char *filePath);
static ArchiveIO fileFromArchive(PathString archivePath, PathString filePath)
{
	return fileFromArchive(archivePath.data(), filePath.data());
}

};

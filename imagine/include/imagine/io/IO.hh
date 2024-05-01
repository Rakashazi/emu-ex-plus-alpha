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

#include <imagine/io/PosixIO.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/io/ArchiveIO.hh>
#ifdef __ANDROID__
#include <imagine/io/AAssetIO.hh>
#endif
#include <imagine/util/variant.hh>
#include <variant>
#include <span>
#include <optional>

namespace IG
{

using IOVariant = std::variant<
PosixIO,
MapIO,
#ifdef __ANDROID__
AAssetIO,
#endif
ArchiveIO>;

class IO : public IOVariant, public IOUtils<IO>, public AddVisit
{
public:
	using IOVariant::IOVariant;
	using IOVariant::operator=;
	using AddVisit::visit;
	using IOUtilsBase = IOUtils<IO>;
	using IOUtilsBase::read;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;
	using AccessHint = IOAccessHint;
	using Advice = IOAdvice;
	using BufferMode = IOBufferMode;
	using SeekMode = IOSeekMode;

	IO(IOBuffer buff): IOVariant{std::in_place_type<MapIO>, std::move(buff)} {}

	// core API
	ssize_t read(void *buff, size_t bytes, std::optional<off_t> offset = {});
	ssize_t write(const void *buff, size_t bytes, std::optional<off_t> offset = {});
	off_t seek(off_t offset, SeekMode mode);
	size_t size();
	bool eof();
	explicit operator bool() const;

	// optional API
	ssize_t writeVector(std::span<const OutVector> buffs, std::optional<off_t> offset = {});
	bool truncate(off_t offset);
	std::span<uint8_t> map();
	void sync();
	void advise(off_t offset, size_t bytes, Advice advice);
};

}

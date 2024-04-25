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

#pragma once

#include <imagine/io/IOUtils.hh>
#include <span>

namespace IG
{

class MapIO : public IOUtils<MapIO>
{
public:
	using IOUtilsBase = IOUtils<MapIO>;
	using IOUtilsBase::read;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;

	constexpr MapIO() = default;
	MapIO(IOBuffer buff): buff{std::move(buff)} {}
	MapIO(std::span<uint8_t> buff): MapIO{IOBuffer{buff}} {}
	explicit MapIO(Readable auto &&io): MapIO{io.buffer(BufferMode::Release)} {}
	explicit MapIO(Readable auto &io): MapIO{io.buffer(BufferMode::Direct)} {}
	ssize_t read(void *buff, size_t bytes, std::optional<off_t> offset = {});
	ssize_t write(const void *buff, size_t bytes, std::optional<off_t> offset = {});
	off_t seek(off_t offset, SeekMode mode);
	size_t size() const { return buff.size(); }
	bool eof() const { return currPos == size(); }
	void sync();
	void advise(off_t offset, size_t bytes, Advice advice);
	auto data(this auto&& self) { return self.buff.data(); }
	std::span<uint8_t> map() { return {data(), size()}; }
	explicit operator bool() const { return data(); }
	IOBuffer releaseBuffer() { return std::move(buff); }
	std::span<uint8_t> subSpan(off_t offset, size_t maxBytes) const;
	MapIO subView(off_t offset, size_t maxBytes) const { return IOBuffer{subSpan(offset, maxBytes), 0}; }

private:
	size_t currPos{};
	IOBuffer buff{};

	ssize_t copyBuffer(auto *buff, size_t bytes, std::optional<off_t> offset);
};

}

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
#include <imagine/util/bitset.hh>
#include <imagine/util/BufferView.hh>
#include <imagine/util/concepts.hh>
#include <memory>
#include <utility>
#include <system_error>
#include <unistd.h> // for SEEK_*

class IODefs
{
public:
	enum class Advice
	{
		NORMAL, SEQUENTIAL, RANDOM, WILLNEED
	};

	enum class AccessHint
	{
		NORMAL, SEQUENTIAL, RANDOM, ALL
	};

	using SeekMode = int;
};

template <class IO>
class IOUtils
{
public:
	ssize_t read(void *buff, size_t bytes);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset);
	ssize_t write(const void *buff, size_t bytes);
	off_t seek(off_t offset, IODefs::SeekMode mode);
	off_t seekS(off_t offset, std::error_code *ecOut = nullptr);
	off_t seekE(off_t offset, std::error_code *ecOut = nullptr);
	off_t seekC(off_t offset, std::error_code *ecOut = nullptr);
	bool rewind();
	off_t tell(std::error_code *ecOut = nullptr);
	ssize_t send(IO &output, off_t *srcOffset, size_t bytes, std::error_code *ecOut = nullptr);
	IG::ConstByteBufferView constBufferView();

	template <class T>
	std::pair<T, ssize_t> read(std::error_code *ecOut = nullptr)
	{
		T obj;
		ssize_t size;
		if constexpr(std::is_same_v<T, bool>)
		{
			// special case to convert value to a valid bool
			uint8_t tmpObj;
			size = static_cast<IO*>(this)->read(&tmpObj, sizeof(T), ecOut);
			obj = tmpObj;
		}
		else
		{
			size = static_cast<IO*>(this)->read(&obj, sizeof(T), ecOut);
		}
		if(size == -1)
			return {{}, size};
		return {obj, size};
	}

	template <class T>
	T get()
	{
		return read<T>(nullptr).first;
	}

	template <class T> requires (!IG::Pointer<std::decay_t<T>>)
	ssize_t write(T &&obj, std::error_code *ecOut = nullptr)
	{
		return static_cast<IO*>(this)->write(&obj, sizeof(T), ecOut);
	}
};

class IO : public IOUtils<IO>, public IODefs
{
public:
	using IOUtilsBase = IOUtils<IO>;
	using IOUtilsBase::read;
	using IOUtilsBase::readAtPos;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::seekS;
	using IOUtilsBase::seekE;
	using IOUtilsBase::seekC;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::constBufferView;
	using IOUtilsBase::get;

	// allow reading file, default if OPEN_WRITE isn't present
	static constexpr uint32_t OPEN_READ = IG::bit(0);
	// allow modifying file
	static constexpr uint32_t OPEN_WRITE = IG::bit(1);
	// create a new file, clobbering any existing one,
	// if OPEN_CREATE isn't present, only existing files are opened
	// for reading/writing/appending
	static constexpr uint32_t OPEN_CREATE = IG::bit(2);
	// if using OPEN_CREATE, don't overwrite a file that already exists
	static constexpr uint32_t OPEN_KEEP_EXISTING = IG::bit(3);
	static constexpr uint32_t OPEN_FLAGS_BITS = 4;

	constexpr IO() {}
	virtual ~IO() = default;

	// reading
	virtual ssize_t read(void *buff, size_t bytes, std::error_code *ecOut) = 0;
	virtual ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut);
	virtual const uint8_t *mmapConst();

	// writing
	virtual ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut) = 0;
	virtual std::error_code truncate(off_t offset);

	// seeking
	virtual off_t seek(off_t offset, SeekMode mode, std::error_code *ecOut) = 0;

	// other functions
	virtual void close() = 0;
	virtual void sync();
	virtual size_t size() = 0;
	virtual bool eof() = 0;
	virtual void advise(off_t offset, size_t bytes, Advice advice);
	virtual explicit operator bool() const = 0;
};

class GenericIO : public IOUtils<GenericIO>
{
public:
	using IOUtilsBase = IOUtils<GenericIO>;
	using IOUtilsBase::read;
	using IOUtilsBase::readAtPos;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::seekS;
	using IOUtilsBase::seekE;
	using IOUtilsBase::seekC;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::constBufferView;
	using IOUtilsBase::get;

	constexpr GenericIO() {}
	GenericIO(IG::derived_from<IO> auto io): io{std::make_unique<decltype(io)>(std::move(io))} {}
	GenericIO(std::unique_ptr<IO> io);
	explicit operator IO*();
	operator IO&();
	IO *release();
	FILE *moveToFileStream(const char *opentype);

	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut);
	const uint8_t *mmapConst();
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut);
	std::error_code truncate(off_t offset);
	off_t seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut);
	void close();
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, IO::Advice advice);
	explicit operator bool() const;

protected:
	std::unique_ptr<IO> io{};
};

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
#include <imagine/util/bits.h>
#include <imagine/util/BufferView.hh>
#include <memory>
#include <algorithm>
#include <system_error>
#include <cstddef>
#include <cstdio> // for SEEK_*

class IODefs
{
public:
	enum Advice
	{
		ADVICE_SEQUENTIAL, ADVICE_WILLNEED
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
	off_t seekS(off_t offset, std::error_code *ecOut);
	off_t seekE(off_t offset, std::error_code *ecOut);
	off_t seekC(off_t offset, std::error_code *ecOut);
	off_t seekS(off_t offset);
	off_t seekE(off_t offset);
	off_t seekC(off_t offset);
	off_t tell(std::error_code *ecOut);
	off_t tell();
	std::error_code readAll(void *buff, size_t bytes);
	std::error_code writeAll(void *buff, size_t bytes);
	std::error_code writeToIO(IO &io);
	IG::ConstBufferView constBufferView();

	template <class T>
	T readVal(std::error_code *ecOut)
	{
		T val;
		if(((IO*)this)->read(&val, sizeof(T), ecOut) != sizeof(T))
		{
			if(ecOut)
				*ecOut = {EINVAL, std::system_category()};
			val = {};
		}
		return val;
	}

	template <class T>
	T readVal()
	{
		return readVal<T>(nullptr);
	}

	template <class T>
	void writeVal(T val, std::error_code *ecOut)
	{
		if(((IO*)this)->write(&val, sizeof(T), ecOut) != sizeof(T))
		{
			if(ecOut)
				*ecOut = {EINVAL, std::system_category()};
		}
	}
};

class IO : public IOUtils<IO>, public IODefs
{
public:
	using IOUtils::read;
	using IOUtils::readAtPos;
	using IOUtils::write;
	using IOUtils::seek;

	// allow reading file, default if OPEN_WRITE isn't present
	static constexpr uint OPEN_READ = IG::bit(0);
	// allow modifying file
	static constexpr uint OPEN_WRITE = IG::bit(1);
	// create a new file, clobbering any existing one,
	// if OPEN_CREATE isn't present, only existing files are opened
	// for reading/writing/appending
	static constexpr uint OPEN_CREATE = IG::bit(2);
	// if using OPEN_CREATE, don't overwrite a file that already exists
	static constexpr uint OPEN_KEEP_EXISTING = IG::bit(3);
	static constexpr uint OPEN_FLAGS_BITS = 4;

	constexpr IO() {}
	virtual ~IO() = 0;

	// reading
	virtual ssize_t read(void *buff, size_t bytes, std::error_code *ecOut) = 0;
	virtual ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut);
	virtual const char *mmapConst() { return nullptr; };

	// writing
	virtual ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut) = 0;
	virtual std::error_code truncate(off_t offset) { return {ENOSYS, std::system_category()}; };

	// seeking
	virtual off_t seek(off_t offset, SeekMode mode, std::error_code *ecOut) = 0;

	// other functions
	virtual void close() = 0;
	virtual void sync() {}
	virtual size_t size() = 0;
	virtual bool eof() = 0;
	virtual void advise(off_t offset, size_t bytes, Advice advice) {}
	virtual explicit operator bool() = 0;
};

class GenericIO : public IOUtils<GenericIO>
{
public:
	using IOUtils::read;
	using IOUtils::readAtPos;
	using IOUtils::write;
	using IOUtils::seek;

	GenericIO() {}
	template<class T>
	GenericIO(T &io): io{std::unique_ptr<IO>{new T(std::move(io))}} {}
	GenericIO(std::unique_ptr<IO> io): io{std::move(io)} {}
	GenericIO(GenericIO &&o);
	GenericIO &operator=(GenericIO &&o);
	operator IO*(){ return io.get(); }
	operator IO&(){ return *io; }
	IO *release() { return io.release(); }
	FILE *moveToFileStream(const char *opentype);

	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut);
	const char *mmapConst();
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut);
	std::error_code truncate(off_t offset);
	off_t seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut);
	void close();
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, IO::Advice advice);
	explicit operator bool();

protected:
	std::unique_ptr<IO> io{};
};

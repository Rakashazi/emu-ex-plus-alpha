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
#include <imagine/util/bits.h>
#include <memory>
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
	ssize_t read(void *buff, size_t bytes) { return ((IO*)this)->read(buff, bytes, nullptr); }
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset) { return ((IO*)this)->readAtPos(buff, bytes, offset, nullptr); }
	ssize_t write(const void *buff, size_t bytes) { return ((IO*)this)->write(buff, bytes, nullptr); }
	off_t seek(off_t offset, IODefs::SeekMode mode) { return ((IO*)this)->seek(offset, mode, nullptr); }
	off_t seekS(off_t offset, CallResult *resultOut) { return ((IO*)this)->seek(offset, SEEK_SET, resultOut); }
	off_t seekE(off_t offset, CallResult *resultOut) { return ((IO*)this)->seek(offset, SEEK_END, resultOut); }
	off_t seekC(off_t offset, CallResult *resultOut) { return ((IO*)this)->seek(offset, SEEK_CUR, resultOut); }
	off_t seekS(off_t offset) { return ((IO*)this)->seek(offset, SEEK_SET); }
	off_t seekE(off_t offset) { return ((IO*)this)->seek(offset, SEEK_END); }
	off_t seekC(off_t offset) { return ((IO*)this)->seek(offset, SEEK_CUR); }

	off_t tell(CallResult *resultOut)
	{
		return ((IO*)this)->seekC(0, resultOut);
	}

	off_t tell() { return ((IO*)this)->tell(nullptr); }

	CallResult readAll(void *buff, size_t bytes)
	{
		CallResult r = OK;
		auto bytesRead = ((IO*)this)->read(buff, bytes, &r);
		if(bytesRead != (ssize_t)bytes)
			return r == OK ? (CallResult)INVALID_PARAMETER : r;
		return OK;
	}

	CallResult writeAll(void *buff, size_t bytes)
	{
		CallResult r = OK;
		auto bytesWritten = ((IO*)this)->write(buff, bytes, &r);
		if(bytesWritten != (ssize_t)bytes)
			return r == OK ? (CallResult)INVALID_PARAMETER : r;
		return OK;
	}

	template <class T>
	T readVal(CallResult *resultOut)
	{
		T val;
		if(((IO*)this)->read(&val, sizeof(T), resultOut) != sizeof(T))
		{
			if(resultOut)
				*resultOut = IO_ERROR;
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
	void writeVal(T val, CallResult *resultOut)
	{
		if(((IO*)this)->write(&val, sizeof(T), resultOut) != sizeof(T))
		{
			if(resultOut)
				*resultOut = IO_ERROR;
		}
	}

	CallResult writeToIO(IO &io)
	{
		seekS(0);
		ssize_t bytesToWrite = ((IO*)this)->size();
		while(bytesToWrite)
		{
			char buff[4096];
			ssize_t bytes = std::min((ssize_t)sizeof(buff), bytesToWrite);
			auto r = readAll(buff, bytes);
			if(r != OK)
			{
				return r;
			}
			if(io.write(buff, bytes) != bytes)
			{
				return IO_ERROR;
			}
			bytesToWrite -= bytes;
		}
		return OK;
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
	virtual ssize_t read(void *buff, size_t bytes, CallResult *resultOut) = 0;
	virtual ssize_t readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut);
	virtual const char *mmapConst() { return nullptr; };

	// writing
	virtual ssize_t write(const void *buff, size_t bytes, CallResult *resultOut) = 0;
	virtual CallResult truncate(off_t offset) { return UNSUPPORTED_OPERATION; };

	// seeking
	virtual off_t seek(off_t offset, SeekMode mode, CallResult *resultOut) = 0;

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

	ssize_t read(void *buff, size_t bytes, CallResult *resultOut);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut);
	const char *mmapConst();
	ssize_t write(const void *buff, size_t bytes, CallResult *resultOut);
	CallResult truncate(off_t offset);
	off_t seek(off_t offset, IO::SeekMode mode, CallResult *resultOut);
	void close();
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, IO::Advice advice);
	explicit operator bool();

protected:
	std::unique_ptr<IO> io{};
};

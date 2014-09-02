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
#include <limits.h>
#include <algorithm>

// allow writes, creates a new file if not present
#define IO_OPEN_WRITE bit(0)
#define IO_OPEN_USED_BITS 1

// keep the existing file if present without overwriting it
#define IO_CREATE_KEEP bit(0)
#define IO_CREATE_USED_BITS 1

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

enum { IO_SEEK_ABS, IO_SEEK_ABS_END, IO_SEEK_REL };

class Io
{
public:
	enum Advice { ADVICE_SEQUENTIAL, ADVICE_WILLNEED };

	constexpr Io() {}
	virtual ~Io() {}

	// reading
	virtual ssize_t readUpTo(void* buffer, size_t numBytes) = 0;
	virtual ssize_t readAtPos(void* buffer, size_t numBytes, ulong offset);
	virtual const char *mmapConst() { return nullptr; };
	CallResult readLine(void* buffer, uint maxBytes);
	int fgetc();
	CallResult read(void* buffer, size_t numBytes);
	size_t fread(void* ptr, size_t size, size_t nmemb);

	// writing
	virtual size_t fwrite(const void* ptr, size_t size, size_t nmemb) = 0;
	virtual void truncate(ulong offset) = 0;

	// file position
	virtual CallResult tell(ulong &offset) = 0;
	long ftell();

	// seeking
	virtual CallResult seek(long offset, uint mode) = 0;
	int fseek(long offset, int whence);

	// seeking shortcuts
	CallResult seekA(long offset) { return seek(offset, IO_SEEK_ABS); }
	CallResult seekAE(long offset) { return seek(offset, IO_SEEK_ABS_END); }
	CallResult seekR(long offset) { return seek(offset, IO_SEEK_REL); }
	CallResult seekB(long offset) { return seek(-offset, IO_SEEK_REL); }

	// other functions
	virtual void close() = 0;
	virtual void sync() = 0;
	virtual ulong size() = 0;
	virtual int eof() = 0;
	virtual void advise(long offset, size_t len, Advice advice) {}

	template <class T>
	CallResult readVar(T &var)
	{
		return read(&var, sizeof(T));
	}

	template <class T, class DEST_T>
	CallResult readVarAsType(DEST_T &var)
	{
		T v;
		auto ret = readVar(v);
		if(ret != OK)
			return ret;
		var = v;
		return OK;
	}

	template <class T>
	size_t writeVar(const T &var)
	{
		return fwrite(&var, sizeof(T), 1);
	}

	CallResult writeToIO(Io &io);
};

class IOFile
{
public:
	constexpr IOFile(Io *io): io_(io) {}
	~IOFile()
	{
		delete io_;
		io_ = nullptr;
	}

	operator bool() const
	{
		return io_;
	}

	Io *io()
	{
		return io_;
	}

	ssize_t readUpTo(void *buffer, size_t numBytes) { return io_->readUpTo(buffer, numBytes); }
	ssize_t readAtPos(void *buffer, size_t numBytes, ulong offset) { return io_->readAtPos(buffer, numBytes, offset); }
	const char *mmapConst() { return io_->mmapConst(); }
	CallResult readLine(void* buffer, uint maxBytes) { return io_->readLine(buffer, maxBytes); }
	int fgetc() { return io_->fgetc(); }
	CallResult read(void *buffer, size_t numBytes) { return io_->read(buffer, numBytes); }
	size_t fread(void *ptr, size_t size, size_t nmemb) { return io_->fread(ptr, size, nmemb); }
	size_t fwrite(const void* ptr, size_t size, size_t nmemb) { return io_->fwrite(ptr, size, nmemb); }
	void truncate(ulong offset) { io_->truncate(offset); }
	CallResult tell(ulong &offset) { return io_->tell(offset); }
	long ftell() { return io_->ftell(); }
	CallResult seek(long offset, uint mode) { return io_->seek(offset, mode); }
	CallResult seekA(long offset) { return io_->seekA(offset); }
	CallResult seekAE(long offset) { return io_->seekAE(offset); }
	CallResult seekR(long offset) { return io_->seekR(offset); }
	CallResult seekB(long offset) { return io_->seekB(offset); }
	int fseek(long offset, int whence) { return io_->fseek(offset, whence); }
	void close() { io_->close(); }
	void sync() { io_->sync(); }
	ulong size() { return io_->size(); }
	int eof() { return io_->eof(); }
	void advise(long offset, size_t len, Io::Advice advice) { return io_->advise(offset, len, advice); }

private:
	Io *io_;
};

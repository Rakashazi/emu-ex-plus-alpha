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

#include <engine-globals.h>
#include <limits.h>
#include <algorithm>

// allow writes, creates a new file if not present
#define IO_OPEN_WRITE BIT(0)
#define IO_OPEN_USED_BITS 1

// keep the existing file if present without overwriting it
#define IO_CREATE_KEEP BIT(0)
#define IO_CREATE_USED_BITS 1

#ifndef SEEK_SET
	#define SEEK_SET 0
	#define SEEK_CUR 1
	#define SEEK_END 2
#endif

enum { IO_SEEK_ABS, IO_SEEK_ABS_END, IO_SEEK_ADD, IO_SEEK_SUB };

class Io
{
public:
	constexpr Io() { }
	virtual ~Io() { }

	// reading
	virtual size_t readUpTo(void* buffer, size_t numBytes) = 0;
	virtual const uchar *mmapConst() { return 0; };
	virtual CallResult readLine(void* buffer, uint maxBytes);

	virtual int fgetc()
	{
		uchar byte;
		if(read(&byte, 1) == OK)
		{
			return byte;
		}
		else
			return EOF;
	}

	CallResult read(void* buffer, size_t numBytes)
	{
		if(readUpTo(buffer, numBytes) != numBytes)
			return IO_ERROR;
		else
			return OK;
	}

	virtual size_t fread(void* ptr, size_t size, size_t nmemb)
	{
		return readUpTo(ptr, (size * nmemb)) / size;
	}

	// writing
	virtual size_t fwrite(const void* ptr, size_t size, size_t nmemb) = 0;
	virtual void truncate(ulong offset) = 0;

	// file position
	virtual CallResult tell(ulong* offset) = 0;

	virtual long ftell()
	{
		ulong pos = 0;
		return tell(&pos) == OK ? (long)pos : -1;
	}

	// seeking
	virtual CallResult seekU(ulong offset, uint mode) = 0;
	virtual CallResult seekS(long offset, uint mode) = 0;

	CallResult seekAbs(ulong offset) { return seekU(offset, IO_SEEK_ABS); }
	CallResult seekAbsE(long offset) { return seekS(offset, IO_SEEK_ABS_END); }
	CallResult seekF(ulong offset) { return seekU(offset, IO_SEEK_ADD); }
	CallResult seekB(ulong offset) { return seekU(offset, IO_SEEK_SUB); }
	CallResult seekRel(long offset) { return seekS(offset, IO_SEEK_ADD); }
	CallResult seekA(ulong offset) { return seekAbs(offset); }
	CallResult seekR(long offset) { return seekRel(offset); }

	virtual int fseek(long offset, int whence)
	{
		// TODO: are we returning the correct error codes to simulate fseek?
		switch(whence)
		{
			case SEEK_SET: return seekAbs(offset) == OK ? 0 : -1;
			case SEEK_CUR: return seekRel(offset) == OK ? 0 : -1;
			case SEEK_END: return seekAbsE(offset) == OK ? 0 : -1; // io_seekAbsB subtracts the offset so we negate it since fseek expects to add it
		}
		return -1;
	}

	// other functions
	virtual void close() = 0;
	virtual void sync() = 0;
	virtual ulong size() = 0;
	virtual int eof() = 0;

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

	CallResult writeToIO(Io *io)
	{
		auto bytesToWrite = size();
		uchar buff[4096];
		while(bytesToWrite)
		{
			size_t bytes = std::min((ulong)sizeof(buff), bytesToWrite);
			CallResult ret = read(buff, bytes);
			if(ret != OK)
			{
				logErr("error reading from IO source with %d bytes to write", (int)bytesToWrite);
				return ret;
			}
			if(io->fwrite(buff, bytes, 1) != 1)
			{
				logErr("error writing to IO destination with %d bytes to write", (int)bytesToWrite);
				return IO_ERROR;
			}
			bytesToWrite -= bytes;
		}
		return OK;
	}
};

// Functions to wrap stdio functionality

static size_t io_fwrite(void* ptr, size_t size, size_t nmemb, Io *stream)
{
	return stream->fwrite(ptr, size, nmemb);
}

static size_t fread(void *data, size_t size, size_t count, Io *stream)
{
	return stream->fread(data, size, count);
}

static int fseek(Io *stream, long int offset, int whence)
{
	return stream->fseek(offset, whence);
}

static long int ftell(Io *stream)
{
	return stream->ftell();
}

static int fclose(Io *stream)
{
	delete stream;
	return 0;
}

static int io_ferror(Io *stream) // can't override ferror because it's a macro on some C libs
{
	return 0;
}

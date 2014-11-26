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

#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/io/api/stdio.hh>
#include <imagine/logger/logger.h>

IO::~IO() {}

ssize_t IO::readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut)
{
	auto savedOffset = tell();
	seekS(offset);
	auto bytesRead = read(buff, bytes, resultOut);
	seekS(savedOffset);
	return bytesRead;
}

GenericIO::GenericIO(GenericIO &&o)
{
	io = std::move(o.io);
}

GenericIO &GenericIO::operator=(GenericIO &&o)
{
	io = std::move(o.io);
	return *this;
}

ssize_t GenericIO::read(void *buff, size_t bytes, CallResult *resultOut)
{
	return io ? io->read(buff, bytes, resultOut) : (CallResult)BAD_STATE;
}

ssize_t GenericIO::readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut)
{
	return io ? io->readAtPos(buff, bytes, offset, resultOut) : (CallResult)BAD_STATE;
}

const char *GenericIO::mmapConst()
{
	return io ? io->mmapConst() : nullptr;
}

ssize_t GenericIO::write(const void *buff, size_t bytes, CallResult *resultOut)
{
	return io ? io->write(buff, bytes, resultOut) : (CallResult)BAD_STATE;
}

CallResult GenericIO::truncate(off_t offset)
{
	return io ? io->truncate(offset) : (CallResult)BAD_STATE;
}

off_t GenericIO::tell(CallResult *resultOut)
{
	if(io)
		return io->tell(resultOut);
	else
	{
		if(resultOut)
			*resultOut = BAD_STATE;
		return 0;
	}
}

CallResult GenericIO::seek(off_t offset, IO::SeekMode mode)
{
	return io ? io->seek(offset, mode) : (CallResult)BAD_STATE;
}

void GenericIO::close()
{
	if(io)
		io->close();
}

void GenericIO::sync()
{
	if(io)
		io->sync();
}

size_t GenericIO::size()
{
	return io ? io->size() : 0;
}

bool GenericIO::eof()
{
	return io ? io->eof() : false;
}

void GenericIO::advise(off_t offset, size_t bytes, IO::Advice advice)
{
	if(io)
		io->advise(offset, bytes, advice);
}

GenericIO::operator bool()
{
	return io && *io;
}

AssetIO openAppAssetIO(const char *name)
{
	AssetIO io;
	#ifdef __ANDROID__
	io.open(name);
	#else
	io.open(makeFSPathStringPrintf("%s/%s", Base::assetPath(), name).data());
	#endif
	return io;
}

CallResult writeToNewFile(const char *path, void *data, size_t size)
{
	FileIO f;
	auto r = f.create(path);
	if(!f)
		return r;
	if(f.writeAll(data, size) != OK)
		return WRITE_ERROR;
	return OK;
}

ssize_t readFromFile(const char *path, void *data, size_t size)
{
	FileIO f;
	f.open(path);
	if(!f)
		return -1;
	auto readSize = f.read(data, size);
	return readSize;
}

CallResult writeIOToNewFile(IO &io, const char *path)
{
	FileIO file;
	auto r = file.create(path);
	if(r != OK)
		return r;
	return io.writeToIO(file);
}

int fgetc(IO &io)
{
	char byte;
	return io.read(&byte, 1) == 1 ? byte : EOF;
}

size_t fread(void *ptr, size_t size, size_t nmemb, IO &io)
{
	auto bytesRead = io.read(ptr, (size * nmemb));
	return bytesRead > 0 ? bytesRead / size : 0;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, IO &io)
{
	auto bytesWritten = io.write(ptr, (size * nmemb));
	return bytesWritten > 0 ? bytesWritten / size : 0;
}

long ftell(IO &io)
{
	return io.tell();
}

int fseek(IO &io, long offset, int whence)
{
	return io.seek(offset, (IODefs::SeekMode)whence);
}

int fclose(IO &stream)
{
	stream.close();
	return 0;
}

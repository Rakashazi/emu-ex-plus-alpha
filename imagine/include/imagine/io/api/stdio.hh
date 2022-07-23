#pragma once
#include <imagine/io/ioDefs.hh>
#include <cstdio>

// Functions to wrap basic stdio functionality

namespace IG
{

int fgetc(Readable auto &io)
{
	char byte;
	return io.read(&byte, 1) == 1 ? byte : EOF;
}

size_t fread(void *ptr, size_t size, size_t nmemb, Readable auto &io)
{
	auto bytesRead = io.read(ptr, (size * nmemb));
	return bytesRead > 0 ? bytesRead / size : 0;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, Writable auto &io)
{
	auto bytesWritten = io.write(ptr, (size * nmemb));
	return bytesWritten > 0 ? bytesWritten / size : 0;
}

long ftell(Seekable auto &io)
{
	return io.tell();
}

int fseek(Seekable auto &io, long offset, int whence)
{
	return io.seek(offset, (IOSeekMode)whence) == -1 ? -1 : 0;
}

int fclose(auto &stream)
{
	return 0;
}

}

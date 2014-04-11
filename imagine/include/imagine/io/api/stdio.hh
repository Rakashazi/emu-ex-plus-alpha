#pragma once
#include <imagine/io/Io.hh>

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

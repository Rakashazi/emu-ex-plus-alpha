#pragma once

// include the Vorbis or Tremor header before this file

#include <imagine/io/api/stdio.hh>

namespace IOAPI
{

	namespace Vorbis
	{

	static size_t read(void *ptr, size_t size, size_t nmemb, void *datasource)
	{
		return fread(ptr, size, nmemb, *(IO*)datasource);
	}

	static int seek(void *datasource, ogg_int64_t offset, int whence)
	{
		return fseek(*(IO*)datasource, offset, whence);
	}

	static int close(void *datasource)
	{
		return fclose(*(IO*)datasource);
	}

	static long tell(void *datasource)
	{
		return ftell(*(IO*)datasource);
	}

	}

static ov_callbacks vorbis
{
	Vorbis::read, Vorbis::seek, Vorbis::close, Vorbis::tell
};

static ov_callbacks vorbisNoClose
{
	Vorbis::read, Vorbis::seek, nullptr, Vorbis::tell
};

}

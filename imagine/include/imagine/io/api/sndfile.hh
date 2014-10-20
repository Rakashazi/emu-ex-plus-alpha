#pragma once
#include <sndfile.h>
#include <imagine/io/IO.hh>

namespace IOAPI
{

static SF_VIRTUAL_IO sndfile
{
	[](void *user_data) // get_filelen
	{
		return (sf_count_t)((IO*)user_data)->size();
	},
	[](sf_count_t offset, int whence, void *user_data) // seek
	{
		auto &io = *(IO*)user_data;
		io.seek(offset, (IO::SeekMode)whence);
		return (sf_count_t)io.tell();
	},
	[](void *ptr, sf_count_t count, void *user_data) // read
	{
		return (sf_count_t)((IO*)user_data)->read(ptr, count);
	},
	[](const void *ptr, sf_count_t count, void *user_data) // write
	{
		return (sf_count_t)((IO*)user_data)->write(ptr, count);
	},
	[](void *user_data) // tell
	{
		return (sf_count_t)((IO*)user_data)->tell();
	}
};

}

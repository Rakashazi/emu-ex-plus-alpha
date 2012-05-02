#pragma once

namespace SndFileIOAPI
{
	static sf_count_t get_filelen(void *user_data)
	{
		return ((Io*)user_data)->size();
	}

	static sf_count_t seek(sf_count_t offset, int whence, void *user_data)
	{
		fseek((Io*)user_data, offset, whence);
		return ftell((Io*)user_data);
	}

	static sf_count_t read(void *ptr, sf_count_t count, void *user_data)
	{
		return ((Io*)user_data)->readUpTo(ptr, count);
	}

	static sf_count_t write(const void *ptr, sf_count_t count, void *user_data)
	{
		return 0; // not implemented
	}

	static sf_count_t tell(void *user_data)
	{
		return ftell((Io*)user_data);
	}
}

static SF_VIRTUAL_IO imagineSndFileIO =
{
		SndFileIOAPI::get_filelen, SndFileIOAPI::seek, SndFileIOAPI::read,
		SndFileIOAPI::write, SndFileIOAPI::tell
};

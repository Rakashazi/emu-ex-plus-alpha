#pragma once

#include <imagine/io/IO.hh>

static bool isSeekModeValid(IO::SeekMode mode)
{
	switch(mode)
	{
		case SEEK_SET:
		case SEEK_END:
		case SEEK_CUR:
			return true;
		default:
			return false;
	}
}

static off_t transformOffsetToAbsolute(IO::SeekMode mode, off_t offset, off_t startPos, off_t endPos, off_t currentPos)
{
	switch(mode)
	{
		case SEEK_SET:
			return offset + startPos;
		case SEEK_END:
			return offset + endPos;
		case SEEK_CUR:
			return offset + currentPos;
	}
	bug_exit("bad offset mode: %d", (int)mode);
	return 0;
}

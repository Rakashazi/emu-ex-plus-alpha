#pragma once

#include <imagine/io/Io.hh>

static bool transformOffsetToAbsolute(uint mode, long &newOffset, long startOffset, long endOffset, long currentOffset)
{
	switch(mode)
	{
		case IO_SEEK_ABS:
			newOffset += startOffset;
			return true;
		case IO_SEEK_ABS_END:
			newOffset += endOffset;
			return true;
		case IO_SEEK_REL:
			newOffset += currentOffset;
			return true;
	}
	return false;
}

static bool isSeekModeValid(uint mode)
{
	switch(mode)
	{
		case IO_SEEK_ABS:
		case IO_SEEK_ABS_END:
		case IO_SEEK_REL:
			return true;
		default:
			return false;
	}
}

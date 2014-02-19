#pragma once

#include <io/Io.hh>

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

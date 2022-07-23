#pragma once

#include <imagine/io/ioDefs.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>

namespace IG
{

inline auto transformOffsetToAbsolute(IOSeekMode mode, auto offset, auto startPos, auto endPos, auto currentPos)
{
	switch(mode)
	{
		case IOSeekMode::SET:
			return offset + startPos;
		case IOSeekMode::END:
			return offset + endPos;
		case IOSeekMode::CUR:
			return offset + currentPos;
		default:
			bug_unreachable("IOSeekMode == %d", (int)mode);
	}
}

inline const char *accessHintStr(IOAccessHint access)
{
	switch(access)
	{
		case IOAccessHint::NORMAL: return "Normal";
		case IOAccessHint::SEQUENTIAL: return "Sequential";
		case IOAccessHint::RANDOM: return "Random";
		case IOAccessHint::ALL: return "All";
		default:
			bug_unreachable("IOAccessHint == %d", (int)access);
	}
}

inline const char *adviceStr(IOAdvice advice)
{
	switch(advice)
	{
		case IOAdvice::NORMAL: return "Normal";
		case IOAdvice::SEQUENTIAL: return "Sequential";
		case IOAdvice::RANDOM: return "Random";
		case IOAdvice::WILLNEED: return "Will Need";
		default:
			bug_unreachable("IOAdvice == %d", (int)advice);
	}
}

}

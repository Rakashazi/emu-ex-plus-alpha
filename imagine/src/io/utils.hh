#pragma once

#include <imagine/io/IO.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>

namespace IG
{

inline auto transformOffsetToAbsolute(IO::SeekMode mode, auto offset, auto startPos, auto endPos, auto currentPos)
{
	switch(mode)
	{
		case IO::SeekMode::SET:
			return offset + startPos;
		case IO::SeekMode::END:
			return offset + endPos;
		case IO::SeekMode::CUR:
			return offset + currentPos;
		default:
			bug_unreachable("IO::SeekMode == %d", (int)mode);
			return decltype(offset + startPos){};
	}
}

inline const char *accessHintStr(IO::AccessHint access)
{
	switch(access)
	{
		case IO::AccessHint::NORMAL: return "Normal";
		case IO::AccessHint::SEQUENTIAL: return "Sequential";
		case IO::AccessHint::RANDOM: return "Random";
		case IO::AccessHint::ALL: return "All";
		default:
			bug_unreachable("IO::AccessHint == %d", (int)access);
			return "";
	}
}

inline const char *adviceStr(IO::Advice advice)
{
	switch(advice)
	{
		case IO::Advice::NORMAL: return "Normal";
		case IO::Advice::SEQUENTIAL: return "Sequential";
		case IO::Advice::RANDOM: return "Random";
		case IO::Advice::WILLNEED: return "Will Need";
		default:
			bug_unreachable("IO::Advice == %d", (int)advice);
			return "";
	}
}

}

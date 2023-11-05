#pragma once

#include <imagine/io/ioDefs.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>

namespace IG
{

inline auto asString(IOAccessHint access)
{
	switch(access)
	{
		case IOAccessHint::Normal: return "Normal";
		case IOAccessHint::Sequential: return "Sequential";
		case IOAccessHint::Random: return "Random";
		case IOAccessHint::All: return "All";
	}
	bug_unreachable("IOAccessHint == %d", (int)access);
}

inline auto asString(IOAdvice advice)
{
	switch(advice)
	{
		case IOAdvice::Normal: return "Normal";
		case IOAdvice::Sequential: return "Sequential";
		case IOAdvice::Random: return "Random";
		case IOAdvice::WillNeed: return "Will Need";
	}
	bug_unreachable("IOAdvice == %d", (int)advice);
}

}

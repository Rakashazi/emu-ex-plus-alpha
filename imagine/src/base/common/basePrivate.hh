#pragma once

#include <imagine/base/Base.hh>

namespace Base
{

extern InterProcessMessageDelegate onInterProcessMessage;
extern ResumeDelegate onResume;
extern FreeCachesDelegate onFreeCaches;
extern ExitDelegate onExit;

[[gnu::cold]] void engineInit();

}

#pragma once

#include <imagine/base/Base.hh>

namespace Base
{

enum class ActivityState : uint8_t
{
	PAUSED,
	RUNNING,
	EXITING
};

[[gnu::cold]] void engineInit();
ActivityState activityState();
void setPausedActivityState();
void setRunningActivityState();
void setExitingActivityState();
bool appIsPaused();
bool appIsExiting();

}

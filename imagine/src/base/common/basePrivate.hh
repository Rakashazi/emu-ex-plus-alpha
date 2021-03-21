#pragma once

#include <cstdint>

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

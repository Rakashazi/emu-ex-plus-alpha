#pragma once
#include <util/collection/DLList.hh>
#include <base/Base.hh>
#include <util/time/sys.hh>

class PollWaitTimer
{
public:
	Base::CallbackDelegate callback;
	TimeSys targetTime;
	static DLList<PollWaitTimer>::Node timerListNode[4];
	static DLList<PollWaitTimer> timerList;

	constexpr PollWaitTimer() { }
	constexpr PollWaitTimer(Base::CallbackDelegate callback): callback(callback) { }

	bool operator ==(PollWaitTimer const& rhs) const
	{
		return callback == rhs.callback;
	}

	void remove()
	{
		logMsg("removing callback");
		timerList.remove(*this);
	}

	int add(int ms)
	{
		logMsg("setting callback to run in %d ms", ms);
		TimeSys callTime;
		callTime.setTimeNow();
		callTime.addUSec(ms * 1000);
		targetTime = callTime;
		return timerList.add(*this);
	}

	int calcPollWaitForFunc() const
	{
		TimeSys now;
		now.setTimeNow();
		if(now >= targetTime)
		{
			return 0;
		}
		return (targetTime - now).toMs() + 1;
	}

	static bool hasCallbacks()
	{
		return timerList.size;
	}

	static PollWaitTimer *getNextCallback()
	{
		TimeSys closestTime;
		PollWaitTimer *closest = 0;
		forEachInDLList(&timerList, e)
		{
			if(e.targetTime > closestTime)
			{
				closestTime = e.targetTime;
				closest = &e;
			}
		}
		return closest;
	}

	static void processCallbacks()
	{
		if(timerList.size)
		{
			TimeSys now;
			now.setTimeNow();

			// check the callbacks list for target times that have passed
			// TODO: sort list by targetTime so only part of the list may be checked
			forEachInDLList(&timerList, e)
			{
				if(now >= e.targetTime)
				{
					logMsg("running callback");
					e.callback.invoke();
					e_it.removeElem();
				}
			}
		}
	}
};

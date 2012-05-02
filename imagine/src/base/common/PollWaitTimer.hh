#pragma once
#include <util/collection/DLList.hh>
#include <base/Base.hh>
#include <util/time/sys.hh>

class PollWaitTimer
{
public:
	Base::TimerCallbackFunc func;
	void *funcCtx;
	TimeSys targetTime;
	static DLList<PollWaitTimer*>::Node timerListNode[4];
	static DLList<PollWaitTimer*> timerList;

	void init()
	{

	}

	void init(Base::TimerCallbackFunc f, void *ctx, int ms)
	{
		 setCallback(f, ctx, ms);
	}

	void remove()
	{
		logMsg("removing callback");
		func = 0;
		timerList.remove(this);
	}

	void setCallback(Base::TimerCallbackFunc f, void *ctx, int ms)
	{
		if(!f)
		{
			remove();
			return;
		}
		logMsg("setting callback to run in %d ms", ms);
		func = f;
		funcCtx = ctx;
		TimeSys callTime;
		callTime.setTimeNow();
		callTime.addUSec(ms * 1000);
		targetTime = callTime;
		timerList.add(this);
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
		closestTime.setUSecs(0);
		PollWaitTimer *closest = 0;
		forEachInDLList(&timerList, e)
		{
			if(e->targetTime > closestTime)
			{
				closestTime = e->targetTime;
				closest = e;
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
				if(now >= e->targetTime)
				{
					logMsg("running callback %p", e->func);
					e->func(e->funcCtx);
					e_it.removeElem();
				}
			}
		}
	}
};

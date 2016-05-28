/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

package com.imagine;

import android.app.*;
import android.util.*;

final class UserActivityFaker
{
	private static final String logTag = "UserActivityFaker";
	private Instrumentation inst = new Instrumentation();

	private final class FakeInputThread extends Thread
	{
		@Override public void run()
		{
			//Log.i(logTag, "starting fake inputs");
			for(;;)
			{
				inst.sendKeyDownUpSync(0);
				//Log.i(logTag, "sent fake input");
				try
				{
					sleep(1000);
				}
				catch(InterruptedException e)
				{
					//Log.i(logTag, "got interrupt");
					return;
				}
			}
		}
	}
	
	private FakeInputThread thread;

	public void start()
	{
		if(thread != null)
			return;
		thread = new FakeInputThread();
		thread.setPriority(Thread.MIN_PRIORITY);
		thread.start();
	}
	
	public void stop()
	{
		if(thread == null)
			return;
		thread.interrupt();
		thread = null;
	}
}

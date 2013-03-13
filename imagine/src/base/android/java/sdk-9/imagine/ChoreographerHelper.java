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

import android.widget.*;
import android.app.*;
import android.content.*;
import android.os.*;
import android.view.*;
import android.util.*;

final class ChoreographerHelper implements Choreographer.FrameCallback
{
	private static String logTag = "ChoreographerHelper";
	private Choreographer choreographer;
	private BaseActivity activity;
	
	public static void checkAvailable() {}
	
	public ChoreographerHelper(BaseActivity act)
	{
		choreographer = Choreographer.getInstance();
		activity = act;
	}

	void postDrawWindow()
	{
		choreographer.postFrameCallback(this);
	}
	
	void cancelDrawWindow()
	{
		choreographer.removeFrameCallback(this);
	}

	@Override public void doFrame(long frameTimeNanos)
	{
		choreographer.postFrameCallback(this);
		if(!activity.drawWindow(frameTimeNanos))
		{
			choreographer.removeFrameCallback(this);
		}
	}
}

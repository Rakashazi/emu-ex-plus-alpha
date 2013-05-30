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

final class ChoreographerHelper
{
	private final class Callback implements Choreographer.FrameCallback
	{
		@Override public void doFrame(long frameTimeNanos)
		{
			choreographer.postFrameCallback(this);
			if(!drawWindow(frameTimeNanos))
			{
				choreographer.removeFrameCallback(this);
			}
		}
	}

	private static final String logTag = "ChoreographerHelper";
	private native boolean drawWindow(long frameTimeNanos);
	private final Choreographer choreographer = Choreographer.getInstance();
	private final Callback callback = new Callback();

	void postDrawWindow()
	{
		choreographer.postFrameCallback(callback);
	}
	
	void cancelDrawWindow()
	{
		choreographer.removeFrameCallback(callback);
	}
}

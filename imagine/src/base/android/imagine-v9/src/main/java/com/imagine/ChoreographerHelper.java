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

import android.view.Choreographer;
import android.os.Looper;
import android.util.Log;

final class ChoreographerHelper
{
	private final class Callback implements Choreographer.FrameCallback
	{
		private long nativeUserData;

		Callback(long nativeUserData)
		{
			this.nativeUserData = nativeUserData;
		}

		@Override public void doFrame(long frameTimeNanos)
		{
			onFrame(nativeUserData, frameTimeNanos);
		}
	}

	private static final String logTag = "ChoreographerHelper";
	private static native void onFrame(long nativeUserData, long frameTimeNanos);
	private Choreographer choreographer = Choreographer.getInstance();
	private final Callback callback;

	ChoreographerHelper(long nativeUserData)
	{
		callback = new Callback(nativeUserData);
	}

	void postFrame()
	{
		choreographer.postFrameCallback(callback);
	}

	void setInstance()
	{
		if(Looper.myLooper() == null)
			Looper.prepare();
		choreographer = Choreographer.getInstance();
	}
}

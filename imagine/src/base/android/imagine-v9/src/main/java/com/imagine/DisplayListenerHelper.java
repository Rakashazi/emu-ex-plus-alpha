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
import android.content.*;
import android.os.*;
import android.util.*;
import android.view.*;
import android.hardware.display.*;

final class DisplayListenerHelper
{
	private static final String logTag = "DisplayListenerHelper";
	private DisplayManager displayManager;
	private native void displayAdd(int id, Display dpy, float refreshRate, DisplayMetrics metrics);
	private native void displayChange(int id, float refreshRate);
	private native void displayRemove(int id);
	private final class Listener implements DisplayManager.DisplayListener
	{
		@Override public void onDisplayAdded(int deviceId)
		{
			//Log.i(logTag, "added id: " + deviceId);
			Display dpy = displayManager.getDisplay(deviceId);
			if(dpy == null)
				return;
			if(!shouldHandleAddedDisplay(deviceId, dpy))
			{
				//Log.i(logTag, "skipped adding display with id: " + deviceId);
				return;
			}
			displayAdd(deviceId, dpy, dpy.getRefreshRate(), displayMetrics(dpy));
		}

		@Override public void onDisplayChanged(int deviceId)
		{
			//Log.i(logTag, "changed id: " + deviceId);
			Display dpy = displayManager.getDisplay(deviceId);
			if(dpy == null)
				return;
			if(!shouldHandleChangedDisplay(deviceId, dpy))
			{
				//Log.i(logTag, "skipped changed display with id: " + deviceId);
				return;
			}
			displayChange(deviceId, dpy.getRefreshRate());
		}

		@Override public void onDisplayRemoved(int deviceId)
		{
			//Log.i(logTag, "removed id: " + deviceId);
			displayRemove(deviceId);
		}
	}
	Listener listener = new Listener();
	
	DisplayListenerHelper(Activity act)
	{
		//Log.i(logTag, "registering input device listener");
		displayManager = (DisplayManager)act.getSystemService(Context.DISPLAY_SERVICE);
	}

	static void enumPresentationDisplays(BaseActivity act)
	{
		DisplayManager displayManager = (DisplayManager)act.getSystemService(Context.DISPLAY_SERVICE);
		for(Display dpy : displayManager.getDisplays(DisplayManager.DISPLAY_CATEGORY_PRESENTATION))
		{
			act.displayEnumerated(dpy, dpy.getDisplayId(), dpy.getRefreshRate(),
				Surface.ROTATION_0, displayMetrics(dpy));
		}
	}

	void setListener(boolean on)
	{
		if(on)
		{
			displayManager.registerDisplayListener(listener, null);
		}
		else
		{
			displayManager.unregisterDisplayListener(listener);
		}
	}

	static boolean shouldHandleAddedDisplay(int deviceId, Display dpy)
	{
		return (dpy.getFlags() & Display.FLAG_PRESENTATION) == Display.FLAG_PRESENTATION;
	}

	static boolean shouldHandleChangedDisplay(int deviceId, Display dpy)
	{
		return deviceId == Display.DEFAULT_DISPLAY ||
			(dpy.getFlags() & Display.FLAG_PRESENTATION) == Display.FLAG_PRESENTATION;
	}

	private static DisplayMetrics displayMetrics(Display dpy)
	{
		DisplayMetrics metrics = new DisplayMetrics();
		dpy.getMetrics(metrics);
		return metrics;
	}
}

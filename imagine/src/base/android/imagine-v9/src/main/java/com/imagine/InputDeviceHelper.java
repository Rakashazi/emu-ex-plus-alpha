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

import android.util.*;
import android.view.InputDevice;

final class InputDeviceHelper
{
	private static final String logTag = "InputDeviceHelper";
	private static native void deviceEnumerated(int devID,
		InputDevice dev, String name, int src, int kbType);
	
	InputDeviceHelper() {}
	
	static boolean shouldHandleDevice(InputDevice dev)
	{
		if(dev == null)
		{
			return false;
		}
		String name = dev.getName();
		if(name == null || name.length() == 0)
		{
			//Log.e(logTag, "no name from device id:" + id);
			return false;
		}
		int src = dev.getSources();
		int hasKeys = src & InputDevice.SOURCE_CLASS_BUTTON;
		if(hasKeys == 0)
		{
			return false;
		}
		// skip various devices that don't have useful functions
		if(name.contains("pwrkey") || name.contains("pwrbutton")) // various power keys
		{
			return false;
		}
		return true;
	}

	public static void enumInputDevices()
	{
		int[] idArr = InputDevice.getDeviceIds();
		for(int id : idArr)
		{
			InputDevice dev = InputDevice.getDevice(id);
			if(shouldHandleDevice(dev))
			{
				deviceEnumerated(id, dev, dev.getName(), dev.getSources(), dev.getKeyboardType());
			}
		}
	}
}

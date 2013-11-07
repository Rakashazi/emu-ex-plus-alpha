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
import android.hardware.input.*;

final class InputDeviceListenerHelper
{
	private static final String logTag = "InputDeviceListenerHelper";
	private native void deviceChange(int id, int change);
	private final class Listener implements InputManager.InputDeviceListener
	{
		@Override public void onInputDeviceAdded(int deviceId)
		{
			//Log.i(logTag, "added id: " + deviceId);
			deviceChange(deviceId, 0);
		}

		@Override public void onInputDeviceChanged(int deviceId)
		{
			//Log.i(logTag, "changed id: " + deviceId);
			deviceChange(deviceId, 1);
		}
		
		@Override public void onInputDeviceRemoved(int deviceId)
		{
			//Log.i(logTag, "removed id: " + deviceId);
			deviceChange(deviceId, 2);
		}
	}
	
	InputDeviceListenerHelper(Activity act)
	{
		//Log.i(logTag, "registering input device listener");
		InputManager inputManager = (InputManager)act.getApplicationContext().getSystemService(Context.INPUT_SERVICE);
		inputManager.registerInputDeviceListener(new Listener(), null);
	}
}

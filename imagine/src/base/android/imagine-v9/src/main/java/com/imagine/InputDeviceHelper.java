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
import android.view.MotionEvent;

final class InputDeviceHelper
{
	private static final String logTag = "InputDeviceHelper";
	
	static boolean shouldHandleDevice(InputDevice dev)
	{
		if(dev == null)
		{
			return false;
		}
		if(dev.getName() == null)
		{
			//Log.e(logTag, "no name from device id:" + id);
			return false;
		}
		return true;
	}
	
	static boolean isPowerButtonName(String name)
	{
		if(name.contains("pwrkey") || name.contains("pwrbutton"))
		{
			return true;
		}
		return false;
	}
	
	static int axisBits(InputDevice dev)
	{
		if((dev.getSources() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK)
		{
			return 0;
		}
		final int AXIS_BIT_X = 1 << 0, AXIS_BIT_Y = 1 << 1, AXIS_BIT_Z = 1 << 2,
			AXIS_BIT_RX = 1 << 3, AXIS_BIT_RY = 1 << 4, AXIS_BIT_RZ = 1 << 5,
			AXIS_BIT_HAT_X = 1 << 6, AXIS_BIT_HAT_Y = 1 << 7,
			AXIS_BIT_LTRIGGER = 1 << 8, AXIS_BIT_RTRIGGER = 1 << 9,
			AXIS_BIT_RUDDER = 1 << 10, AXIS_BIT_WHEEL = 1 << 11,
			AXIS_BIT_GAS = 1 << 12, AXIS_BIT_BRAKE = 1 << 13;
		int bits = 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_X) != null) ? AXIS_BIT_X : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_Y) != null) ? AXIS_BIT_Y : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_Z) != null) ? AXIS_BIT_Z : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_RX) != null) ? AXIS_BIT_RX : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_RY) != null) ? AXIS_BIT_RY : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_RZ) != null) ? AXIS_BIT_RZ : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_HAT_X) != null) ? AXIS_BIT_HAT_X : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_HAT_Y) != null) ? AXIS_BIT_HAT_Y : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_RUDDER) != null) ? AXIS_BIT_RUDDER : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_WHEEL) != null) ? AXIS_BIT_WHEEL : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_LTRIGGER) != null) ? AXIS_BIT_LTRIGGER : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_RTRIGGER) != null) ? AXIS_BIT_RTRIGGER : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_GAS) != null) ? AXIS_BIT_GAS : 0;
		bits |= (dev.getMotionRange(MotionEvent.AXIS_BRAKE) != null) ? AXIS_BIT_BRAKE : 0;
		return bits;
	}

	static int vendorProductId(InputDevice dev)
	{
		int vendorProductId = 0;
		if(android.os.Build.VERSION.SDK_INT >= 19)
		{
			Log.e(logTag, dev.getName() + " vendor:" + dev.getVendorId() + " product:" + dev.getProductId());
			vendorProductId = ((dev.getVendorId() & 0xFFFF) << 16) | (dev.getProductId() & 0xFFFF);
		}
		return vendorProductId;
	}

	static void enumInputDevices(BaseActivity act, long nativeUserData)
	{
		int[] idArr = InputDevice.getDeviceIds();
		for(int id : idArr)
		{
			InputDevice dev = InputDevice.getDevice(id);
			if(!shouldHandleDevice(dev))
				continue;
			act.inputDeviceEnumerated(nativeUserData, id, dev, dev.getName(), dev.getSources(), dev.getKeyboardType(),
				axisBits(dev), vendorProductId(dev), isPowerButtonName(dev.getName()));
		}
	}
}

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

import android.os.Handler;
import android.content.Context;
import com.bda.controller.*;

final class MOGAHelper implements ControllerListener
{
	private static final String logTag = "MOGAHelper";
	Controller controller;
	private long nativeUserData;
	private native void keyEvent(long nativeUserData, int action, int keyCode, long time);
	private native void motionEvent(long nativeUserData, float axisX, float axisY, float axisZ, float axisRZ,
		float axisLTrigger, float axisRTrigger, long time);
	private native void stateEvent(long nativeUserData, int state, int action);

	MOGAHelper(Context context, long nativeUserData)
	{
		this.nativeUserData = nativeUserData;
		controller = Controller.getInstance(context);
		controller.init();
		controller.setListener(this, new Handler());
	}

	@Override public void onKeyEvent(KeyEvent event)
	{
		keyEvent(nativeUserData, event.getAction(), event.getKeyCode(), event.getEventTime());
	}

	@Override public void onMotionEvent(MotionEvent event)
	{
		motionEvent(nativeUserData,
			event.getAxisValue(Controller.AXIS_X), event.getAxisValue(Controller.AXIS_Y),
			event.getAxisValue(Controller.AXIS_Z), event.getAxisValue(Controller.AXIS_RZ),
			event.getAxisValue(Controller.AXIS_LTRIGGER), event.getAxisValue(Controller.AXIS_RTRIGGER),
			event.getEventTime());
	}

	@Override public void onStateEvent(StateEvent event)
	{
		stateEvent(nativeUserData, event.getState(), event.getAction());
	}

	int getState(int state)
	{
		return controller.getState(state);
	}

	void onPause()
	{
		controller.onPause();
	}

	void onResume()
	{
		controller.onResume();
	}

	void exit()
	{
		controller.exit();
		controller = null;
	}
}

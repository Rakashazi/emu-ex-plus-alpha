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

// API Level 5, Android 2.0

package com.imagine;

import android.os.*;
import android.view.*;
import android.content.res.Configuration;

final class SDKCompat
{
	public static boolean onTouchEvent(final MotionEvent event)
	{
		return SDK5.onTouchEvent(event);
	}
	
	public static int getNavigationHidden(Configuration config)
	{
		return SDK5.getNavigationHidden(config);
	}
}

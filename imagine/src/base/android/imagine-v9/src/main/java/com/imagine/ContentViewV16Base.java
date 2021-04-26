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

import android.view.View;
import android.graphics.Rect;
import android.content.Context;
import android.app.Activity;
import android.util.Log;

class ContentViewV16Base extends View
{
	protected static final String logTag = "ContentView";
	protected long nativeUserData;
	protected Rect contentRect = new Rect();
	protected Rect newContentRect = new Rect();
	protected int windowWidth, windowHeight;

	public ContentViewV16Base(Context context, long nativeUserData)
	{
		super(context);
		this.nativeUserData = nativeUserData;
	}

	void resetNativeUserData()
	{
		nativeUserData = 0;
	}
}

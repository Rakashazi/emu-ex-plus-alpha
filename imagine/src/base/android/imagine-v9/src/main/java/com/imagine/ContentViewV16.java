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

//For API level >= 16, SYSTEM_UI_FLAG_LAYOUT_* always gives us a full screen view rectangle,
// so use fitSystemWindows to get the area not overlapping the system windows
final class ContentViewV16 extends ContentViewV16Base
{
	public ContentViewV16(Context context, long nativeUserData)
	{
		super(context, nativeUserData);
	}
	
	@Override protected boolean fitSystemWindows(Rect insets)
	{
		if(getWidth() == 0 || getHeight() == 0)
			return true;
		View rootView = getRootView();
		int newWindowWidth = rootView.getWidth();
		int newWindowHeight = rootView.getHeight();
		//Log.i(logTag, "system window insets: " + insets.left + "," + insets.top + " " + insets.right + "," + insets.bottom);
		// adjust insets to become content rect
		newContentRect.left = 0;
		newContentRect.right = getWidth();
		newContentRect.top = insets.top;
		newContentRect.bottom = getHeight();
		int visFlags = getWindowSystemUiVisibility();
		boolean applyNavInsets = ((visFlags & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) ? true : false;
		if(applyNavInsets)
		{
			newContentRect.left += insets.left;
			newContentRect.right -= insets.right;
			newContentRect.bottom -= insets.bottom;
		}
		if(nativeUserData != 0 && (!contentRect.equals(newContentRect) || newWindowWidth != windowWidth || newWindowHeight != windowHeight))
		{
			//Log.i(logTag, "content rect: " + contentRect.left + "," + contentRect.top + " " + contentRect.right + "," + contentRect.bottom
			//		+ " -> " + newContentRect.left + "," + newContentRect.top + " " + newContentRect.right + "," + newContentRect.bottom);
			BaseActivity.onContentRectChanged(nativeUserData,
				newContentRect.left, newContentRect.top, newContentRect.right, newContentRect.bottom,
				newWindowWidth, newWindowHeight);
			contentRect.set(newContentRect);
			windowWidth = newWindowWidth;
			windowHeight = newWindowHeight;
		}
		return true;
	}
	
	@Override protected void onLayout(boolean changed, int left, int top, int right, int bottom)
	{
		//Log.i(logTag, "onLayout called: " + left + ":" + top + ":" + right + ":" + bottom);
		requestFitSystemWindows();
	}
}

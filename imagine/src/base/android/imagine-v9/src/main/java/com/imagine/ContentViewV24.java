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
import android.view.WindowInsets;
import android.view.DisplayCutout;
import android.graphics.Rect;
import android.content.Context;
import android.app.Activity;
import android.util.Log;

final class ContentViewV24 extends ContentViewV16Base
{
	public ContentViewV24(Context context, long nativeUserData)
	{
		super(context, nativeUserData);
	}

	@Override public WindowInsets onApplyWindowInsets(WindowInsets insets)
	{
		if(getWidth() == 0 || getHeight() == 0)
			return insets;
		View rootView = getRootView();
		int newWindowWidth = rootView.getWidth();
		int newWindowHeight = rootView.getHeight();
		WindowInsets rootInsets = getRootWindowInsets();
		//Log.i(logTag, "system window insets: " + rootInsets.left + "," + rootInsets.top + " " + rootInsets.right + "," + rootInsets.bottom);
		// adjust insets to become content rect
		newContentRect.left = 0;
		newContentRect.right = newWindowWidth;
		newContentRect.top = rootInsets.getSystemWindowInsetTop();
		newContentRect.bottom = newWindowHeight;
		Activity act = (Activity)getContext();
		if(act.isInMultiWindowMode())
		{
			// always account for status bar in multi-window mode
			newContentRect.top = rootInsets.getStableInsetTop();
		}
		int visFlags = getWindowSystemUiVisibility();
		boolean applyNavInsets = act.isInMultiWindowMode()
			|| ((visFlags & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) ? true : false;
		if(applyNavInsets)
		{
			newContentRect.left += rootInsets.getSystemWindowInsetLeft();
			newContentRect.right -= rootInsets.getSystemWindowInsetRight();
			newContentRect.bottom -= rootInsets.getSystemWindowInsetBottom();
		}
		else if(android.os.Build.VERSION.SDK_INT > 28)
		{
			DisplayCutout cutout = rootInsets.getDisplayCutout();
			if(cutout != null)
			{
				newContentRect.left += cutout.getSafeInsetLeft();
				newContentRect.right -= cutout.getSafeInsetRight();
				newContentRect.bottom -= cutout.getSafeInsetBottom();
			}
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
		return insets;
	}
	
	@Override protected void onLayout(boolean changed, int left, int top, int right, int bottom)
	{
		//Log.i(logTag, "onLayout called: " + left + ":" + top + ":" + right + ":" + bottom);
		requestApplyInsets();
	}
}

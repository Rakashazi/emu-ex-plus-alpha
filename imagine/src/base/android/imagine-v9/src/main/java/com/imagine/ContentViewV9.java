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
import android.util.Log;

//For API level <= 9, FLAG_LAYOUT_INSET_DECOR will adjust the view rectangle to not overlap the system windows
// For API level >= 10 and <= 15, use getGlobalVisibleRect since fitSystemWindows isn't called
final class ContentViewV9 extends View
{
	private static final String logTag = "ContentView";
	private long nativeUserData;
	private Rect contentRect = new Rect();
	private Rect globalRect = new Rect(); // preallocate
	private int windowWidth, windowHeight;

	public ContentViewV9(Context context, long nativeUserData)
	{
		super(context);
		this.nativeUserData = nativeUserData;
	}

	@Override protected void onLayout(boolean changed, int left, int top, int right, int bottom)
	{
		//Log.i(logTag, "onLayout called: " + left + ":" + top + ":" + right + ":" + bottom);
		View rootView = getRootView();
		int newWindowWidth = rootView.getWidth();
		int newWindowHeight = rootView.getHeight();
		if(android.os.Build.VERSION.SDK_INT >= 10)
		{
			if(getGlobalVisibleRect(globalRect))
			{
				//Log.i(logTag, "getGlobalVisibleRect: " + globalRect.left + ":" + globalRect.top + ":" + globalRect.right + ":" + globalRect.bottom);
				left = globalRect.left;
				top = globalRect.top;
				right = globalRect.right;
				bottom = globalRect.bottom;
			}
		}
		if(contentRect.left != left || contentRect.top != top
			|| contentRect.right != right || contentRect.bottom != bottom
			|| newWindowWidth != windowWidth || newWindowHeight != windowHeight)
		{
			//Log.i(logTag, "content rect: " + contentRect.left + "," + contentRect.top + " " + contentRect.right + "," + contentRect.bottom
			//		+ " -> " + left + "," + top + " " + right + "," + bottom);
			BaseActivity.onContentRectChanged(nativeUserData, left, top, right, bottom, newWindowWidth, newWindowHeight);
			contentRect.left = left;
			contentRect.top = top;
			contentRect.right = right;
			contentRect.bottom = bottom;
			windowWidth = newWindowWidth;
			windowHeight = newWindowHeight;
		}
	}
}

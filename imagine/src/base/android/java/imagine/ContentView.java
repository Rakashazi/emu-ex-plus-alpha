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

//For API level >= 16, SYSTEM_UI_FLAG_LAYOUT_* always gives us a full screen view rectangle,
// so use fitSystemWindows to get the area not overlapping the system windows
final class ContentView extends View
{
	public long windowAddr;
	private Rect contentRect = new Rect();
	private int windowWidth, windowHeight;

	public ContentView(Context context)
	{
		super(context);
	}
	
	public ContentView(Context context, long windowAddr)
	{
		super(context);
		this.windowAddr = windowAddr; 
	}
	
	@Override protected boolean fitSystemWindows(Rect insets)
	{
		if(getWidth() == 0 || getHeight() == 0)
			return true;
		//Log.i(logTag, "system window insets: " + insets.left + "," + insets.top + " " + insets.right + "," + insets.bottom);
		View rootView = getRootView();
		int newWindowWidth = rootView.getWidth();
		int newWindowHeight = rootView.getHeight();
		// adjust insets to become content rect
		insets.right = getWidth() - insets.right;
		insets.bottom = getHeight() - insets.bottom;
		if(!contentRect.equals(insets) || newWindowWidth != windowWidth || newWindowHeight != windowHeight)
		{
			//Log.i(logTag, "content rect: " + contentRect.left + "," + contentRect.top + " " + contentRect.right + "," + contentRect.bottom
			//		+ " -> " + insets.left + "," + insets.top + " " + insets.right + "," + insets.bottom);
			BaseActivity.onContentRectChanged(windowAddr, insets.left, insets.top, insets.right, insets.bottom, newWindowWidth, newWindowHeight);
			contentRect.left = insets.left;
			contentRect.top = insets.top;
			contentRect.right = insets.right;
			contentRect.bottom = insets.bottom;
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

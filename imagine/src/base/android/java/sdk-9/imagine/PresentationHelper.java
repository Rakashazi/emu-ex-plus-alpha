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
import android.view.*;
import android.hardware.display.*;

import android.graphics.*;
import android.graphics.drawable.*;

final class PresentationHelper extends Presentation implements SurfaceHolder.Callback2
{
	private static final String logTag = "Presentation";
	private long windowAddr;
	private View view;
	private native void onSurfaceCreated(long windowAddr, Surface surface);
	private native void onSurfaceChanged(long windowAddr);
	private native void onSurfaceRedrawNeeded(long windowAddr);
	private native void onSurfaceDestroyed(long windowAddr);
	
	PresentationHelper(Activity context, Display display, long windowAddr)
	{
		super(context, display);
		this.windowAddr = windowAddr;
		view = new View(context);
		/*GradientDrawable drawable = new GradientDrawable();
        drawable.setShape(GradientDrawable.RECTANGLE);
        drawable.setGradientType(GradientDrawable.RADIAL_GRADIENT);

        // Set the background to a random gradient.
        Point p = new Point();
        getDisplay().getSize(p);
        drawable.setGradientRadius(Math.max(p.x, p.y) / 2);
        int[] colors = {0xFFFF0000, 0xFFCC0099};
        drawable.setColors(colors);
        view.setBackground(drawable);*/
	}
	
	@Override protected void onCreate(Bundle savedInstanceState)
	{
		Log.i(logTag, "onCreate");
		getWindow().takeSurface(this);
        setContentView(view);
	}
	
	public void surfaceCreated(SurfaceHolder holder)
	{
		Log.i(logTag, "surfaceCreated");
		onSurfaceCreated(windowAddr, holder.getSurface());
	}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
	{
		Log.i(logTag, "surfaceChanged");
		onSurfaceChanged(windowAddr);
	}
	
	public void surfaceRedrawNeeded(SurfaceHolder holder)
	{
		Log.i(logTag, "surfaceRedrawNeeded");
		onSurfaceRedrawNeeded(windowAddr);
	}
	
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		Log.i(logTag, "surfaceDestroyed");
		onSurfaceDestroyed(windowAddr);
	}
}

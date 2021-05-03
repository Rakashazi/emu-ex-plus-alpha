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

import android.view.*;
import android.graphics.*;
import android.os.*;
import android.util.Log;

final class FontRenderer
{
	private static final String logTag = "FontRenderer";
	private final Canvas canvas = new Canvas();

	// Android 2.3's Canvas setBitmap() derefs parameter without null checking,  
	// fixed in Android 4.0.3+ but must use a dummy bitmap for old versions
	private final Bitmap nullBitmap =
		android.os.Build.VERSION.SDK_INT < 15 ? Bitmap.createBitmap(1, 1, Bitmap.Config.ALPHA_8) : null;

	private native void charMetricsCallback(long metricsAddr,
		int xSize, int ySize, int xOff, int yOff, int xAdv);

	private Bitmap glyphData(int idx, Paint paint, long metricsAddr, boolean makeBitmap)
	{
		// metrics
		char[] cStr = new char[2];
		cStr[0] = (char)idx;
		//Log.i(logTag, "glyph idx:" + idx);
		Rect rect = new Rect();
		paint.getTextBounds(cStr, 0, 1, rect);
		int xSize = rect.right - rect.left;
		int ySize = rect.bottom - rect.top;
		if(xSize == 0 || ySize == 0)
			return null;
		float[] w = new float[2];
		paint.getTextWidths(cStr, 0, 1, w);
		int advance = (int)w[0];
		//Log.i(logTag, "rect:" + rect.left + ":" + rect.right + ":" + rect.top + ":" + rect.bottom);
		int cXSize = xSize;
		int cYSize = ySize;
		int left = rect.left;
		int top = -rect.top;
		int bottom = rect.bottom;
		charMetricsCallback(metricsAddr, cXSize, cYSize, left, top, advance);
		
		// bitmap
		if(!makeBitmap)
			return null;
		Bitmap bitmap = Bitmap.createBitmap(cXSize, cYSize, Bitmap.Config.ALPHA_8);
		canvas.setBitmap(bitmap);
		canvas.drawText(cStr, 0, 1, -left, cYSize - bottom, paint);
		canvas.setBitmap(nullBitmap); // release text bitmap so it's destroyed later
		return bitmap;
	}

	Bitmap bitmap(int idx, Paint paint, long metricsAddr)
	{
		return glyphData(idx, paint, metricsAddr, true);
	}

	void metrics(int idx, Paint paint, long metricsAddr)
	{
		glyphData(idx, paint, metricsAddr, false);
	}

	Paint makePaint(int size, boolean isBold)
	{
		Paint p = new Paint();
		if(isBold)
		{
			p.setTypeface(Typeface.DEFAULT_BOLD);
		}
		p.setAntiAlias(true);
		p.setTextSize(size);
		p.setColor(0xffffffff);
		return p;
	}
}

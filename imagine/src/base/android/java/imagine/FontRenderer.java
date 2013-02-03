package com.imagine;

import android.view.*;
import android.graphics.*;
import android.os.*;
import android.util.Log;

final class FontRenderer
{
	private static String logTag = "FontRenderer";
	Canvas canvas = new Canvas();
	Paint paint;
	Bitmap bitmap;
	char activeChar;
	int cXSize, cYSize, top, left, bottom, advance /*, ascender, descender*/;
	
	public int currentCharYOffset() { return top; }
	public int currentCharXOffset() { return left; }
	public int currentCharXAdvance() { return advance; }
	
	public Bitmap charBitmap()
	{
		bitmap = Bitmap.createBitmap(cXSize, cYSize, Bitmap.Config.ALPHA_8);
		bitmap.eraseColor(Color.TRANSPARENT);
		canvas.setBitmap(bitmap);
		char[] cStr = new char[2];
		cStr[0] = activeChar;
		canvas.drawText(cStr, 0, 1, -left, cYSize - bottom, paint);
		return bitmap;
	}
	
	public void unlockCharBitmap(Bitmap bitmap)
	{
		bitmap.recycle();
		bitmap = null;
	}
	
	public boolean activeChar(int idx)
	{
		if(activeChar != idx)
		{
			char[] cStr = new char[2];
			cStr[0] = (char)idx;
			//Log.i(logTag, "active char " + activeChar + " x size: " + cXSize);*/
			Rect rect = new Rect();
			paint.getTextBounds(cStr, 0, 1, rect);
			int xSize = rect.right - rect.left;
			int ySize = rect.bottom - rect.top;
			if(xSize == 0 || ySize == 0)
				return false;
			float[] w = new float[2];
			paint.getTextWidths(cStr, 0, 1, w);
			advance = (int)w[0];
			//Log.i(logTag, "active char " + (char)idx + " rect " + rect.left + ":" + rect.right + ":" + rect.top + ":" + rect.bottom);
			cXSize = xSize;
			cYSize = ySize;
			left = rect.left;
			top = -rect.top;
			bottom = rect.bottom;
			activeChar = (char)idx;
		}
		return true;
	}
	
	public int currentCharXSize()
	{
		return cXSize;
	}

	public int currentCharYSize()
	{
		return cYSize;
	}
	
	/*public int currentFaceDescender()
	{
		return descender;
	}
	
	public int currentFaceAscender()
	{
		return ascender;
	}*/

	public Paint newSize(int size)
	{
		Paint p = new Paint();
		p.setAntiAlias(true);
		p.setTextSize(size);
		p.setColor(0xffffffff);
		return p;
	}
	
	public void applySize(Paint p)
	{
		if(paint != p)
		{
			paint = p;
			/*Paint.FontMetrics fm = paint.getFontMetrics();
			maxY = (int)Math.ceil( Math.abs( fm.bottom ) + Math.abs( fm.top ) );
			ascender = (int)Math.ceil( Math.abs( fm.ascent ) );
			descender = (int)Math.ceil( Math.abs( fm.descent ) );
			Log.i(logTag, "font ascender: " + ascender + " descender: " + descender);*/
		}
	}
	
	public void freeSize(Paint p)
	{
		if(paint == p)
			paint = null;
	}
	
	protected void finalize() throws Throwable
	{
		if(bitmap != null)
			bitmap.recycle();
	}
}

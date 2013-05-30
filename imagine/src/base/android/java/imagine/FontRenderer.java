package com.imagine;

import android.view.*;
import android.graphics.*;
import android.os.*;
import android.util.Log;

final class FontRenderer
{
	private static final String logTag = "FontRenderer";
	private final Canvas canvas = new Canvas();
	private Paint paint;
	private Bitmap bitmap;
	private char activeChar;
	private int cXSize, cYSize, top, left, bottom, advance /*, ascender, descender*/;
	
	public int currentCharYOffset() { return top; }
	public int currentCharXOffset() { return left; }
	public int currentCharXAdvance() { return advance; }
	
	Bitmap charBitmap()
	{
		bitmap = Bitmap.createBitmap(cXSize, cYSize, Bitmap.Config.ALPHA_8);
		Log.i(logTag, "created bitmap " + bitmap.toString());
		bitmap.eraseColor(Color.TRANSPARENT);
		canvas.setBitmap(bitmap);
		char[] cStr = new char[2];
		cStr[0] = activeChar;
		canvas.drawText(cStr, 0, 1, -left, cYSize - bottom, paint);
		return bitmap;
	}
	
	void unlockCharBitmap(Bitmap bitmap)
	{
		bitmap.recycle();
		bitmap = null;
	}
	
	boolean activeChar(int idx)
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
	
	int currentCharXSize()
	{
		return cXSize;
	}

	int currentCharYSize()
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

	Paint newSize(int size)
	{
		Paint p = new Paint();
		p.setAntiAlias(true);
		p.setTextSize(size);
		p.setColor(0xffffffff);
		return p;
	}
	
	void applySize(Paint p)
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
	
	void freeSize(Paint p)
	{
		if(paint == p)
			paint = null;
	}
	
	public void finalize() throws Throwable
	{
		if(bitmap != null)
			bitmap.recycle();
	}
}

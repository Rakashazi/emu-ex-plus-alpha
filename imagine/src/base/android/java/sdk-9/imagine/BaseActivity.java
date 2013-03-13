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

import android.widget.*;
import android.app.*;
import android.content.*;
import android.content.DialogInterface.*;
import android.view.inputmethod.*;
import android.graphics.drawable.*;
import android.view.View.*;
import android.os.*;
import android.view.*;
import android.graphics.*;
import android.util.*;
import android.hardware.*;
import android.media.*;
import android.content.res.Configuration;
import android.view.inputmethod.InputMethodManager;
import android.bluetooth.*;
import android.content.pm.*;
import java.lang.reflect.*;

// This class is also named BaseActivity to prevent shortcuts from breaking with previous SDK < 9 APKs

public final class BaseActivity extends NativeActivity
{
	private static String logTag = "BaseActivity";
	//private native void layoutChange(int bottom);
	native boolean drawWindow(long frameTimeNanos);
	private boolean surfaceIs32Bit = false;
	private static boolean hasChoreographer = false;
	private static Method setSystemUiVisibility =
		android.os.Build.VERSION.SDK_INT >= 11 ? Util.getMethod(View.class, "setSystemUiVisibility", new Class[] { int.class }) : null;
	private ChoreographerHelper choreographerHelper;

	static
	{
		try
		{
			ChoreographerHelper.checkAvailable();
			hasChoreographer = true;
		}
		catch(Throwable t)
		{
			// Choreographer class not availible
		}
	}

	void postDrawWindow()
	{
		choreographerHelper.postDrawWindow();
	}
	
	void cancelDrawWindow()
	{
		choreographerHelper.cancelDrawWindow();
	}
	
	boolean hasPermanentMenuKey()
	{
		if(android.os.Build.VERSION.SDK_INT < 14) return true;
		boolean hasKey = true;
		try
		{
			Method hasPermanentMenuKeyFunc = ViewConfiguration.class.getMethod("hasPermanentMenuKey");
			ViewConfiguration viewConf = ViewConfiguration.get(getApplicationContext());
			try
			{
				hasKey = (Boolean)hasPermanentMenuKeyFunc.invoke(viewConf);
			}
			catch (IllegalAccessException ie)
			{
				//Log.i(logTag, "IllegalAccessException calling hasPermanentMenuKeyFunc");
			}
			catch (InvocationTargetException ite)
			{
				//Log.i(logTag, "InvocationTargetException calling hasPermanentMenuKeyFunc");
			}
		}
		catch (NoSuchMethodException nsme)
		{
			//Log.i(logTag, "hasPermanentMenuKeyFunc not present even though SDK >= 14"); // should never happen
		}
		return hasKey;
	}
		
	int sigHash()
	{
		try
		{
			Signature[] sig = getPackageManager().getPackageInfo(getApplicationContext().getPackageName(), PackageManager.GET_SIGNATURES).signatures;
			//Log.i(logTag, "sig hash " + sig[0].hashCode());
			return sig[0].hashCode();
		}
		catch(PackageManager.NameNotFoundException e)
		{
			return 0;
		}
	}
	
	static boolean gbAnimatesRotation()
	{
		// Check if Gingerbread OS provides rotation animation
		return android.os.Build.DISPLAY.contains("cyano"); // Disable our rotation animation on CM7
	}
	
	Display defaultDpy()
	{
		return getWindowManager().getDefaultDisplay();
	}
	
	String apkPath()
	{
		return getApplicationInfo().sourceDir;
	}
	
	String filesDir()
	{
		return getApplicationContext().getFilesDir().getAbsolutePath();
	}
	
	static String extStorageDir()
	{
		return Environment.getExternalStorageDirectory().getAbsolutePath();
	}
	
	static String devName()
	{
		return android.os.Build.DEVICE;
	}
	
	Vibrator systemVibrator()
	{
		Vibrator vibrator = (Vibrator)getApplicationContext().getSystemService(Context.VIBRATOR_SERVICE);
		boolean hasVibrator = vibrator != null ? true : false;
		if(hasVibrator && android.os.Build.VERSION.SDK_INT >= 11)
		{
			// check if a vibrator is really present
			try
			{
				Method hasVibratorFunc = Vibrator.class.getMethod("hasVibrator");
				try
				{
					hasVibrator = (Boolean)hasVibratorFunc.invoke(vibrator);
				}
				catch (IllegalAccessException ie)
				{
					//Log.i(logTag, "IllegalAccessException calling hasVibratorFunc");
				}
				catch (InvocationTargetException ite)
				{
					//Log.i(logTag, "InvocationTargetException calling hasVibratorFunc");
				}
			}
			catch (NoSuchMethodException nsme)
			{
				//Log.i(logTag, "hasVibratorFunc not present even though SDK >= 11"); // should never happen
			}
		}
		return hasVibrator ? vibrator : null;
	}
	
	void setKeepScreenOn(boolean on)
	{
		getWindow().getDecorView().setKeepScreenOn(on);
	}
	
	void setUIVisibility(int mode)
	{
		if(setSystemUiVisibility == null)
		{
			return;
		}
		try
		{
			setSystemUiVisibility.invoke(findViewById(android.R.id.content), mode);
		}
		catch (IllegalAccessException ie)
		{
			//Log.i(logTag, "IllegalAccessException calling setSystemUiVisibility");
		}
		catch (InvocationTargetException ite)
		{
			//Log.i(logTag, "InvocationTargetException calling setSystemUiVisibility");
		}
	}
	
	void setFullscreen(boolean fullscreen)
	{
		Window win = getWindow();
		if(fullscreen)
			win.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
		else
			win.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
	}
	
	public void addNotification(String onShow, String title, String message)
	{
		NotificationHelper.addNotification(getApplicationContext(), onShow, title, message);
	}
	
	public void removeNotification()
	{
		NotificationHelper.removeNotification();
	}
	
	static native void onBTScanStatus(int result);
	static native boolean onScanDeviceClass(int btClass);
	static native void onScanDeviceName(String name, String addr);
	
	public BluetoothAdapter btDefaultAdapter()
	{
		//Log.i(logTag, "btDefaultAdapter()");
		return Bluetooth.defaultAdapter(this);
	}
	
	public int btStartScan(BluetoothAdapter adapter)
	{
		//Log.i(logTag, "btStartScan()");
		return Bluetooth.startScan(this, adapter);
	}
	
	public void btCancelScan(BluetoothAdapter adapter)
	{
		Bluetooth.cancelScan(this, adapter);
	}
	
	public BluetoothSocket btOpenSocket(BluetoothAdapter adapter, String address, int ch, boolean l2cap)
	{
		//Log.i(logTag, "btOpenSocket()");
		return Bluetooth.openSocket(adapter, address, ch, l2cap);
	}
	
	/*@Override public void onGlobalLayout()
	{
		super.onGlobalLayout();
		Rect r = new Rect();
		View view = getWindow().getDecorView();//findViewById(android.R.id.activityRoot);
		view.getWindowVisibleDisplayFrame(r);
		//int visibleY = r.bottom - r.top;
		//Log.i(logTag, "height " + view.getRootView().getHeight() + ", visible " + visibleY);
		//layoutChange(visibleY);
		layoutChange(r.bottom);
     }*/
	
	@Override protected void onCreate(Bundle savedInstanceState)
	{
		if(hasChoreographer)
		{
			//Log.i(logTag, "using Choreographer");
			choreographerHelper = new ChoreographerHelper(this);
		}
		Bluetooth.adapter = BluetoothAdapter.getDefaultAdapter();
		super.onCreate(savedInstanceState);
	}
	
	@Override protected void onResume()
	{
		removeNotification();
		super.onResume();
	}
	
	@Override protected void onDestroy()
	{
		//Log.i(logTag, "onDestroy");
		removeNotification();
		super.onDestroy();
	}
	
	static native void sysTextInputEnded(String text);
	
	public static void endSysTextInput(String text)
	{
		sysTextInputEnded(text);
	}
	
	public void startSysTextInput(final String initialText, final String promptText,
		final int x, final int y, final int width, final int height)
	{
		TextEntry.startSysTextInput(this, initialText, promptText, x, y, width, height);
	}
	
	public void finishSysTextInput(final boolean canceled)
	{
		TextEntry.finishSysTextInput(canceled);
	}
	
	public void placeSysTextInput(final int x, final int y, final int width, final int height)
	{
		TextEntry.placeSysTextInput(x, y, width, height);
	}

	@Override public void surfaceDestroyed(SurfaceHolder holder)
	{
		//Log.i(logTag, "surfaceDestroyed");
		super.surfaceDestroyed(holder);
		// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
		// resuming the app and ANativeWindow_setBuffersGeometry() has no effect.
		// Explicitly setting the format here seems to fix the problem. Android bug?
		if(android.os.Build.VERSION.SDK_INT < 11)
		{
			if(surfaceIs32Bit)
				getWindow().setFormat(PixelFormat.RGBA_8888);
			else
				getWindow().setFormat(PixelFormat.RGB_565);
		}
	}
}
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
	private native void jEnvConfig(float xdpi, float ydpi, int refreshRate, Display dpy, String devName,
			String filesPath, String eStoragePath, String apkPath, Vibrator sysVibrator,
			boolean hasPermanentMenuKey, boolean osAnimatesRotation, int sigHash);
	//private native void layoutChange(int bottom);
	private boolean surfaceIs32Bit = false;
	private static Method setSystemUiVisibility =
		android.os.Build.VERSION.SDK_INT >= 11 ? Util.getMethod(View.class, "setSystemUiVisibility", new Class[] { int.class }) : null;
	
	private int sigHash()
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

	private void setupEnv()
	{
		//Log.i(logTag, "got focus view " + getWindow().getDecorView());
		//contentView = findViewById(android.R.id.content);//getWindow().getDecorView();
		//view.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE);
		Bluetooth.adapter = BluetoothAdapter.getDefaultAdapter();
		Display dpy = getWindowManager().getDefaultDisplay();
		DisplayMetrics metrics = new DisplayMetrics();
		dpy.getMetrics(metrics);
		//Log.i(logTag, "Metrics: " + metrics.toString());
		boolean osAnimatesRotation = false;
		if(android.os.Build.VERSION.SDK_INT >= 11 || android.os.Build.DISPLAY.contains("cyano"))
		{
			// Disable our rotation animation on Android 3.0+ or CM7
			osAnimatesRotation = true;
		}
		//int xMM = (int)(((float)metrics.widthPixels / metrics.xdpi) * 25.4);
		//int yMM = (int)(((float)metrics.heightPixels / metrics.ydpi) * 25.4);
		int orientation = dpy.getRotation();
		boolean isStraightOrientation = orientation == Surface.ROTATION_0 || orientation == Surface.ROTATION_180;
		Context context = getApplicationContext();
		boolean hasPermanentMenuKey = true;
		if(android.os.Build.VERSION.SDK_INT >= 14)
		{
			try
			{
				Method hasPermanentMenuKeyFunc = ViewConfiguration.class.getMethod("hasPermanentMenuKey", new Class[] {});
				ViewConfiguration viewConf = ViewConfiguration.get(context);
				try
				{
					hasPermanentMenuKey = (Boolean)hasPermanentMenuKeyFunc.invoke(viewConf);
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
		}
		Vibrator vibrator = (Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE);
		jEnvConfig(isStraightOrientation ? metrics.xdpi : metrics.ydpi,
			isStraightOrientation ? metrics.ydpi : metrics.xdpi,
			(int)dpy.getRefreshRate(), dpy, android.os.Build.DEVICE,
			context.getFilesDir().getAbsolutePath(), Environment.getExternalStorageDirectory().getAbsolutePath(),
			getApplicationInfo().sourceDir, vibrator, hasPermanentMenuKey, osAnimatesRotation, sigHash());
	}
	
	private static final int SET_KEEP_SCREEN_ON = 0, SET_SYSTEM_UI_VISIBILITY = 1,
		SHOW_SOFT_INPUT = 2, HIDE_SOFT_INPUT = 3;
	
	public void postUIThread(int func, final int param)
	{
		if(func == SET_KEEP_SCREEN_ON)
			runOnUiThread(new Runnable()
			{
				public void run()
				{
					getWindow().getDecorView().setKeepScreenOn(param == 0 ? false : true);
				}
			});
		/*else if(func == SHOW_SOFT_INPUT)
			runOnUiThread(new Runnable()
			{
				public void run()
				{
					//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
					InputMethodManager mIMM = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
					mIMM.showSoftInput(getWindow().getDecorView(), 0);
				}
			});
		else if(func == HIDE_SOFT_INPUT)
			runOnUiThread(new Runnable()
			{
				public void run()
				{
					InputMethodManager mIMM = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
					mIMM.hideSoftInputFromWindow(getWindow().getDecorView().getWindowToken(), 0);
				}
			});*/
		else
		{
			if(setSystemUiVisibility != null)
			{
				runOnUiThread(new Runnable()
				{
					public void run()
					{
						try
						{
							setSystemUiVisibility.invoke(findViewById(android.R.id.content), param);
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
				});
			}
		}
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
		super.onCreate(savedInstanceState);
		setupEnv();
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
		final Activity act = this;
		runOnUiThread(new Runnable()
		{
			public void run()
			{
				TextEntry.startSysTextInput(act, initialText, promptText, x, y, width, height);
			}
		});
	}
	
	public void finishSysTextInput(final boolean canceled)
	{
		runOnUiThread(new Runnable()
		{
			public void run()
			{
				TextEntry.finishSysTextInput(canceled);
			}
		});
	}
	
	public void placeSysTextInput(final int x, final int y, final int width, final int height)
	{
		runOnUiThread(new Runnable()
		{
			public void run()
			{
				TextEntry.placeSysTextInput(x, y, width, height);
			}
		});
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
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

public final class BaseActivity extends NativeActivity implements AudioManager.OnAudioFocusChangeListener
{
	private static final String logTag = "BaseActivity";
	//private native void layoutChange(int bottom);
	private static final Method setSystemUiVisibility =
		android.os.Build.VERSION.SDK_INT >= 11 ? Util.getMethod(View.class, "setSystemUiVisibility", new Class[] { int.class }) : null;

	private final class IdleHelper implements MessageQueue.IdleHandler
	{
		private final MessageQueue msgQueue = Looper.myQueue();
		private final Handler handler = new Handler();
		private native boolean drawWindow();

		void postDrawWindow()
		{
			msgQueue.addIdleHandler(this);
			handler.sendMessageAtFrontOfQueue(Message.obtain()); // force idle handler to run in case of no pending msgs
			//Log.i(logTag, "start idle handler");
		}
		
		void cancelDrawWindow()
		{
			//Log.i(logTag, "stop idle handler");
			msgQueue.removeIdleHandler(this);
		}

		@Override public boolean queueIdle()
		{
			//Log.i(logTag, "in idle handler");
			if(drawWindow())
			{
				//Log.i(logTag, "will re-run");
				handler.sendMessageAtFrontOfQueue(Message.obtain()); // force idle handler to re-run in case of no pending msgs
				return true;
			}
			else
			{
				//Log.i(logTag, "won't re-run");
				return false;
			}
		}
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
	
	DisplayMetrics displayMetrics()
	{
		return getResources().getDisplayMetrics();
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
	
	AudioManager audioManager()
	{
		return (AudioManager)getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
	}
	
	@Override public void onAudioFocusChange(int focusChange)
	{
		//Log.i(logTag, "audio focus change: " focusChange);
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
	
	void addNotification(String onShow, String title, String message)
	{
		NotificationHelper.addNotification(getApplicationContext(), onShow, title, message);
	}
	
	void removeNotification()
	{
		NotificationHelper.removeNotification();
	}
	
	static native void onBTScanStatus(int result);
	static native boolean onScanDeviceClass(int btClass);
	static native void onScanDeviceName(String name, String addr);
	static native void onBTOn(boolean success);
	
	BluetoothAdapter btDefaultAdapter()
	{
		//Log.i(logTag, "btDefaultAdapter()");
		return Bluetooth.defaultAdapter(this);
	}
	
	int btStartScan(BluetoothAdapter adapter)
	{
		//Log.i(logTag, "btStartScan()");
		return Bluetooth.startScan(adapter) ? 1 : 0;
	}
	
	void btCancelScan(BluetoothAdapter adapter)
	{
		Bluetooth.cancelScan(adapter);
	}
	
	BluetoothSocket btOpenSocket(BluetoothAdapter adapter, String address, int ch, boolean l2cap)
	{
		//Log.i(logTag, "btOpenSocket()");
		return Bluetooth.openSocket(adapter, address, ch, l2cap);
	}
	
	int btState(BluetoothAdapter adapter)
	{
		return adapter.getState();
	}
	
	private static final int REQUEST_BT_ON = 1;
	
	void btTurnOn()
	{
		Intent btOn = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
		startActivityForResult(btOn, REQUEST_BT_ON);
	}
	
	@Override protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if(requestCode == REQUEST_BT_ON)
		{
			onBTOn(resultCode == RESULT_OK);
		}
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
		getWindow().setBackgroundDrawable(null);
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
	
	static void endSysTextInput(String text)
	{
		sysTextInputEnded(text);
	}
	
	void startSysTextInput(final String initialText, final String promptText,
		int x, int y, int width, int height, int fontSize)
	{
		TextEntry.startSysTextInput(this, initialText, promptText, x, y, width, height, fontSize);
	}
	
	void finishSysTextInput(final boolean canceled)
	{
		TextEntry.finishSysTextInput(canceled);
	}
	
	void placeSysTextInput(final int x, final int y, final int width, final int height)
	{
		TextEntry.placeSysTextInput(x, y, width, height);
	}
	
	FontRenderer newFontRenderer()
	{
		return new FontRenderer();
	}
	
	ChoreographerHelper newChoreographerHelper()
	{
		return new ChoreographerHelper();
	}
	
	IdleHelper newIdleHelper()
	{
		return new IdleHelper();
	}
}
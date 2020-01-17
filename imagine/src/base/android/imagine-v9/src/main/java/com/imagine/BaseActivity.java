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
import android.net.*;
import android.content.res.*;
import android.view.inputmethod.InputMethodManager;
import android.bluetooth.*;
import android.content.pm.*;
import java.lang.reflect.*;
import java.io.*;
import android.support.v4.content.ContextCompat;
import android.support.v4.app.ActivityCompat;

// This class is also named BaseActivity to prevent shortcuts from breaking with previous SDK < 9 APKs

public final class BaseActivity extends NativeActivity implements AudioManager.OnAudioFocusChangeListener
{
	private static final String logTag = "BaseActivity";
	static native void onContentRectChanged(long windowAddr,
		int left, int top, int right, int bottom, int windowWidth, int windowHeight);
	private static final Method setSystemUiVisibility =
		android.os.Build.VERSION.SDK_INT >= 11 ? Util.getMethod(View.class, "setSystemUiVisibility", new Class[] { int.class }) : null;
	private static final int commonUILayoutFlags = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
	
	boolean hasPermanentMenuKey()
	{
		if(android.os.Build.VERSION.SDK_INT < 14) return true;
		boolean hasKey = true;
		try
		{
			Method hasPermanentMenuKeyFunc = ViewConfiguration.class.getMethod("hasPermanentMenuKey");
			ViewConfiguration viewConf = ViewConfiguration.get(this);
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
			Signature[] sig = getPackageManager().getPackageInfo(getPackageName(), PackageManager.GET_SIGNATURES).signatures;
			//Log.i(logTag, "sig hash " + sig[0].hashCode());
			return sig[0].hashCode();
		}
		catch(PackageManager.NameNotFoundException e)
		{
			return 0;
		}
	}
	
	boolean packageIsInstalled(String name)
	{
		boolean found = false;
		try
		{
			getPackageManager().getPackageInfo(name, 0);
			found = true;
		}
		catch(PackageManager.NameNotFoundException e)
		{
		}
		return found;
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
	
	DisplayMetrics getDisplayMetrics(Display display)
	{
		DisplayMetrics metrics = new DisplayMetrics();
		display.getMetrics(metrics);
		return metrics;
	}
	
	String filesDir()
	{
		return getFilesDir().getAbsolutePath();
	}
	
	String cacheDir()
	{
		return getCacheDir().getAbsolutePath();
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
		Vibrator vibrator = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
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
		return (AudioManager)getSystemService(Context.AUDIO_SERVICE);
	}
	
	boolean hasLowLatencyAudio()
	{
		return getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_LOW_LATENCY);
	}
	
	@Override public void onAudioFocusChange(int focusChange)
	{
		//Log.i(logTag, "audio focus change: " focusChange);
	}
	
	void setUIVisibility(int mode)
	{
		if(setSystemUiVisibility == null)
		{
			return;
		}
		try
		{
			int flags = mode | commonUILayoutFlags;
			if((android.os.Build.VERSION.SDK_INT == 16 || android.os.Build.VERSION.SDK_INT == 17)
				&& (mode & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0)
			{
				// if not hiding navigation, use a "stable" layout so Android 4.1 & 4.2 don't return all 0 view insets
				//Log.i(logTag, "using stable layout");
				flags |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
			}
			setSystemUiVisibility.invoke(getWindow().getDecorView(), flags);
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
	
	void setWinFlags(int flags, int mask)
	{
		getWindow().setFlags(flags, mask);
	}
	
	void setWinFormat(int format)
	{
		getWindow().setFormat(format);
	}
	
	int winFlags()
	{
		return getWindow().getAttributes().flags;
	}
	
	int winFormat()
	{
		return getWindow().getAttributes().format;
	}
	
	void addNotification(String onShow, String title, String message)
	{
		NotificationHelper.addNotification(this, onShow, title, message);
	}
	
	void removeNotification()
	{
		NotificationHelper.removeNotification(this);
	}
	
	static native void onBTScanStatus(int result);
	static native boolean onScanDeviceClass(int btClass);
	static native void onScanDeviceName(String name, String addr);
	static native void onBTOn(boolean success);
	
	BluetoothAdapter btDefaultAdapter()
	{
		//Log.i(logTag, "btDefaultAdapter()");
		return Bluetooth.defaultAdapter();
	}
	
	int btStartScan(BluetoothAdapter adapter)
	{
		//Log.i(logTag, "btStartScan()");
		return Bluetooth.startScan(this, adapter) ? 1 : 0;
	}
	
	void btCancelScan(BluetoothAdapter adapter)
	{
		Bluetooth.cancelScan(this, adapter);
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
	
	@Override public void onGlobalLayout()
	{
		// override to make sure NativeActivity's implementation is never called
		// since our content is laid out with BaseContentView
	}
	
	String intentDataPath()
	{
		//Log.i(logTag, "intent action: " + getIntent().getAction());
		String path = null;
		Uri uri = getIntent().getData();
		if(uri != null)
		{
			path = uri.getPath();
			//Log.i(logTag, "path: " + path);
			getIntent().setData(null); // data is one-time use
		}
		return path;
	}
	
	void addViewShortcut(String name, String path)
	{
		Intent viewIntent = new Intent(this, BaseActivity.class);
		viewIntent.setAction(Intent.ACTION_VIEW);
		viewIntent.setData(Uri.parse("file://" + path));
		int icon = getResources().getIdentifier("icon", "drawable", getPackageName());
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
			ShortcutManager shortcutManager = getSystemService(ShortcutManager.class);
			ShortcutInfo shortcutInfo = new ShortcutInfo.Builder(this, name)
				.setShortLabel(name)
				.setIcon(Icon.createWithResource(this, icon))
				.setIntent(viewIntent)
				.build();
			shortcutManager.requestPinShortcut(shortcutInfo, null);
		}
		else
		{
			Intent launcherIntent = new Intent();
			launcherIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, viewIntent);
			launcherIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, name);
			final String EXTRA_SHORTCUT_DUPLICATE = "duplicate";
			launcherIntent.putExtra(EXTRA_SHORTCUT_DUPLICATE, false);
			launcherIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, Intent.ShortcutIconResource.fromContext(this, icon));
			launcherIntent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
			sendBroadcast(launcherIntent);
		}
	}
	
	@Override protected void onNewIntent(Intent intent)
	{
		setIntent(intent);
	}
	
	@Override protected void onCreate(Bundle savedInstanceState)
	{
		Window win = getWindow();
		win.addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN | WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR);
		super.onCreate(savedInstanceState);
		win.setBackgroundDrawable(null);
		win.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNSPECIFIED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
		if(android.os.Build.VERSION.SDK_INT >= 11)
		{
			// NativeActivity explicitly sets the window format to RGB_565, this is fine in Android 2.3 since we default to that
			// and call setFormat ourselves, but in >= 3.0 we only use ANativeWindow_setBuffersGeometry so set the format back to
			// the default value to avoid a spurious surface destroy & create the next time the window flags are set since it may
			// cause the screen to flash
			win.setFormat(PixelFormat.UNKNOWN);
		}
		// get rid of NativeActivity's view and layout listener, then add our custom view
		View nativeActivityView = findViewById(android.R.id.content);
		nativeActivityView.getViewTreeObserver().removeGlobalOnLayoutListener(this);
		View contentView;
		if(android.os.Build.VERSION.SDK_INT >= 24)
			contentView = new ContentViewV24(this);
		else if(android.os.Build.VERSION.SDK_INT >= 16)
			contentView = new ContentViewV16(this);
		else
			contentView = new ContentViewV9(this);
		setContentView(contentView);
		contentView.requestFocus();
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
	
	static native void sysTextInputEnded(String text, boolean processText, boolean isDoingDismiss);
	
	static void endSysTextInput(String text, boolean processText, boolean isDoingDismiss)
	{
		sysTextInputEnded(text, processText, isDoingDismiss);
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
	
	ChoreographerHelper newChoreographerHelper(long timerAddr)
	{
		return new ChoreographerHelper(timerAddr);
	}
	
	InputDeviceHelper inputDeviceHelper()
	{
		return new InputDeviceHelper();
	}
	
	InputDeviceListenerHelper inputDeviceListenerHelper()
	{
		return new InputDeviceListenerHelper(this);
	}
	
	DisplayListenerHelper displayListenerHelper()
	{
		return new DisplayListenerHelper(this);
	}
	
	MOGAHelper mogaHelper(long mogaSystemAddr)
	{
		return new MOGAHelper(this, mogaSystemAddr);
	}
	
	PresentationHelper presentation(Display display, long windowAddr)
	{
		PresentationHelper p = new PresentationHelper(this, display, windowAddr);
		return p;
	}
	
	UserActivityFaker userActivityFaker()
	{
		return new UserActivityFaker();
	}
	
	void setSustainedPerformanceMode(boolean on)
	{
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
		{
			getWindow().setSustainedPerformanceMode(on);
		}
	}
	
	Bitmap makeBitmap(int width, int height, int format)
	{
		Bitmap.Config config = Bitmap.Config.ARGB_8888;
		if(format == 4)
			config = Bitmap.Config.RGB_565;
		return Bitmap.createBitmap(width, height, config);
	}
	
	boolean writePNG(Bitmap bitmap, String path)
	{
		boolean success;
		try
		{
			FileOutputStream output = new FileOutputStream(path);
			success = bitmap.compress(Bitmap.CompressFormat.PNG, 100, output);
			output.close();
		}
		catch(Exception e)
		{
			success = false;
		}
		bitmap.recycle();
		return success;
	}
	
	Bitmap bitmapDecodeAsset(String name)
	{
		AssetManager assets = getAssets();
		InputStream in;
		try
		{
			in = assets.open(name);
		}
		catch(Exception e)
		{
			return null;
		}
		Bitmap bitmap = BitmapFactory.decodeStream(in);
		return bitmap;
	}
	
	String libDir()
	{
		ActivityInfo ai;
		try
		{
			ai = getPackageManager().getActivityInfo(getIntent().getComponent(), PackageManager.GET_META_DATA);
		}
		catch(PackageManager.NameNotFoundException e)
		{
			throw new RuntimeException("Error getting activity info", e);
		}
		return ai.applicationInfo.nativeLibraryDir;
	}
	
	String mainSOPath()
	{
		String libname = "main";
		return libDir() + "/" + System.mapLibraryName(libname);
	}
	
	boolean requestPermission(String permission)
	{
		if(ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_GRANTED)
			return true;
		ActivityCompat.requestPermissions(this, new String[]{permission}, 0);
		return false;
	}
	
	void makeErrorPopup(String text)
	{
		TextView view = new TextView(this);
		view.setText(text);
		final PopupWindow win = new PopupWindow(view, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		final View contentView = findViewById(android.R.id.content);
		contentView.post(new Runnable() {
			public void run() {
				win.showAtLocation(contentView, Gravity.CENTER, 0, 0);
			}
		});
	}
	
	void openURL(String url)
	{
		Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
		if(intent.resolveActivity(getPackageManager()) != null)
		{
			startActivity(intent);
		}
	}
}

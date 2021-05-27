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

import android.widget.TextView;
import android.widget.PopupWindow;
import android.app.NativeActivity;
import android.content.Intent;
import android.content.Context;
import android.graphics.drawable.Icon;
import android.os.Vibrator;
import android.os.Bundle;
import android.os.Environment;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewConfiguration;
import android.view.Window;
import android.view.WindowManager;
import android.view.Display;
import android.view.InputDevice;
import android.view.Gravity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.util.DisplayMetrics;
import android.media.AudioManager;
import android.net.Uri;
import android.content.res.AssetManager;
import android.view.inputmethod.InputMethodManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothSocket;
import android.content.pm.Signature;
import android.content.pm.PackageManager;
import android.content.pm.ShortcutManager;
import android.content.pm.ShortcutInfo;
import java.io.FileOutputStream;
import java.io.InputStream;
import android.support.v4.content.ContextCompat;
import android.support.v4.app.ActivityCompat;

// This class is also named BaseActivity to prevent shortcuts from breaking with previous SDK < 9 APKs

public final class BaseActivity extends NativeActivity implements AudioManager.OnAudioFocusChangeListener
{
	private static final String logTag = "BaseActivity";
	static native void onContentRectChanged(long nativeUserData,
		int left, int top, int right, int bottom, int windowWidth, int windowHeight);
	static native void displayEnumerated(long nativeUserData, Display dpy, int id,
		float refreshRate, int rotation, DisplayMetrics metrics);
	static native void inputDeviceEnumerated(long nativeUserData,
		int devID, InputDevice dev, String name, int src, int kbType,
		int jsAxisBits, boolean isPowerButton);
	static native void documentTreeOpened(long nativeUserData, String path);
	private static final int commonUILayoutFlags = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
	private Display defaultDpy;
	private long activityResultNativeUserData;
	private static final int REQUEST_OPEN_DOCUMENT_TREE = 1;
	private static final int REQUEST_BT_ON = 2;

	boolean hasPermanentMenuKey()
	{
		if(android.os.Build.VERSION.SDK_INT >= 14)
		{
			return ViewConfiguration.get(this).hasPermanentMenuKey();
		}
		return true;
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

	int mainDisplayRotation()
	{
		return defaultDpy.getRotation();
	}

	void enumDisplays(long nativeUserData)
	{
		displayEnumerated(nativeUserData, defaultDpy, Display.DEFAULT_DISPLAY,
			defaultDpy.getRefreshRate(), defaultDpy.getRotation(),
			getResources().getDisplayMetrics());
		if(android.os.Build.VERSION.SDK_INT >= 17)
		{
			DisplayListenerHelper.enumPresentationDisplays(this, nativeUserData);
		}
	}

	void enumInputDevices(long nativeUserData)
	{
		if(android.os.Build.VERSION.SDK_INT >= 12)
		{
			InputDeviceHelper.enumInputDevices(this, nativeUserData);
		}
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
			hasVibrator = vibrator.hasVibrator();
		}
		return hasVibrator ? vibrator : null;
	}
	
	AudioManager audioManager()
	{
		return (AudioManager)getSystemService(Context.AUDIO_SERVICE);
	}
	
	@Override public void onAudioFocusChange(int focusChange)
	{
		//Log.i(logTag, "audio focus change: " focusChange);
	}
	
	void setUIVisibility(int mode)
	{
		if(android.os.Build.VERSION.SDK_INT >= 11)
		{
			int flags = mode | commonUILayoutFlags;
			if((android.os.Build.VERSION.SDK_INT == 16 || android.os.Build.VERSION.SDK_INT == 17)
				&& (mode & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0)
			{
				// if not hiding navigation, use a "stable" layout so Android 4.1 & 4.2 don't return all 0 view insets
				//Log.i(logTag, "using stable layout");
				flags |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
			}
			getWindow().getDecorView().setSystemUiVisibility(flags);
		}
	}

	void setWinFlags(int flags, int mask)
	{
		getWindow().setFlags(flags, mask);
	}

	int winFlags()
	{
		return getWindow().getAttributes().flags;
	}

	Window setMainContentView(long nativeUserData)
	{
		// get rid of NativeActivity's view and layout listener, then add our custom view
		View nativeActivityView = findViewById(android.R.id.content);
		nativeActivityView.getViewTreeObserver().removeGlobalOnLayoutListener(this);
		View contentView;
		if(android.os.Build.VERSION.SDK_INT >= 24)
			contentView = new ContentViewV24(this, nativeUserData);
		else if(android.os.Build.VERSION.SDK_INT >= 16)
			contentView = new ContentViewV16(this, nativeUserData);
		else
			contentView = new ContentViewV9(this, nativeUserData);
		setContentView(contentView);
		contentView.requestFocus();
		return getWindow();
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
	
	void btTurnOn()
	{
		Intent btOn = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
		startActivityForResult(btOn, REQUEST_BT_ON);
	}
	
	@Override protected void onActivityResult(int requestCode, int resultCode, Intent intent)
	{
		if(requestCode == REQUEST_BT_ON)
		{
			onBTOn(resultCode == RESULT_OK);
		}
		else if(android.os.Build.VERSION.SDK_INT >= 30 && requestCode == REQUEST_OPEN_DOCUMENT_TREE &&
			resultCode == RESULT_OK && intent != null)
		{
			final String path = StorageManagerHelper.pathFromOpenDocumentTreeResult(this, intent);
			if(path != null)
			{
				documentTreeOpened(activityResultNativeUserData, path);
			}
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
		defaultDpy = getWindowManager().getDefaultDisplay();
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

	TextEntry newTextEntry(final String initialText, final String promptText,
		int x, int y, int width, int height, int fontSize, long nativeUserData)
	{
		return new TextEntry(this, initialText, promptText, x, y, width, height, fontSize, nativeUserData);
	}

	FontRenderer newFontRenderer()
	{
		return new FontRenderer();
	}

	ChoreographerHelper choreographerHelper(long timerAddr)
	{
		if(android.os.Build.VERSION.SDK_INT < 16)
			return null;
		return new ChoreographerHelper(timerAddr);
	}

	InputDeviceListenerHelper inputDeviceListenerHelper(long nativeUserData)
	{
		if(android.os.Build.VERSION.SDK_INT < 16)
			return null;
		return new InputDeviceListenerHelper(this, nativeUserData);
	}
	
	DisplayListenerHelper displayListenerHelper(long nativeUserData)
	{
		if(android.os.Build.VERSION.SDK_INT < 17)
			return null;
		return new DisplayListenerHelper(this, nativeUserData);
	}
	
	MOGAHelper mogaHelper(long nativeUserData)
	{
		return new MOGAHelper(this, nativeUserData);
	}
	
	PresentationHelper presentation(Display display, long nativeUserData)
	{
		if(android.os.Build.VERSION.SDK_INT < 17)
			return null;
		return new PresentationHelper(this, display, nativeUserData);
	}

	StorageManagerHelper storageManagerHelper()
	{
		if(android.os.Build.VERSION.SDK_INT < 24)
			return null;
		return new StorageManagerHelper();
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
		if(Build.VERSION.SDK_INT >= 12)
		{
			bitmap.setHasAlpha(false);
		}
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
		if(android.os.Build.VERSION.SDK_INT >= 24)
			return null;
		return getApplicationInfo().nativeLibraryDir;
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

	void openDocumentTree(long nativeUserData)
	{
		if(android.os.Build.VERSION.SDK_INT < 30)
			return;
		activityResultNativeUserData = nativeUserData;
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		startActivityForResult(intent, REQUEST_OPEN_DOCUMENT_TREE);
	}
}

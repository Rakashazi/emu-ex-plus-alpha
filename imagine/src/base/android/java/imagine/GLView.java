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

import javax.microedition.khronos.egl.*;
import javax.microedition.khronos.opengles.*;
import android.view.*;
import android.graphics.*;
import android.content.Context;
import android.app.Activity;
import android.os.*;
import android.util.Log;

final class GLView extends SurfaceView implements SurfaceHolder.Callback, MessageQueue.IdleHandler
{
	private static String logTag = "GLView";
	private Activity act;
	public boolean initNative, initSurface;
	private static GLView mainGlView;

	public GLView(Activity context)
	{
		super(context);
		act = context;
		mainGlView = this;
		initSurface = false;
		initNative = false;
		SurfaceHolder holder = getHolder();
		holder.setType(SurfaceHolder.SURFACE_TYPE_GPU); // needed for Android 1.6
		holder.addCallback(this);
		mEglHelper = new EglHelper();
		mEglHelper.start();
		if(mEglHelper.configHasAlpha)
			holder.setFormat(PixelFormat.TRANSLUCENT);
		//Log.i(logTag, "set surface type");
		setFocusable(true);
		setFocusableInTouchMode(true);
		//setKeepScreenOn(true);
	}

	static native boolean touchEvent(int action, int x, int y, int pid);
	@Override public boolean onTouchEvent(MotionEvent event)
	{
		//Log.i(logTag, "onTouchEvent");
		//Log.i(logTag, "touch in view " + event.getX() + "," + event.getY());
		if(SDKCompat.onTouchEvent(event))
		{
			postUpdate();
		}
		return true;
	}
	
	private static native boolean trackballEvent(int action, float x, float y);
	@Override public boolean onTrackballEvent(MotionEvent event)
	{
		//Log.i(logTag, "onTrackballEvent");
		if(trackballEvent(event.getAction(), event.getX(), event.getY()))
		{
			postUpdate();
		}
		return true;
	}

	private static EglHelper mEglHelper;
	//private static GL10 gl;
	private static MessageQueue queue;
	static Handler handler;
	private static Runnable painter;
	private static boolean idleHandlerActive;//runnableInQueue;
	
	private static void swapBuffers()
	{
		//Log.i(logTag, "gl swap");
		mEglHelper.swap();
	}
	
	@Override public void surfaceCreated(SurfaceHolder holder)
	{
		//Log.i(logTag, "surfaceCreated");
		mEglHelper.createSurface(holder);
		if(!mEglHelper.verifyContext())
		{
			//Log.i(logTag, "context lost");
			act.finish();
			return;
		}
	}
	
	private static native boolean nativeRender();
	@Override public boolean queueIdle()
	{
		//Log.i(logTag, "in idle handler");
		if(nativeRender())
		{
			//Log.i(logTag, "will re-run");
			Message msg = Message.obtain();
			handler.sendMessageAtFrontOfQueue(msg); // force idle handler to re-run in case of no pending msgs
			return true;
		}
		else
		{
			//Log.i(logTag, "won't re-run");
			idleHandlerActive = false;
			return false;
		}
	}

	private static native void nativeInit(int w, int h);
	private static native void nativeResize(int w, int h);
	@Override public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
		/*Log.i(logTag, "surfaceChanged " + Integer.toString(w) + " " + Integer.toString(h)
		+ " " + getHolder().getSurfaceFrame().toString());*/
		if(mEglHelper.mEglContext == null)
		{
			//Log.i(logTag, "context lost");
			return;
		}
		if(!initNative)
		{
			handler = getHandler();
			nativeInit(w, h);
			queue = Looper.myQueue();
			queue.addIdleHandler(this);
			idleHandlerActive = true;
			initNative = true;
		}
		else
		{
			nativeResize(w, h);
			postUpdate();
			if(initSurface == true && android.os.Build.VERSION.SDK_INT >= 11) //HACK: resize is from a screen rotation, do swap() again or display may not render, Android bug? Custom rotation animation in pre 3.0 redraws the screen at least one more time anyway
			{
				//Log.i(logTag, "extra swap after rotation");
				mEglHelper.swap();
			}
		}
		initSurface = true;
	}
	
	public static void postUpdate()
	{
		if(!idleHandlerActive)
		{
			queue.addIdleHandler(mainGlView);
			idleHandlerActive = true;
			//Log.i(logTag, "start idle handler");
		}
	}
	
	public static void stopUpdate()
	{
		//Log.i(logTag, "stop idle handler");
		queue.removeIdleHandler(mainGlView);
		idleHandlerActive = false;
	}
	
	@Override public void surfaceDestroyed(SurfaceHolder  holder)
	{
		//Log.i(logTag, "surfaceDestroyed");
		stopUpdate();
		mEglHelper.destroySurface();
		initSurface = false;
	}
}

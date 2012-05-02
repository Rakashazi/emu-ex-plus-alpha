package com.imagine;

import android.os.*;
import android.view.*;
import android.content.res.Configuration;
import android.util.Log;

final class SDK5
{	
	private static String logTag = "SDK5";
	
	private static void dumpEvent(MotionEvent event)
	{
		String names[] = { "DOWN" , "UP" , "MOVE" , "CANCEL" , "OUTSIDE" ,
		  "POINTER_DOWN" , "POINTER_UP" , "7?" , "8?" , "9?" };
		StringBuilder sb = new StringBuilder();
		int action = event.getAction();
		int actionCode = action & MotionEvent.ACTION_MASK;
		sb.append("event ACTION_" ).append(names[actionCode]);
		if (actionCode == MotionEvent.ACTION_POINTER_DOWN
			 || actionCode == MotionEvent.ACTION_POINTER_UP) {
		  sb.append("(pid " ).append(
		  action >> MotionEvent.ACTION_POINTER_ID_SHIFT);
		  sb.append(")" );
		}
		sb.append("[" );
		for (int i = 0; i < event.getPointerCount(); i++) {
		  sb.append("#" ).append(i);
		  sb.append("(pid " ).append(event.getPointerId(i));
		  sb.append(")=" ).append((int) event.getX(i));
		  sb.append("," ).append((int) event.getY(i));
		  if (i + 1 < event.getPointerCount())
			 sb.append(";" );
		}
		sb.append("]" );
		Log.i("", sb.toString());
	}

	public static boolean onTouchEvent(final MotionEvent event)
	{
		//dumpEvent(event);
		//Log.i(logTag, "pointers: " + event.getPointerCount());
		
		int eventAction = event.getAction();
		int action = eventAction & MotionEvent.ACTION_MASK;
		int pid = eventAction >> MotionEvent.ACTION_POINTER_ID_SHIFT;
		int pointers = event.getPointerCount();
		// 2.2 API
		//int action = event.getActionMasked();
		//int pid = event.getPointerId(event.getActionIndex());
		
		boolean postUpdate = false;
		for (int i = 0; i < pointers; i++)
		{
			int sendAction = action;
			int pointerId = event.getPointerId(i);
			if(action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_POINTER_UP)
			{
				// send ACTION_POINTER_* for only the pointer it belongs to
				if(pid != pointerId)
					sendAction = MotionEvent.ACTION_MOVE;
			}
			postUpdate |= GLView.touchEvent(sendAction, (int)event.getX(i), (int)event.getY(i), pointerId);
		}
		return postUpdate;
	}
	
	public static int getNavigationHidden(Configuration config)
	{
		//Log.i(logTag, "called getNavigationHidden");
		return config.navigationHidden;
	}
}

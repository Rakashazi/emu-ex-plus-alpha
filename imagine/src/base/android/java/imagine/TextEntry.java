package com.imagine;

import android.widget.*;
import android.app.*;
import android.content.*;
import android.view.inputmethod.*;
import android.graphics.drawable.*;
import android.view.View.*;
import android.os.*;
import android.view.*;
import android.graphics.*;
import android.util.*;
import java.lang.reflect.*;

class TextEntry
{
	private static String logTag = "TextEntry";

	static final class TextEntryPopupWindow extends PopupWindow
	implements View.OnTouchListener, PopupWindow.OnDismissListener, TextView.OnEditorActionListener
	{
		EditText editBox;
		
		public TextEntryPopupWindow(EditText textView, int width, int height, boolean focusable)
		{
			super(textView, width, height, focusable);
			setBackgroundDrawable(new BitmapDrawable());
			setTouchInterceptor(this);
			setOnDismissListener(this);
			editBox = textView;
			editBox.setOnEditorActionListener(this);
		}
		
		@Override public void onDismiss()
		{
			//Log.i(logTag, "popup dismissed");
			editBox.setText(null);
			editBox.setImeActionLabel(null, 0);
			if(editBox.getId() == 0)
			{
				//Log.i(logTag, "text input canceled");
				BaseActivity.endSysTextInput(null);
			}
		}
		
		@Override public boolean onTouch(View v, MotionEvent event)
		{
			final int x = (int) event.getX();
			final int y = (int) event.getY();

			if (//(event.getAction() == MotionEvent.ACTION_DOWN) &&
				((x < 0) || (x >= getWidth()) || (y < 0) || (y >= getHeight())))
			{
				//Log.i(logTag, "popup touch outside " + x + "," + y);
				return true;
			}
			return false;
		}

		@Override public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
		{
			//Log.i(logTag, "got editor action " + actionId);
			BaseActivity.endSysTextInput(editBox.getText().toString());
			editBox.setId(1); // indicate text entry was not canceled
			dismiss();
			return false;
		}
	}
	
	private static TextEntryPopupWindow popup = null;

	static void startSysTextInput(Activity act, String initialText, String promptText, int x, int y, int width, int height)
	{
		if(popup == null)
		{
			popup = new TextEntryPopupWindow(new EditText(act.getApplicationContext()), width, height, true);
		}
		popup.editBox.setId(0); // reset indicator of canceled text entry
		popup.editBox.setText(initialText);
		popup.editBox.setImeActionLabel(promptText, 0);
		popup.editBox.setSingleLine();
		popup.showAtLocation(act.findViewById(android.R.id.content), Gravity.LEFT | Gravity.TOP, x, y);
	}
	
	static void finishSysTextInput(boolean canceled)
	{
		if(popup == null) return;
		popup.dismiss();
	}
	
	static void placeSysTextInput(int x, int y, int width, int height)
	{
		if(popup == null) return;
		popup.update(x, y, width, height);
	}
	
}
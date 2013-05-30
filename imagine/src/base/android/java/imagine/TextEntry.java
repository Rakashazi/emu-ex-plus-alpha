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
	private static final String logTag = "TextEntry";
	
	static final class TextEntryPopupWindow extends Dialog
	implements DialogInterface.OnDismissListener, TextView.OnEditorActionListener
	{
		EditText editBox;
		
		public TextEntryPopupWindow(Activity act, String initialText, String promptText, int x, int y, int width, int height, int fontSize)
		{
			super(act);
			editBox = new EditText(act);
			editBox.setId(0); // reset indicator of canceled text entry
			editBox.setTextSize(TypedValue.COMPLEX_UNIT_PX, fontSize);
			editBox.setText(initialText);
			editBox.setImeActionLabel(promptText, 0);
			editBox.setSingleLine();
			editBox.setOnEditorActionListener(this);
			getWindow().setBackgroundDrawable(new BitmapDrawable());
			getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
			getWindow().getAttributes().gravity = Gravity.LEFT | Gravity.TOP;
			requestWindowFeature(Window.FEATURE_NO_TITLE);
			setContentView(editBox);
			updateRect(x, y, width, height);
			setOnDismissListener(this);
			setCanceledOnTouchOutside(false);
			Log.i(logTag, "4");
		}
		
		void updateRect(int x, int y, int width, int height)
		{
			//Log.i(logTag, "setting popup size " + x + " " + y + " " + width + " " + height);
			WindowManager.LayoutParams p = getWindow().getAttributes();
			p.x = x;
			p.y = y;
			ViewGroup.LayoutParams vp = editBox.getLayoutParams();
			vp.width = width;
			vp.height = height;
			getWindow().setAttributes(p);
		}
		
		@Override public void onDismiss(DialogInterface dialog)
		{
			//Log.i(logTag, "popup dismissed");
			editBox.setText(null);
			editBox.setImeActionLabel(null, 0);
			if(editBox.getId() == 0)
			{
				//Log.i(logTag, "text input canceled");
				BaseActivity.endSysTextInput(null);
			}
			dismissedDialog();
		}
		
		/*@Override public boolean onTouchEvent(MotionEvent event)
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
		}*/

		@Override public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
		{
			//Log.i(logTag, "got editor action " + actionId);
			String content = editBox.getText().toString();
			editBox.setId(1); // indicate text entry was not canceled
			dismiss();
			BaseActivity.endSysTextInput(content);
			return false;
		}
	}
	
	private static TextEntryPopupWindow popup = null;

	static void startSysTextInput(Activity act, String initialText, String promptText, int x, int y, int width, int height, int fontSize)
	{
		if(popup != null)
		{
			finishSysTextInput(true);
		}
		popup = new TextEntryPopupWindow(act, initialText, promptText, x, y, width, height, fontSize);
		popup.show();
	}
	
	static void dismissedDialog()
	{
		popup = null;
	}
	
	static void finishSysTextInput(boolean canceled)
	{
		if(popup == null) return;
		popup.dismiss();
	}
	
	static void placeSysTextInput(int x, int y, int width, int height)
	{
		if(popup == null) return;
		popup.updateRect(x, y, width, height);
	}
	
}
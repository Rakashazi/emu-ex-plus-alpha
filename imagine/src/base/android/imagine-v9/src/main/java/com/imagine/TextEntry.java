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
	private static native void textInputEnded(long nativeUserData, String text, boolean processText, boolean isDoingDismiss);
	
	final class TextEntryPopupWindow extends Dialog
	implements DialogInterface.OnDismissListener, TextView.OnEditorActionListener
	{
		private static final int PROCESS_TEXT_ON_DISMISS = 0;
		private static final int SKIP_TEXT_ON_DISMISS = 1;
		private EditText editBox;
		private long nativeUserData;
		
		public TextEntryPopupWindow(Activity act, String initialText, String promptText,
			int x, int y, int width, int height, int fontSize, long nativeUserData)
		{
			super(act);
			this.nativeUserData = nativeUserData;
			editBox = new EditText(act);
			editBox.setId(PROCESS_TEXT_ON_DISMISS);
			editBox.setTextSize(TypedValue.COMPLEX_UNIT_PX, fontSize);
			editBox.setText(initialText);
			editBox.setImeActionLabel(promptText, 0);
			editBox.setSingleLine();
			editBox.setOnEditorActionListener(this);
			Window win = getWindow();
			win.setBackgroundDrawableResource(android.R.color.transparent);
			win.setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
				WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_DIM_BEHIND);
			win.getAttributes().gravity = Gravity.LEFT | Gravity.TOP;
			win.requestFeature(Window.FEATURE_NO_TITLE);
			setContentView(editBox);
			updateRect(x, y, width, height);
			setOnDismissListener(this);
			setCanceledOnTouchOutside(false);
		}

		void updateRect(int x, int y, int width, int height)
		{
			//Log.i(logTag, "setting popup size " + x + " " + y + " " + width + " " + height);
			WindowManager.LayoutParams p = getWindow().getAttributes();
			p.x = x;
			p.y = y;
			editBox.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);
			ViewGroup.LayoutParams vp = editBox.getLayoutParams();
			vp.width = width;
			vp.height = height + editBox.getExtendedPaddingTop() + editBox.getExtendedPaddingBottom();
			getWindow().setAttributes(p);
		}

		void requestLayout()
		{
			editBox.requestLayout();
		}

		@Override public void onDismiss(DialogInterface dialog)
		{
			//Log.i(logTag, "popup dismissed");
			//editBox.setText(null);
			//editBox.setImeActionLabel(null, 0);
			dismissedDialog();
			boolean processText = editBox.getId() == PROCESS_TEXT_ON_DISMISS; // check if text already processed in onEditorAction
			endTextInput(nativeUserData, null, processText, true);
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
			editBox.setId(SKIP_TEXT_ON_DISMISS);
			endTextInput(nativeUserData, content, true, false);
			dismiss();
			return false;
		}
	}
	
	private TextEntryPopupWindow popup = null;

	TextEntry(Activity act, String initialText, String promptText,
		int x, int y, int width, int height, int fontSize, long nativeUserData)
	{
		popup = new TextEntryPopupWindow(act, initialText, promptText, x, y, width, height, fontSize, nativeUserData);
		popup.show();
	}

	void finish(boolean canceled)
	{
		if(popup == null) return;
		popup.dismiss();
	}

	void place(int x, int y, int width, int height)
	{
		if(popup == null) return;
		popup.updateRect(x, y, width, height);
		popup.requestLayout();
	}

	private void dismissedDialog()
	{
		popup = null;
	}

	private void endTextInput(long nativeUserData, String text, boolean processText, boolean isDoingDismiss)
	{
		textInputEnded(nativeUserData, text, processText, isDoingDismiss);
	}
}
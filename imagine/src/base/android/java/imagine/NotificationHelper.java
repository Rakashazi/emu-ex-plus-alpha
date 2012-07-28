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

import android.app.*;
import android.content.*;

final class NotificationHelper
{
	private static NotificationManager notificationManager = null;
	static void addNotification(Context ctx, String onShow, String title, String message)
	{
		if(notificationManager == null)
			notificationManager = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
		int icon = 0x7f020000; // TODO: don't hard-code
		long when = System.currentTimeMillis();
		Notification notification = new Notification(icon, onShow, when);
		notification.flags |= /*Notification.FLAG_ONGOING_EVENT |*/ Notification.FLAG_AUTO_CANCEL;
		CharSequence contentTitle = title;
		CharSequence contentText = message;
		Intent notificationIntent = new Intent(ctx, BaseActivity.class);
		PendingIntent contentIntent = PendingIntent.getActivity(ctx, 0, notificationIntent, 0);

		notification.setLatestEventInfo(ctx, contentTitle, contentText, contentIntent);
		notificationManager.notify(1, notification);
	}
	
	static void removeNotification()
	{
		if(notificationManager != null)
			notificationManager.cancel(1);
	}
}
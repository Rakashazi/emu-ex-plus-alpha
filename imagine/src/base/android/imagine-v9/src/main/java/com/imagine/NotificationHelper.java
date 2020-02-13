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
import android.support.v4.app.NotificationCompat;

final class NotificationHelper
{
	static void addNotification(Context ctx, String onShow, String title, String message)
	{
		NotificationManager notificationManager = (NotificationManager)ctx.getSystemService(Context.NOTIFICATION_SERVICE);
		int icon = ctx.getResources().getIdentifier("icon", "mipmap", ctx.getPackageName());
		long when = System.currentTimeMillis();
		NotificationCompat.Builder builder = new NotificationCompat.Builder(ctx);
		Intent notificationIntent = new Intent(ctx, BaseActivity.class);
		PendingIntent contentIntent = PendingIntent.getActivity(ctx, 0, notificationIntent, 0);
		Notification notification =
			builder.setContentIntent(contentIntent)
				.setSmallIcon(icon)
				.setTicker(onShow)
				.setWhen(when)
				.setAutoCancel(true)
				.setContentTitle(title)
				.setContentText(message)
				.build();
		notificationManager.notify(1, notification);
	}

	static void removeNotification(Context ctx)
	{
		NotificationManager notificationManager = (NotificationManager)ctx.getSystemService(Context.NOTIFICATION_SERVICE);
		notificationManager.cancel(1);
	}
}
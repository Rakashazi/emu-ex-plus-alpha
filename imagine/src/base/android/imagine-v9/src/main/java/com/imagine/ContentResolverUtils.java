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

import android.content.ContentResolver;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.database.Cursor;
import android.util.Log;
import java.util.Date;
import java.text.DateFormat;
import java.io.File;

final class ContentResolverUtils
{
	private static final String logTag = "ContentResolverUtils";

	// File open flags, keep in sync with IO.hh
	static final int OPEN_READ = 1;
	static final int OPEN_WRITE = 1 << 1;
	static final int OPEN_KEEP_EXISTING = 1 << 3;

	static String fileOpenFlagsString(int flags)
	{
		final int usedFlags = OPEN_READ | OPEN_WRITE | OPEN_KEEP_EXISTING;
		switch(flags & usedFlags)
		{
			default:
			case OPEN_READ: return "r";
			case OPEN_WRITE|OPEN_KEEP_EXISTING: return "w";
			case OPEN_READ|OPEN_WRITE|OPEN_KEEP_EXISTING: return "rw";
			case OPEN_WRITE: return "wt";
			case OPEN_READ|OPEN_WRITE: return "rwt";
		}
	}

	static int openUriFd(ContentResolver resolver, String uriStr, int flags)
	{
		Uri uri = Uri.parse(uriStr);
		try
		{
			return resolver.openFileDescriptor(uri, fileOpenFlagsString(flags)).detachFd();
		}
		catch(Exception e)
		{
			if(android.os.Build.VERSION.SDK_INT >= 21 && (flags & OPEN_WRITE) != 0)
			{
				// no existing file, try creating it in the document part of the URI path
				final int docPos = uriStr.lastIndexOf("/document/");
				int subStrPos = uriStr.lastIndexOf("%2F");
				if(subStrPos == -1 || subStrPos < docPos) // no /, look for :
				{
					subStrPos = uriStr.lastIndexOf("%3A");
					if(subStrPos == -1 || subStrPos < docPos)
					{
						return -1;
					}
					subStrPos += 3;
				}
				final Uri pathUri = Uri.parse(uriStr.substring(0, subStrPos));
				final File file = new File(uri.getPath());
				final String name = file.getName();
				return openDocumentUriFd(resolver, pathUri, name, flags);
			}
			//Log.i(logTag, "openUriFd exception:" + e.toString());
			return -1;
		}
	}

	static int openDocumentUriFd(ContentResolver resolver, Uri pathUri, String name, int flags)
	{
		try
		{
			String mimeType = "application/octet-stream";
			if(name.endsWith(".png"))
				mimeType = "image/png";
			final Uri docUri = DocumentsContract.createDocument(resolver, pathUri, mimeType, name);
			return resolver.openFileDescriptor(docUri, fileOpenFlagsString(flags)).detachFd();
		}
		catch(Exception e)
		{
			//Log.i(logTag, "openDocumentUriFd exception:" + e.toString());
			return -1;
		}
	}

	static boolean uriExists(ContentResolver resolver, String uriStr)
	{
		try(Cursor c = resolver.query(Uri.parse(uriStr),
			new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID}, null, null, null);)
		{
			return c.getCount() > 0;
		}
		catch(Exception e)
		{
			//Log.i(logTag, "uriExists exception:" + e.toString());
			return false;
		}
	}

	static String uriLastModified(ContentResolver resolver, String uriStr)
	{
		final long mTime = queryLong(resolver, Uri.parse(uriStr), DocumentsContract.Document.COLUMN_LAST_MODIFIED, 0);
		if(mTime == 0)
			return "";
		return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(new Date(mTime));
	}

	static String uriDisplayName(ContentResolver resolver, Uri uri)
	{
		return queryString(resolver, uri, DocumentsContract.Document.COLUMN_DISPLAY_NAME);
	}

	static String uriDisplayName(ContentResolver resolver, String uriStr)
	{
		return uriDisplayName(resolver, Uri.parse(uriStr));
	}

	static boolean deleteUri(ContentResolver resolver, String uriStr)
	{
		try
		{
			return DocumentsContract.deleteDocument(resolver, Uri.parse(uriStr));
		}
		catch(Exception e)
		{
			//Log.i(logTag, "deleteUri exception:" + e.toString());
			return false;
		}
	}

	static boolean listUriFiles(ContentResolver resolver, long nativeUserData, String uriStr)
	{
		final Uri uri = Uri.parse(uriStr);
		final Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(uri, DocumentsContract.getDocumentId(uri));
		try(final Cursor c = resolver.query(childrenUri,
			new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID, DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE},
			null, null, null);)
		{
			while(c.moveToNext())
			{
				final String documentId = c.getString(0);
				final String displayName = c.getString(1);
				final String mimeType = c.getString(2);
				final boolean isDir = mimeType.equals(DocumentsContract.Document.MIME_TYPE_DIR);
				final Uri documentUri = DocumentsContract.buildDocumentUriUsingTree(uri, documentId);
				if(!BaseActivity.uriFileListed(nativeUserData, documentUri.toString(), displayName, isDir))
					break;
			}
			return true;
		}
		catch(Exception e)
		{
			//Log.i(logTag, "listUriFiles exception:" + e.toString());
			return false;
		}
	}

	static long queryLong(ContentResolver resolver, Uri uri, String column, long defaultValue)
	{
		try(final Cursor c = resolver.query(uri, new String[] {column}, null, null, null))
		{
			if(!c.moveToFirst())
				return defaultValue;
			return c.getLong(0);
		}
		catch(Exception e)
		{
			return defaultValue;
		}
	}

	static String queryString(ContentResolver resolver, Uri uri, String column)
	{
		try(final Cursor c = resolver.query(uri, new String[] {column}, null, null, null);)
		{
			if(!c.moveToFirst())
				return "";
			String str = c.getString(0);
			return str != null ? str : "";
		}
		catch(Exception e)
		{
			return "";
		}
	}
}
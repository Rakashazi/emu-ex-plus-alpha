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
import java.io.File;

final class ContentResolverUtils
{
	private static final String logTag = "ContentResolverUtils";

	// File open flags, keep in sync with IO.hh
	static final int READ_BIT = 1;
	static final int WRITE_BIT = 1 << 1;
	static final int CREATE_BIT = 1 << 2;
	static final int TRUNCATE_BIT = 1 << 3;

	static String fileOpenFlagsString(int flags)
	{
		final int usedFlags = READ_BIT | WRITE_BIT | TRUNCATE_BIT;
		switch(flags & usedFlags)
		{
			default:
			case READ_BIT: return "r";
			case WRITE_BIT: return "w";
			case READ_BIT|WRITE_BIT: return "rw";
			case WRITE_BIT|TRUNCATE_BIT: return "wt";
			case READ_BIT|WRITE_BIT|TRUNCATE_BIT: return "rwt";
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
			if(android.os.Build.VERSION.SDK_INT >= 21 && (flags & CREATE_BIT) != 0)
			{
				// no existing file, try creating it in the document part of the URI path
				final Uri dirUri = documentUriDir(uriStr);
				if(dirUri == null)
					return -1;
				return openDocumentUriFd(resolver, dirUri, documentUriName(uriStr), flags);
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

	static boolean createDirUri(ContentResolver resolver, String uriStr)
	{
		try
		{
			if(uriExists(resolver, uriStr))
				return false;
			final Uri dirUri = documentUriDir(uriStr);
			if(dirUri == null)
				return false;
			DocumentsContract.createDocument(resolver, dirUri,
				DocumentsContract.Document.MIME_TYPE_DIR, documentUriName(uriStr));
			return true;
		}
		catch(Exception e)
		{
			//Log.i(logTag, "createFolder exception:" + e.toString());
			return false;
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
		final long mTime = uriLastModifiedTime(resolver, uriStr);
		if(mTime == 0)
			return "";
		return BaseActivity.formatDateTime(mTime);
	}

	static long uriLastModifiedTime(ContentResolver resolver, String uriStr)
	{
		return queryLong(resolver, Uri.parse(uriStr), DocumentsContract.Document.COLUMN_LAST_MODIFIED, 0);
	}

	static String uriDisplayName(ContentResolver resolver, Uri uri)
	{
		return queryString(resolver, uri, DocumentsContract.Document.COLUMN_DISPLAY_NAME);
	}

	static String uriDisplayName(ContentResolver resolver, String uriStr)
	{
		return uriDisplayName(resolver, Uri.parse(uriStr));
	}

	static String uriMimeType(ContentResolver resolver, String uriStr)
	{
		return queryString(resolver, Uri.parse(uriStr), DocumentsContract.Document.COLUMN_MIME_TYPE);
	}

	static boolean deleteUri(ContentResolver resolver, String uriStr, boolean isDir)
	{
		try
		{
			final Uri uri = Uri.parse(uriStr);
			if(isDir) // make sure directory is empty
			{
				final Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(uri, DocumentsContract.getDocumentId(uri));
				try(final Cursor c = resolver.query(childrenUri,
					new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID},
					null, null, null);)
				{
					if(c.getCount() > 0)
						return false;
				}
			}
			return DocumentsContract.deleteDocument(resolver, uri);
		}
		catch(Exception e)
		{
			//Log.i(logTag, "deleteUri exception:" + e.toString());
			return false;
		}
	}

	static boolean renameUri(ContentResolver resolver, String oldUriStr, String newUriStr)
	{
		final Uri oldUriDir = documentUriDir(oldUriStr);
		final Uri newUriDir = documentUriDir(newUriStr);
		if(oldUriDir == null || newUriDir == null)
			return false;
		final String oldName = documentUriName(oldUriStr);
		final String newName = documentUriName(newUriStr);
		Uri oldUri = Uri.parse(oldUriStr);
		if(!oldName.equals(newName))
		{
			try
			{
				oldUri = DocumentsContract.renameDocument(resolver, oldUri, newName);
			}
			catch(Exception e)
			{
				//Log.i(logTag, "renameDocument exception:" + e.toString());
				return false;
			}
		}
		if(!oldUriDir.equals(newUriDir))
		{
			try
			{
				DocumentsContract.moveDocument(resolver, oldUri, oldUriDir, newUriDir);
			}
			catch(Exception e)
			{
				//Log.i(logTag, "moveDocument exception:" + e.toString());
				return false;
			}
		}
		return true;
	}

	static boolean listUriFiles(ContentResolver resolver, long nativeUserData, String uriStr)
	{
		final Uri uri = Uri.parse(uriStr);
		try(final Cursor c = resolver.query(DocumentsContract.buildChildDocumentsUriUsingTree(uri, DocumentsContract.getDocumentId(uri)),
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

	static Uri documentUriDir(String uriStr)
	{
		final int docPos = uriStr.lastIndexOf("/document/");
		int subStrPos = uriStr.lastIndexOf("%2F");
		if(subStrPos == -1 || subStrPos < docPos) // no /, look for :
		{
			subStrPos = uriStr.lastIndexOf("%3A");
			if(subStrPos == -1 || subStrPos < docPos)
			{
				return null;
			}
			subStrPos += 3;
		}
		return Uri.parse(uriStr.substring(0, subStrPos));
	}

	static String documentUriName(Uri uri)
	{
		final File file = new File(uri.getPath());
		return file.getName();
	}

	static String documentUriName(String uriStr)
	{
		return documentUriName(Uri.parse(uriStr));
	}
}
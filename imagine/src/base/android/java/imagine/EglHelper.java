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
import android.util.Log;

final class EglHelper
{
	public boolean configHasAlpha;
	private boolean contextIsCurrent;
	
	private static EGL10 mEgl;
	private static EGLDisplay mEglDisplay;
	private static EGLSurface mEglSurface;
	private static EGLConfig mEglConfig;
	public static EGLContext mEglContext;
	
	public EglHelper() { }

	// config chooser
	private EGLConfig chooseConfig(EGL10 egl, EGLDisplay display)
	{
		int[] configSpec = new int[] {
		        //EGL10.EGL_RED_SIZE, 8,
		        //EGL10.EGL_GREEN_SIZE, 8,
		        //EGL10.EGL_BLUE_SIZE, 8,
				//EGL10.EGL_ALPHA_SIZE, 1,
				//EGL10.EGL_DEPTH_SIZE, 1,
				//EGL10.EGL_STENCIL_SIZE, 1,
		        //EGL10.EGL_CONFIG_CAVEAT, EGL10.EGL_NONE,
				//EGL10.EGL_TRANSPARENT_TYPE, EGL10.EGL_TRANSPARENT_RGB,
		        //EGL10.EGL_NATIVE_RENDERABLE, 1,
		        EGL10.EGL_NONE};
		
		int[] configsRet = new int[1];
		if(!egl.eglChooseConfig(display, configSpec, null, 0, configsRet))
		{
		    throw new IllegalArgumentException("eglChooseConfig failed");
		}
		
		int configs = configsRet[0];
		//Log.i("EGLConfig", configs + " configs returned");
		if(configs == 0)
			return null;
		
		EGLConfig[] config = new EGLConfig[configs];
		if(!egl.eglChooseConfig(display, configSpec, config, configs, configsRet))
		{
			throw new IllegalArgumentException("eglChooseConfig 2 failed");
		}
		//printConfigs(egl, display, config);
		configHasAlpha = getConfigAttrib(egl, display, config[0], EGL10.EGL_ALPHA_SIZE) > 0 ? true : false;
		return config[0];
		
		/*EGLConfig config = chooseConfig(egl, display, configs, 5, 6, 5, 0, 0, 0);
		if (config == null)
		{
		    throw new IllegalArgumentException("No config chosen");
		}
		return config;*/
	}
	
	private void printConfigs(EGL10 e, EGLDisplay d, EGLConfig[] config)
	{
		for (EGLConfig c : config)
		{
			int cav = getConfigAttrib(e, d, c, EGL10.EGL_CONFIG_CAVEAT);
			Log.v("EGLConfig",
				"Mode r:" + getConfigAttrib(e, d, c, EGL10.EGL_RED_SIZE)
				+ " g:" + getConfigAttrib(e, d, c, EGL10.EGL_GREEN_SIZE)
				+ " b:" + getConfigAttrib(e, d, c, EGL10.EGL_BLUE_SIZE)
				+ " a:" + getConfigAttrib(e, d, c, EGL10.EGL_ALPHA_SIZE)
				+ " d:" + getConfigAttrib(e, d, c, EGL10.EGL_DEPTH_SIZE)
				+ " s:" + getConfigAttrib(e, d, c, EGL10.EGL_STENCIL_SIZE)
				+ " cav:" + (cav == EGL10.EGL_SLOW_CONFIG ? "slow" : cav == EGL10.EGL_NON_CONFORMANT_CONFIG ? "nonc" : "no")
				+ " nrend:" + ((getConfigAttrib(e, d, c, EGL10.EGL_NATIVE_RENDERABLE) == 0) ? "no" : "yes")
				//+ " v:" + getConfigAttrib(e, d, c, EGL10.EGL_NATIVE_VISUAL_TYPE)
				+ " size:" + getConfigAttrib(e, d, c, EGL10.EGL_BUFFER_SIZE)
				//+ " lvl:" + getConfigAttrib(e, d, c, EGL10.EGL_LEVEL)
				+ " smpl:" + getConfigAttrib(e, d, c, EGL10.EGL_SAMPLES)
				//+ " surf:" + getConfigAttrib(e, d, c, EGL10.EGL_SURFACE_TYPE)
				//+ " trans:" + getConfigAttrib(e, d, c, EGL10.EGL_TRANSPARENT_TYPE)
				+ " id:" + getConfigAttrib(e, d, c, EGL10.EGL_CONFIG_ID)
				);
		}
	}
	
	private int getConfigAttrib(EGL10 egl, EGLDisplay display, EGLConfig config, int attribute)
	{
		int[] valRet = new int[1];
		if(!egl.eglGetConfigAttrib(display, config, attribute, valRet))
		{
			throw new IllegalArgumentException("eglGetConfigAttrib failed");
		}
		return valRet[0];
	}

	// context
	private EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig config)
	{
		// for ES 2.0 support
		int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
		int mEGLContextClientVersion = 0;
		int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, mEGLContextClientVersion,
		        EGL10.EGL_NONE };

		return egl.eglCreateContext(display, config, EGL10.EGL_NO_CONTEXT,
		        mEGLContextClientVersion != 0 ? attrib_list : null);
	}

	private void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context)
	{
		if (!egl.eglDestroyContext(display, context))
		{
		    throw new RuntimeException("eglDestroyContext failed");//+ EGLLogWrapper.getErrorString(egl.eglGetError()));
		}
	}
	
	public boolean verifyContext()
	{
		EGLContext currentContext = mEgl.eglGetCurrentContext();
		if(currentContext != EGL10.EGL_NO_CONTEXT && mEgl.eglGetError() != EGL11.EGL_CONTEXT_LOST)
		{
			return true;
		}
		else
		{
			mEglContext = null;
			return false;
		}
	}

	public void start()
	{
		mEgl = (EGL10) EGLContext.getEGL();

		mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		if (mEglDisplay == EGL10.EGL_NO_DISPLAY)
		{
		    throw new RuntimeException("eglGetDisplay failed");
		}
		
		//Log.i("EglHelper", "Extensions: " + mEgl.eglQueryString(mEglDisplay, EGL10.EGL_EXTENSIONS));

		// initialize EGL with the display
		int[] version = new int[2];
		if(!mEgl.eglInitialize(mEglDisplay, version))
		{
		    throw new RuntimeException("eglInitialize failed");
		}
		//Log.i("EglHelper", "EGL version " + version[0] + "." + version[1]);
		mEglConfig = chooseConfig(mEgl, mEglDisplay);
		
		if(mEglConfig == null)
		{
			throw new RuntimeException("failed to find any egl configs");
		}

		mEglContext = createContext(mEgl, mEglDisplay, mEglConfig);
		if (mEglContext == null || mEglContext == EGL10.EGL_NO_CONTEXT)
		{
		    mEglContext = null;
		    throwEglException("createContext");
		}
		contextIsCurrent = false;
		
		mEglSurface = null;
	}

	public void createSurface(SurfaceHolder holder)
	{
		/*
		 * Check preconditions.
		 */
		if (mEgl == null)
		{
		    throw new RuntimeException("egl not initialized");
		}
		if (mEglDisplay == null)
		{
		    throw new RuntimeException("eglDisplay not initialized");
		}
		if (mEglConfig == null)
		{
		    throw new RuntimeException("mEglConfig not initialized");
		}
		/*
		 *  The window size has changed, so we need to create a new
		 *  surface.
		 */
		if (mEglSurface != null && mEglSurface != EGL10.EGL_NO_SURFACE)
		{
		    /*
		     * Unbind and destroy the old EGL surface, if
		     * there is one.
		     */
		    mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE,
		            EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
		    mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
		}

		mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, mEglConfig, holder, null);

		if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE)
		{
		    int error = mEgl.eglGetError();
		    if (error == EGL10.EGL_BAD_NATIVE_WINDOW)
		    {
		        //Log.e("EglHelper", "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
		        return;
		    }
		    throw new RuntimeException("createWindowSurface " + error);
		}

		/*
		 * Before we can issue GL commands, we need to make sure
		 * the context is current and bound to a surface.
		 */
		if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext))
		{
		    throwEglException("eglMakeCurrent");
		}
		contextIsCurrent = true;
	}

	public void swap()
	{
		if (! mEgl.eglSwapBuffers(mEglDisplay, mEglSurface))
		{
			//throwEglException("eglSwapBuffers");
		    /*
		     * Check for EGL_CONTEXT_LOST, which means the context
		     * and all associated data were lost (For instance because
		     * the device went to sleep). We need to sleep until we
		     * get a new surface.
		     */
		    /*int error = mEgl.eglGetError();
		    switch(error) {
		    case EGL11.EGL_CONTEXT_LOST:
		        return false;
		    case EGL10.EGL_BAD_NATIVE_WINDOW:
		        // The native window is bad, probably because the
		        // window manager has closed it. Ignore this error,
		        // on the expectation that the application will be closed soon.
		        Log.e("EglHelper", "eglSwapBuffers returned EGL_BAD_NATIVE_WINDOW. tid=" + Thread.currentThread().getId());
		        break;
		    default:
		        throwEglException("eglSwapBuffers", error);
		    }*/
		}
	}

	public void destroySurface()
	{
		if (mEglSurface != null && mEglSurface != EGL10.EGL_NO_SURFACE)
		{
		    mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE,
		            EGL10.EGL_NO_SURFACE,
		            EGL10.EGL_NO_CONTEXT);
		    mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
		    mEglSurface = null;
		}
	}

	public void finish()
	{
		if (mEglContext != null)
		{
			mEgl.eglDestroyContext(mEglDisplay, mEglContext);
		    mEglContext = null;
		}
		if (mEglDisplay != null)
		{
		    mEgl.eglTerminate(mEglDisplay);
		    mEglDisplay = null;
		}
	}

	private void throwEglException(String function)
	{
		throw new RuntimeException(function + " failed: " + mEgl.eglGetError());
	}
}

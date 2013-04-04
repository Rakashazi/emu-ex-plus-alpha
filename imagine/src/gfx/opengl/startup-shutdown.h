#pragma once

#include <gfx/Gfx.hh>
#include <base/Base.hh>
#include <assert.h>

#include <util/time/sys.hh>
static TimeSys startFrameTime;//, halfFrameTime;//, oneFrameTime, firstOneFrameTime;

#include "settings.h"

//#include "geometry-test.h"

namespace Gfx
{

CallResult setOutputVideoMode(const Base::Window &win)
{
	logMsg("running init");

	if(checkGLErrorsVerbose)
		logMsg("using verbose error checks");
	else if(checkGLErrors)
		logMsg("using error checks");

	if(animateOrientationChange)
	{
		projAngleM.init(0);
	}

	//logMsg("resizing viewport to %dx%d", x, y);
	resizeGLScene(win);
	
	auto extensions = (const char*)glGetString(GL_EXTENSIONS);
	assert(extensions);
	auto version = (const char*)glGetString(GL_VERSION);
	assert(version);
	auto rendererName = (const char*)glGetString(GL_RENDERER);
	logMsg("version: %s (%s)\nextensions: %s", version, rendererName, extensions);
	
	#ifndef CONFIG_GFX_OPENGL_ES
		#if !defined CONFIG_BASE_X11 && !defined CONFIG_BASE_MACOSX
		GLenum err = glewInit();
		if (err != GLEW_OK)
		{
			logErr("could not init Glew, error: %s", glewGetErrorString(err));
			return(INVALID_PARAMETER);
		}
		#endif
	
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	auto dotPos = strchr(version, '.');
	int majorVer = dotPos[-1]-'0' , minorVer = dotPos[1]-'0';
	const bool hasGL1_5 = (majorVer > 1) || (majorVer == 1 && minorVer >= 5);
	const bool hasGL1_4 = hasGL1_5 || (majorVer == 1 && minorVer >= 4);
	const bool hasGL1_3 = hasGL1_4 || (majorVer == 1 && minorVer >= 3);
	const bool hasGL1_2 = hasGL1_3 || (majorVer == 1 && minorVer >= 2);
	const bool hasGL1_1 = hasGL1_2 || (majorVer == 1 && minorVer >= 1);
	assert(hasGL1_1); // needed for Vertex Arrays
	#else
	const bool hasGL1_3 = 0;
	const bool hasGL1_5 = 0;
	#endif
	
	setClearColor(0., 0., 0.);
	if(Config::envIsAndroid || Config::envIsWebOS)
	{
		//glDisable(GL_MULTISAMPLE);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	}
	//glcEnable(GL_TEXTURE_2D);
	//glShadeModel(GL_SMOOTH);
	//glcEnable(GL_DEPTH_TEST);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	setVisibleGeomFace(FRONT_FACES);
	//setVisibleGeomFace(BOTH_FACES);

	#ifndef CONFIG_GFX_OPENGL_ES
	//glcAlphaFunc(GL_GREATER,0.0f);
	//glcEnable(GL_ALPHA_TEST);
	#endif

	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	textureSizeSupport.maxXSize = textureSizeSupport.maxYSize = texSize;
	assert(textureSizeSupport.maxXSize > 0 && textureSizeSupport.maxYSize > 0);
	logMsg("max texture size is %d", texSize);

	vsyncEnable();
	checkForAnisotropicFiltering(extensions);
	checkForAutoMipmapGeneration(extensions, version, rendererName);
	checkForMultisample(extensions);
	checkForNonPow2Textures(extensions, rendererName);
	checkForBGRPixelSupport(extensions);
	checkForCompressedTexturesSupport(hasGL1_3);
	checkForFBOFuncs(extensions);
	checkForVBO(version, hasGL1_5);
	if(useFBOFuncs) useAutoMipmapGeneration = 0; // prefer FBO mipmap function if present

	/*#ifdef CONFIG_GFX_OPENGL_ES
	if(strstr(extensions, "GL_EXT_discard_framebuffer") != NULL)
	{
		discardFrameBuffer = 1;
		logMsg("framebuffer discarding supported");
	}
	#endif*/

	#if defined CONFIG_BASE_ANDROID
		//checkForDrawTexture(extensions, rendererName);
		setupAndroidOGLExtensions(extensions, rendererName);
	#endif

	/*#ifdef CONFIG_GFX_OPENGL_GLX
	if(GLXEW_SGI_video_sync)
	{
		useSGIVidSync = 1;
		glXGetVideoSyncSGI(&gfx_frameTime);
	}
	else
	#endif
	{
		//logMsg("no video sync counter, using system time");
		startFrameTime.setTimeNow();
		//halfFrameTime.setUSecs(16000/2);
		//oneFrameTime.setUSecs(16666);
		//firstOneFrameTime.setUSecs(19500);
		//gfx_frameClockTime = 0;
	}*/

	glcEnableClientState(GL_VERTEX_ARRAY);
	return OK;
}

#ifdef CONFIG_BASE_ANDROID
static void setupAndroidOGLExtensions(const char *extensions, const char *rendererName)
{
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		if(Base::androidSDK() < 14)
			directTextureConf.checkForEGLImageKHR(extensions, rendererName);
	#endif
	#ifdef CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
		if(surfaceTextureConf.isSupported())
		{
			if(!strstr(extensions, "GL_OES_EGL_image_external"))
			{
				logWarn("SurfaceTexture is supported but OpenGL extension missing, disabling");
				surfaceTextureConf.deinit();
			}
			else if(strstr(rendererName, "Adreno"))
			{
				if(strstr(rendererName, "200")) // Textures may stop updating on HTC EVO 4G (supersonic) on Android 4.1
				{
					logWarn("buggy SurfaceTexture implementation, disabling by default");
					surfaceTextureConf.use = surfaceTextureConf.whiteListed = 0;
				}

				// When deleting a SurfaceTexture, Adreno 225 on Android 4.0 will unbind
				// the current GL_TEXTURE_2D texture, even though its state shouldn't change.
				// This hack will fix-up the GL state cache manually when that happens.
				logWarn("enabling SurfaceTexture GL_TEXTURE_2D binding hack");
				surfaceTextureConf.texture2dBindingHack = 1;
			}
		}
	#endif
}
#endif

}

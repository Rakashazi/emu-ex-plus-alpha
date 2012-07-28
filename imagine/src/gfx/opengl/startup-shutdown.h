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

CallResult init()
{
	logMsg("running init");

	#ifndef CHECK_GL_ERRORS
		logMsg("error checking off");
	#endif

	if( Base::openGLInit() != OK )
		return (INIT_ERROR);

	#if defined(USE_TEX_CACHE)
		texReuseCache_initWithArray(&texCache, texCacheStore, TEX_CACHE_ENTRIES);
	#endif

	//log_mPrintf(LOGGER_MESSAGE,"done init");

	//geom_test_init();

	if(animateOrientationChange)
	{
		projAngleM.init(0);
	}

	return OK;
}

CallResult setOutputVideoMode(const Base::Window &win)
{
	if(forceNoMultisample)
	{
		Base::openGLSetOutputVideoMode(win);
	}
	else
	{
		if(Base::openGLSetMultisampleVideoMode(win) != OK)
		{
			logMsg("multisample video mode not supported");
			forceNoMultisample = 1;
			Base::openGLSetOutputVideoMode(win);
		}
	}

	#if defined(CONFIG_ENV_WEBOS)
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE); // avoid blending with the video layer
	#endif

	//logMsg("resizing viewport to %dx%d", x, y);
	resizeGLScene(win);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // WebOS does clear in base module
	
	extensions = (const char*)glGetString(GL_EXTENSIONS);
	assert(extensions != 0);
	version = (const char*)glGetString(GL_VERSION);
	rendererName = (const char*)glGetString(GL_RENDERER);
	logMsg("version: %s (%s)\nextensions: %s", version, rendererName, extensions);
	
	#ifndef CONFIG_GFX_OPENGL_ES
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		logErr("could not init Glew, error: %s", glewGetErrorString(err));
		return(INVALID_PARAMETER);
	}
	
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	#endif
	
	if(Config::envIsAndroid || Config::envIsWebOS)
	{
		glDisable(GL_MULTISAMPLE);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	}
	//glcEnable(GL_TEXTURE_2D);
	//glShadeModel(GL_SMOOTH);
	//glcEnable(GL_DEPTH_TEST);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	setVisibleGeomFace(FRONT_FACES);
	//setVisibleGeomFace(BOTH_FACES);

	#ifndef CONFIG_GFX_OPENGL_ES
	//glAlphaFunc(GL_GREATER,0.0f);
	//glEnable(GL_ALPHA_TEST);
	#endif

	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	textureSizeSupport.maxXSize = textureSizeSupport.maxYSize = texSize;
	assert(textureSizeSupport.maxXSize > 0 && textureSizeSupport.maxYSize > 0);
	logMsg("max texture size is %d", texSize);

	vsyncEnable();
	checkForAnisotropicFiltering();
	checkForAutoMipmapGeneration();
	checkForMultisample();
	checkForVertexArrays();
	checkForNonPow2Textures();
	checkForBGRPixelSupport();
	checkForTextureClampToEdge();
	checkForCompressedTexturesSupport();
	checkForFBOFuncs();
	checkForVBO();
	if(useFBOFuncs) useAutoMipmapGeneration = 0; // prefer FBO mipmap function if present

	/*#ifdef CONFIG_GFX_OPENGL_ES
	if(strstr(extensions, "GL_EXT_discard_framebuffer") != NULL)
	{
		discardFrameBuffer = 1;
		logMsg("framebuffer discarding supported");
	}
	#endif*/

	#if defined(CONFIG_GFX_OPENGL_ES) && !defined(CONFIG_BASE_PS3)
		//checkForDrawTexture();
	#endif

	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		#ifdef CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
			if(!Base::hasSurfaceTexture())
		#endif
				directTextureConf.checkForEGLImageKHR(rendererName);
	#endif

	#ifdef CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
		if(Base::hasSurfaceTexture() && !strstr(extensions, "GL_OES_EGL_image_external"))
		{
			logWarn("SurfaceTexture is supported but OpenGL extension missing, disabling");
			Base::disableSurfaceTexture();
		}
	#endif

	#ifdef CONFIG_GFX_OPENGL_GLX
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
	}

	glcEnableClientState(GL_VERTEX_ARRAY);

	glClear(GL_COLOR_BUFFER_BIT);
	return OK;
}

}

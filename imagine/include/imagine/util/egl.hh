#pragma once

#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif

#include <EGL/egl.h>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>

#ifndef EGL_CONTEXT_MAJOR_VERSION_KHR
#define EGL_CONTEXT_MAJOR_VERSION_KHR 0x3098
#endif
#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR 0x00000001
#endif
#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif

static const char* eglSurfaceTypeToStr(EGLint type)
{
	switch(type & 0x7)
	{
		case EGL_WINDOW_BIT: return "window";
		case EGL_PBUFFER_BIT: return "pbuffer";
		case EGL_PIXMAP_BIT: return "pixmap";
		case EGL_WINDOW_BIT|EGL_PBUFFER_BIT: return "window|pbuffer";
		case EGL_WINDOW_BIT|EGL_PIXMAP_BIT: return "window|pixmap";
		case EGL_PBUFFER_BIT|EGL_PIXMAP_BIT: return "pbuffer|pixmap";
		case EGL_WINDOW_BIT|EGL_PBUFFER_BIT|EGL_PIXMAP_BIT: return "window|pbuffer|pixmap";
	}
	return "unknown";
}

static const char* eglRenderableTypeToStr(EGLint type)
{
	switch(type & 0x4F)
	{
		case EGL_OPENGL_BIT: return "GL";
		case EGL_OPENGL_ES_BIT: return "ES";
		case EGL_OPENGL_ES2_BIT: return "ES2";
		case EGL_OPENGL_ES3_BIT: return "ES3";
		case EGL_OPENVG_BIT: return "VG";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES_BIT: return "GL|ES";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT: return "GL|ES2|ES3";
		case EGL_OPENGL_BIT|EGL_OPENVG_BIT: return "GL|VG";
		case EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT: return "ES|ES2";
		case EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT: return "ES|ES2|ES3";
		case EGL_OPENGL_ES_BIT|EGL_OPENVG_BIT: return "ES|VG";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT: return "GL|ES|ES2";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT: return "GL|ES|ES2|ES3";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES_BIT|EGL_OPENVG_BIT: return "GL|ES|VG";
		case EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENVG_BIT: return "ES|ES2|VG";
		case EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT|EGL_OPENVG_BIT: return "ES|ES2|ES3|VG";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENVG_BIT: return "GL|ES|ES2|VG";
		case EGL_OPENGL_BIT|EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT|EGL_OPENVG_BIT: return "GL|ES|ES2|ES3|VG";
	}
	return "unknown";
}

static const char* eglConfigCaveatToStr(EGLint cav)
{
	switch(cav)
	{
		case EGL_NONE: return "none";
		case EGL_SLOW_CONFIG: return "slow";
		case EGL_NON_CONFORMANT_CONFIG: return "non-conformant";
	}
	return "unknown";
}

static void printEGLConf(EGLDisplay display, EGLConfig config)
{
	EGLint id, buffSize, redSize, greenSize, blueSize, alphaSize, cav, depthSize, stencilSize, nID, nRend,
		sType, minSwap, maxSwap, sampleBuff, renderType;
	eglGetConfigAttrib(display, config, EGL_CONFIG_ID, &id);
	eglGetConfigAttrib(display, config, EGL_BUFFER_SIZE, &buffSize);
	eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
	eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &greenSize);
	eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blueSize);
	eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alphaSize);
	eglGetConfigAttrib(display, config, EGL_CONFIG_CAVEAT, &cav);
	eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depthSize);
	eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencilSize);
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nID);
	eglGetConfigAttrib(display, config, EGL_NATIVE_RENDERABLE, &nRend);
	eglGetConfigAttrib(display, config, EGL_SURFACE_TYPE, &sType);
	eglGetConfigAttrib(display, config, EGL_RENDERABLE_TYPE, &renderType);
	eglGetConfigAttrib(display, config, EGL_MIN_SWAP_INTERVAL, &minSwap);
	eglGetConfigAttrib(display, config, EGL_MAX_SWAP_INTERVAL, &maxSwap);
	eglGetConfigAttrib(display, config, EGL_SAMPLE_BUFFERS, &sampleBuff);
	logMsg("config:0x%X %d:%d:%d:%d (%d) cav:%s(0x%X) d:%d sten:%d nid:%d nrend:%d stype:%s(0x%X) rtype:%s(0x%X) sampleBuff:%d swap:%d-%d",
			id, redSize, greenSize, blueSize, alphaSize, buffSize,
			eglConfigCaveatToStr(cav), cav, depthSize, stencilSize,
			nID, nRend,eglSurfaceTypeToStr(sType), sType,
			eglRenderableTypeToStr(renderType), renderType, sampleBuff,
			minSwap, maxSwap);
}

static void printEGLConfs(EGLDisplay display)
{
	EGLConfig conf[96];
	EGLint num = 0;
	eglGetConfigs(display, conf, std::size(conf), &num);
	logMsg("EGLDisplay has %d configs:", num);
	iterateTimes(num, i)
	{
		printEGLConf(display, conf[i]);
	}
}

static void printEGLConfsWithAttr(EGLDisplay display, const EGLint *attr)
{
	EGLConfig conf[96];
	EGLint num = 0;
	eglChooseConfig(display, attr, conf, std::size(conf), &num);
	logMsg("got %d configs", num);
	iterateTimes(num, i)
	{
		printEGLConf(display, conf[i]);
	}
}

static EGLBoolean eglSurfaceIsValid(EGLDisplay display, EGLSurface surface)
{
	EGLint dummy;
	return eglQuerySurface(display, surface, EGL_CONFIG_ID, &dummy);
}

static EGLSurface makeDummyPbuffer(EGLDisplay display, EGLConfig config)
{
	const EGLint attribs[]{EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
	return eglCreatePbufferSurface(display, config, attribs);
}

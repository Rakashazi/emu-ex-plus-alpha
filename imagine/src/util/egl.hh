#pragma once

#include <EGL/egl.h>
#include <logger/interface.h>
#include <util/cLang.h>

static const EGLint eglAttrWinLowColor[] =
{
	EGL_NONE
};

static const EGLint eglAttrWinRGB888[] =
{
	EGL_CONFIG_CAVEAT, EGL_NONE,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_NONE
};

static const EGLint eglAttrWinMaxRGBA[] =
{
	EGL_CONFIG_CAVEAT, EGL_NONE,
	EGL_BLUE_SIZE, 1,
	EGL_GREEN_SIZE, 1,
	EGL_RED_SIZE, 1,
	EGL_ALPHA_SIZE, 1,
	EGL_NONE
};

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
	EGLint buffSize, redSize, greenSize, blueSize, alphaSize, cav, depthSize, stencilSize, nID, nRend,
		sType, minSwap, maxSwap, sampleBuff;
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
	eglGetConfigAttrib(display, config, EGL_MIN_SWAP_INTERVAL, &minSwap);
	eglGetConfigAttrib(display, config, EGL_MAX_SWAP_INTERVAL, &maxSwap);
	eglGetConfigAttrib(display, config, EGL_SAMPLE_BUFFERS, &sampleBuff);
	logMsg("config %d %d:%d:%d:%d cav:%s(0x%X) d:%d sten:%d nid:%d nrend:%d stype:%s(0x%X) sampleBuff:%d swap:%d-%d",
			buffSize, redSize, greenSize, blueSize, alphaSize,
			eglConfigCaveatToStr(cav), cav, depthSize, stencilSize,
			nID, nRend, eglSurfaceTypeToStr(sType), sType, sampleBuff,
			minSwap, maxSwap);
}

static void printEGLConfs(EGLDisplay display)
{
	EGLConfig conf[96];
	EGLint num = 0;
	eglGetConfigs(display, conf, sizeofArray(conf), &num);
	logMsg("got %d configs", num);
	iterateTimes(num, i)
	{
		printEGLConf(display, conf[i]);
	}
}

static void printEGLConfsWithAttr(EGLDisplay display, const EGLint *attr)
{
	EGLConfig conf[96];
	EGLint num = 0;
	eglChooseConfig(display, attr, conf, sizeofArray(conf), &num);
	logMsg("got %d configs", num);
	iterateTimes(num, i)
	{
		printEGLConf(display, conf[i]);
	}
}

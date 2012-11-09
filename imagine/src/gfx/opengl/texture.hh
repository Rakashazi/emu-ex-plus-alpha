#pragma once

#include <gfx/GfxBufferImage.hh>

namespace Gfx
{

static GfxTextureHandle newTexRef()
{
	GLuint ref;
	glGenTextures(1, &ref);
	//logMsg("got texture id %u", ref);
	return ref;
}

static void freeTexRef(GfxTextureHandle texRef)
{
	//logMsg("deleting texture id %u", texRef);
	glcDeleteTextures(1, &texRef);
}

}

static uint setUnpackAlignForPitch(uint pitch)
{
	uint alignment = 1;
	if(!Config::envIsPS3 && pitch % 8 == 0) alignment = 8;
	else if(pitch % 4 == 0) alignment = 4;
	else if(pitch % 2 == 0) alignment = 2;
	//logMsg("setting unpack alignment %d", alignment);
	glcPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	//glcPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	return alignment;
}

#ifdef CONFIG_GFX_OPENGL_ES
#define GL_MIRRORED_REPEAT 0x8370
#endif

static int bestClampMode(bool textured)
{
	if(textured)
	{
		logMsg("repeating image");
		return GL_REPEAT;
	}
	#ifndef CONFIG_GFX_OPENGL_ES
		return useTextureClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP;
	#else
		return GL_CLAMP_TO_EDGE;
		//return GL_MIRRORED_REPEAT;
	#endif
}

static GLenum pixelFormatToOGLDataType(const PixelFormatDesc &format)
{
	switch(format.id)
	{
		case PIXEL_RGBA8888:
		case PIXEL_BGRA8888:
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
			return GL_UNSIGNED_INT_8_8_8_8_REV;
		#endif
		case PIXEL_ARGB8888:
		case PIXEL_ABGR8888:
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
			return GL_UNSIGNED_INT_8_8_8_8;
		#endif
		case PIXEL_RGB888:
		case PIXEL_BGR888:
		case PIXEL_I8:
		case PIXEL_IA88:
			return GL_UNSIGNED_BYTE;
		case PIXEL_RGB565:
			return GL_UNSIGNED_SHORT_5_6_5;
		case PIXEL_ARGB1555:
			return GL_UNSIGNED_SHORT_5_5_5_1;
		case PIXEL_ARGB4444:
			return GL_UNSIGNED_SHORT_4_4_4_4;
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
		case PIXEL_BGRA4444:
			return GL_UNSIGNED_SHORT_4_4_4_4_REV;
		case PIXEL_ABGR1555:
			return GL_UNSIGNED_SHORT_1_5_5_5_REV;
		#endif
		default: bug_branch("%d", format.id); return 0;
	}
}

static GLenum pixelFormatToOGLFormat(const PixelFormatDesc &format)
{
	#if defined(CONFIG_BASE_PS3)
		if(format.id == PIXEL_ARGB8888)
			return GL_BGRA;
	#endif
	if(format.isGrayscale())
	{
		if(format.aBits)
			return GL_LUMINANCE_ALPHA;
		else return GL_LUMINANCE;
	}
	#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
	else if(format.isBGROrder())
	{
		assert(supportBGRPixels);
		if(format.aBits)
		{
			return GL_BGRA;
		}
		else return GL_BGR;
	}
	#else
	else if(format.isBGROrder() && format.aBits)
	{
		assert(supportBGRPixels);
		return GL_BGRA;
	}
	#endif
	else if(format.aBits)
	{
		return GL_RGBA;
	}
	else return GL_RGB;
}

static int pixelToOGLInternalFormat(const PixelFormatDesc &format)
{
	#if defined(CONFIG_GFX_OPENGL_ES) && !defined(CONFIG_BASE_PS3)
		#ifdef CONFIG_BASE_IOS
			if(format.id == PIXEL_BGRA8888) // Apple's BGRA extension loosens the internalformat match requirement
				return GL_RGBA;
		#endif
		return pixelFormatToOGLFormat(format); // OpenGL ES manual states internalformat always equals format
	#else

	#if !defined(CONFIG_BASE_PS3)
	if(useCompressedTextures)
	{
		switch(format.id)
		{
			case PIXEL_RGBA8888:
			case PIXEL_BGRA8888:
				return GL_COMPRESSED_RGBA;
			case PIXEL_RGB888:
			case PIXEL_BGR888:
			case PIXEL_RGB565:
			case PIXEL_ARGB1555:
			case PIXEL_ARGB4444:
			case PIXEL_BGRA4444:
				return GL_COMPRESSED_RGB;
			case PIXEL_I8:
				return GL_COMPRESSED_LUMINANCE;
			case PIXEL_IA88:
				return GL_COMPRESSED_LUMINANCE_ALPHA;
			default: bug_branch("%d", format.id); return 0;
		}
	}
	else
	#endif
	{
		switch(format.id)
		{
			case PIXEL_BGRA8888:
			#if defined(CONFIG_BASE_PS3)
				return GL_BGRA;
			#endif
			case PIXEL_ARGB8888:
			case PIXEL_ABGR8888:
			#if defined(CONFIG_BASE_PS3)
				return GL_ARGB_SCE;
			#endif
			case PIXEL_RGBA8888:
				return GL_RGBA8;
			case PIXEL_RGB888:
			case PIXEL_BGR888:
				return GL_RGB8;
			case PIXEL_RGB565:
				return GL_RGB5;
			case PIXEL_ABGR1555:
			case PIXEL_ARGB1555:
				return GL_RGB5_A1;
			case PIXEL_ARGB4444:
			case PIXEL_BGRA4444:
				return GL_RGBA4;
			case PIXEL_I8:
				return GL_LUMINANCE8;
			case PIXEL_IA88:
				return GL_LUMINANCE8_ALPHA8;
			default: bug_branch("%d", format.id); return 0;
		}
	}

	#endif
}

static const char *glImageFormatToString(int format)
{
	switch(format)
	{
		#if defined(CONFIG_BASE_PS3)
		case GL_ARGB_SCE: return "ARGB_SCE";
		#endif
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
		case GL_RGBA8: return "RGBA8";
		case GL_RGB8: return "RGB8";
		case GL_RGB5_A1: return "RGB5_A1";
		case GL_RGB5: return "RGB5";
		case GL_RGBA4: return "RGBA4";
		case GL_BGR: return "BGR";
		case GL_LUMINANCE8: return "LUMINANCE8";
		case GL_LUMINANCE8_ALPHA8: return "LUMINANCE8_ALPHA8";
		#endif
		#if !defined(CONFIG_GFX_OPENGL_ES)
		case GL_COMPRESSED_RGBA: return "COMPRESSED_RGBA";
		case GL_COMPRESSED_RGB: return "COMPRESSED_RGB";
		case GL_COMPRESSED_LUMINANCE: return "COMPRESSED_LUMINANCE";
		case GL_COMPRESSED_LUMINANCE_ALPHA: return "COMPRESSED_LUMINANCE_ALPHA";
		#endif
		case GL_RGBA: return "RGBA";
		//#if defined(CONFIG_BASE_PS3) || defined(CONFIG_ENV_WEBOS) || !defined(CONFIG_GFX_OPENGL_ES)
		case GL_BGRA: return "BGRA";
		/*#else
		case GL_BGRA_EXT: return "BGRA";
		#endif*/
		case GL_RGB: return "RGB";
		case GL_LUMINANCE: return "LUMINANCE";
		case GL_LUMINANCE_ALPHA: return "LUMINANCE_ALPHA";
		default: bug_branch("%d", format); return NULL;
	}
}

static const char *glDataTypeToString(int format)
{
	switch(format)
	{
		case GL_UNSIGNED_BYTE: return "B";
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
		case GL_UNSIGNED_INT_8_8_8_8: return "I8888";
		case GL_UNSIGNED_INT_8_8_8_8_REV: return "I8888R";
		case GL_UNSIGNED_SHORT_1_5_5_5_REV: return "S1555";
		case GL_UNSIGNED_SHORT_4_4_4_4_REV: return "S4444R";
		#endif
		case GL_UNSIGNED_SHORT_5_6_5: return "S565";
		case GL_UNSIGNED_SHORT_5_5_5_1: return "S5551";
		case GL_UNSIGNED_SHORT_4_4_4_4: return "S4444";
		default: bug_branch("%d", format); return NULL;
	}
}

enum { MIPMAP_NONE, MIPMAP_LINEAR, MIPMAP_NEAREST };
static GLint openGLFilterType(uint imgFilter, uchar mipmapType)
{
	if(imgFilter == GfxBufferImage::nearest)
	{
		return mipmapType == MIPMAP_NEAREST ? GL_NEAREST_MIPMAP_NEAREST :
			mipmapType == MIPMAP_LINEAR ? GL_NEAREST_MIPMAP_LINEAR :
			GL_NEAREST;
	}
	else
	{
		return mipmapType == MIPMAP_NEAREST ? GL_LINEAR_MIPMAP_NEAREST :
			mipmapType == MIPMAP_LINEAR ? GL_LINEAR_MIPMAP_LINEAR :
			GL_LINEAR;
	}
}

static void setDefaultImageTextureParams(uint imgFilter, uchar mipmapType, int xWrapType, int yWrapType, uint usedX, uint usedY, GLenum target)
{
	//mipmapType = MIPMAP_NONE;
	GLint filter = openGLFilterType(imgFilter, mipmapType);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, xWrapType);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, yWrapType);
	if(filter != GL_LINEAR) // GL_LINEAR is the default
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
	#ifndef CONFIG_ENV_WEBOS
	if(useAnisotropicFiltering)
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	#endif
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//float col[] = {1, 0.5, 0.5, 1};
	//glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, col);

	#if defined(CONFIG_GFX_OPENGL_ES) && !defined(CONFIG_BASE_PS3)
	if(useDrawTex)
	{
		GLint coords [] = {0, 0, (int)usedX, (int)usedY};
		glTexParameteriv(target, GL_TEXTURE_CROP_RECT_OES, coords);
	}
	#endif
}

static uint writeGLTexture(Pixmap &pix, bool includePadding, GLenum target)
{
	//logMsg("writeGLTexture");
	uint alignment = setUnpackAlignForPitch(pix.pitch);
	assert((ptrsize)pix.data % (ptrsize)alignment == 0);
	GLenum format = pixelFormatToOGLFormat(pix.format);
	GLenum dataType = pixelFormatToOGLDataType(pix.format);
	uint xSize = includePadding ? pix.pitchPixels() : pix.x;
	#ifndef CONFIG_GFX_OPENGL_ES
		glcPixelStorei(GL_UNPACK_ROW_LENGTH, (!includePadding && pix.isPadded()) ? pix.pitchPixels() : 0);
		//logMsg("writing %s %dx%d to %dx%d, xline %d", glImageFormatToString(format), 0, 0, pix->x, pix->y, pix->pitch / pix->format->bytesPerPixel);
		clearGLError();
		glTexSubImage2D(target, 0, 0, 0,
				xSize, pix.y, format, dataType, pix.data);
		glErrorCase(err)
		{
			logErr("%s in glTexSubImage2D", glErrorToString(err));
			return 0;
		}
	#else
		clearGLError();
		if(includePadding || pix.pitch == pix.x * pix.format.bytesPerPixel)
		{
			//logMsg("pitch equals x size optimized case");
			glTexSubImage2D(target, 0, 0, 0,
					xSize, pix.y, format, dataType, pix.data);
			glErrorCase(err)
			{
				logErr("%s in glTexSubImage2D", glErrorToString(err));
				return 0;
			}
		}
		else
		{
			logWarn("OGL ES slow glTexSubImage2D case");
			uchar *row = pix.data;
			for(int y = 0; y < (int)pix.y; y++)
			{
				glTexSubImage2D(target, 0, 0, y,
						pix.x, 1, format, dataType, row);
				glErrorCase(err)
				{
					logErr("%s in glTexSubImage2D, line %d", glErrorToString(err), y);
					return 0;
				}
				row += pix.pitch;
			}
		}
	#endif

	return 1;
}

static uint replaceGLTexture(Pixmap &pix, bool upload, uint internalFormat, bool includePadding, GLenum target)
{
	/*#ifdef CONFIG_GFX_OPENGL_ES // pal tex test
	if(pix->format->id == PIXEL_IA88)
	{
		logMsg("testing pal tex");
		return 1;
	}
	#endif*/

	uint alignment = setUnpackAlignForPitch(pix.pitch);
	assert((ptrsize)pix.data % (ptrsize)alignment == 0);
	#ifndef CONFIG_GFX_OPENGL_ES
		glcPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	#endif
	GLenum format = pixelFormatToOGLFormat(pix.format);
	GLenum dataType = pixelFormatToOGLDataType(pix.format);
	uint xSize = includePadding ? pix.pitchPixels() : pix.x;
	if(includePadding && pix.pitchPixels() != pix.x)
		logMsg("including padding in texture size, %d", pix.pitchPixels());
	clearGLError();
	glTexImage2D(target, 0, internalFormat, xSize, pix.y,
				0, format, dataType, upload ? pix.data : 0);
	glErrorCase(err)
	{
		logErr("%s in glTexImage2D", glErrorToString(err));
		return 0;
	}
	return 1;
}

static const PixelFormatDesc *swapRGBToPreferedOrder(const PixelFormatDesc *fmt)
{
	if(Gfx::preferBGR && fmt->id == PIXEL_RGB888)
		return &PixelFormatBGR888;
	else if(Gfx::preferBGRA && fmt->id == PIXEL_RGBA8888)
		return &PixelFormatBGRA8888;
	else
		return fmt;
}

bool GfxBufferImage::hasMipmaps()
{
	return hasMipmaps_;
}

void GfxBufferImage::setFilter(uint filter)
{
	auto filterGL = openGLFilterType(filter, hasMipmaps() ? MIPMAP_LINEAR : MIPMAP_NONE);
	logMsg("setting texture filter %s", filter == GfxBufferImage::nearest ? "nearest" : "linear");
	#if !defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	GLenum target = textureDesc().target;
	#endif
	Gfx::setActiveTexture(textureDesc().tid, target);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filterGL);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filterGL);
}

void GfxBufferImage::setRepeatMode(uint xMode, uint yMode)
{
	#if !defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	GLenum target = textureDesc().target;
	#endif
	Gfx::setActiveTexture(textureDesc().tid, target);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, xMode ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, yMode ? GL_REPEAT : GL_CLAMP_TO_EDGE);
}

void TextureGfxBufferImage::write(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);
	writeGLTexture(p, hints, GL_TEXTURE_2D);

	#ifdef CONFIG_BASE_ANDROID
		if(unlikely(glSyncHackEnabled)) glFinish();
	#endif
}

void TextureGfxBufferImage::replace(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);
	replaceGLTexture(p, 1, pixelToOGLInternalFormat(p.format), hints, GL_TEXTURE_2D);
}

Pixmap *TextureGfxBufferImage::lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback) { return fallback; }

void TextureGfxBufferImage::unlock(Pixmap *pix, uint hints) { write(*pix, hints); }

void TextureGfxBufferImage::deinit()
{
	Gfx::freeTexRef(tid);
	tid = 0;
}

#ifdef CONFIG_BASE_ANDROID
static int pixelFormatToDirectAndroidFormat(const PixelFormatDesc &format)
{
	switch(format.id)
	{
		case PIXEL_RGBA8888: return HAL_PIXEL_FORMAT_RGBA_8888;
		case PIXEL_BGRA8888: return HAL_PIXEL_FORMAT_BGRA_8888;
		//case PIXEL_RGB888: return HAL_PIXEL_FORMAT_RGB_888;
		case PIXEL_RGB565: return HAL_PIXEL_FORMAT_RGB_565;
		case PIXEL_ARGB1555: return HAL_PIXEL_FORMAT_RGBA_5551;
		case PIXEL_ARGB4444: return HAL_PIXEL_FORMAT_RGBA_4444;
		//case PIXEL_I8: return GGL_PIXEL_FORMAT_L_8;
		//case PIXEL_IA88: return GGL_PIXEL_FORMAT_LA_88;
		default: return GGL_PIXEL_FORMAT_NONE;
	}
}
#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE

/*#include <unistd.h>

static void printPrivHnd(buffer_handle_t handle)
{
	private_handle_t* pHnd = (private_handle_t*)handle;
	logMsg("app pid %d version %d numFds %d numInts %d", (int)getpid(), pHnd->version, pHnd->numFds, pHnd->numInts);
	logMsg("fd %d magic %d flags %x size %d offset %d gpu_fd %d base %d lockState %x writeOwner %d gpuaddr %d pid %d",
			pHnd->fd, pHnd->magic, pHnd->flags, pHnd->size, pHnd->offset, pHnd->gpu_fd, pHnd->base, pHnd->lockState, pHnd->writeOwner, pHnd->gpuaddr, pHnd->pid);

}*/

bool DirectTextureGfxBufferImage::testSupport(const char **errorStr)
{
	GLuint ref;
	glGenTextures(1, &ref);
	glcBindTexture(GL_TEXTURE_2D, ref);

	Pixmap pix(PixelFormatRGB565);
	pix.init(nullptr, 256, 256);
	DirectTextureGfxBufferImage directTex;
	if(directTex.init(pix, ref, 256, 256, errorStr))
	{
		logMsg("tests passed");
		directTex.deinit();
		return 1;
	}
	else
	{
		glcDeleteTextures(1, &ref);
		return 0;
	}
}

bool DirectTextureGfxBufferImage::initTexture(Pixmap &pix, uint usedX, uint usedY, bool testLock, const char **errorStr)
{
	int androidFormat = pixelFormatToDirectAndroidFormat(pix.format);
	if(androidFormat == GGL_PIXEL_FORMAT_NONE)
	{
		logMsg("format cannot be used");
		if(errorStr) *errorStr = "Unsupported pixel format";
		goto CLEANUP;
	}

	setupAndroidNativeBuffer(eglBuf, usedX, usedY, androidFormat,
		/*GRALLOC_USAGE_SW_READ_OFTEN |*/ GRALLOC_USAGE_SW_WRITE_OFTEN
		/*| GRALLOC_USAGE_HW_RENDER*/ | GRALLOC_USAGE_HW_TEXTURE);
	int err;
	if((err = directTextureConf.allocBuffer(eglBuf)) != 0)
	{
		logMsg("error in alloc buffer: %s", strerror(-err));
		if(errorStr) *errorStr = "Allocation failed";
		goto CLEANUP;
	}
	logMsg("got buffer %p with stride %d", eglBuf.handle, eglBuf.stride);
	//printPrivHnd(eglBuf.handle);

	/*if(grallocMod->registerBuffer(grallocMod, eglBuf.handle) != 0)
	{
		logMsg("error registering");
	}*/
	if(testLock)
	{
		logMsg("testing locking");
		void *data;
		if(directTextureConf.lockBuffer(eglBuf, GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, usedX, usedY, data) != 0)
		{
			logMsg("error locking");
			if(errorStr) *errorStr = "Lock failed";
			goto CLEANUP;
		}
		logMsg("data addr %p", data);
		//printPrivHnd(eglBuf.handle);
		//memset(data, 0, eglBuf.stride * eglBuf.height * 2);
		logMsg("unlocking");
		if(directTextureConf.unlockBuffer(eglBuf) != 0)
		{
			logMsg("error unlocking");
			if(errorStr) *errorStr = "Unlock failed";
			goto CLEANUP;
		}
	}
	//printPrivHnd(eglBuf.handle);

	eglImg = eglCreateImageKHR(Base::getAndroidEGLDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)&eglBuf, directTextureConf.eglImgAttrs);
	if(eglImg == EGL_NO_IMAGE_KHR)
	{
		logMsg("error creating EGL image");
		if(errorStr) *errorStr = "eglCreateImageKHR failed";
		goto CLEANUP;
	}

	clearGLError();
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)eglImg);
	glErrorCase(err)
	{
		logErr("%s in glEGLImageTargetTexture2DOES", glErrorToString(err));
		if(errorStr) *errorStr = "glEGLImageTargetTexture2DOES failed";
		goto CLEANUP;
	}

	new(&eglPixmap) Pixmap(pix.format);
	eglPixmap.init(0, usedX, usedY, 0);
	eglPixmap.pitch = eglBuf.stride * eglPixmap.format.bytesPerPixel;

	return 1;

	CLEANUP:

	if(eglImg != EGL_NO_IMAGE_KHR)
	{
		logMsg("calling eglDestroyImageKHR");
		eglDestroyImageKHR(Base::getAndroidEGLDisplay(), eglImg);
	}
	if(eglBuf.handle)
	{
		directTextureConf.freeBuffer(eglBuf);
	}
	return 0;
}

bool DirectTextureGfxBufferImage::init(Pixmap &pix, uint texRef, uint usedX, uint usedY, const char **errorStr)
{
	return initTexture(pix, usedX, usedY, 1, errorStr);
}

/*void DirectTextureGfxBufferImage::replace(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);
	if(eglBuf.handle)
	{
		eglDestroyImageKHR(Base::getAndroidEGLDisplay(), eglImg);
		if(directTextureConf.freeBuffer(eglBuf) != 0)
		{
			logWarn("error freeing buffer");
		}
	}
	if(!initTexture(p, usedX, usedY, 0))
	{
		logErr("error redefining EGL texture");
	}
}*/

void DirectTextureGfxBufferImage::write(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);

	//logMsg("updating EGL image");
	void *data;
	Pixmap *texturePix = lock(0, 0, p.x, p.y);
	if(!texturePix)
	{
		return;
	}
	p.copy(0, 0, 0, 0, texturePix, 0, 0);
	unlock();
}

Pixmap *DirectTextureGfxBufferImage::lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback)
{
	void *data;
	if(directTextureConf.lockBuffer(eglBuf, GRALLOC_USAGE_SW_WRITE_OFTEN, x, y, xlen, ylen, data) != 0)
	{
		logMsg("error locking");
		return 0;
	}
	eglPixmap.data = (uchar*)data;
	return &eglPixmap;
}

void DirectTextureGfxBufferImage::unlock(Pixmap *pix, uint hints)
{
	directTextureConf.unlockBuffer(eglBuf);
}

void DirectTextureGfxBufferImage::deinit()
{
	logMsg("freeing EGL image");
	if(eglBuf.handle)
	{
		eglDestroyImageKHR(Base::getAndroidEGLDisplay(), eglImg);
		if(directTextureConf.freeBuffer(eglBuf) != 0)
		{
			logWarn("error freeing buffer");
		}
	}
	TextureGfxBufferImage::deinit();
	Gfx::freeTexRef(tid);
	tid = 0;
}

#endif

#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)

#include <android/native_window_jni.h>

struct SurfaceTextureGfxBufferImage: public GfxBufferImageInterface
{
	jobject surfaceTex, surface;
	ANativeWindow* nativeWin;
	Pixmap pix {PixelFormatRGB565}, *backingPix;
	void init(int tid, Pixmap &pixmap)
	{
		using namespace Base;
		using namespace Gfx;
		backingPix = &pixmap; // not yet used

		var_selfs(tid);
		new(&pix) Pixmap(pixmap.format);
		pix.init(0, pixmap.x, pixmap.y, 0);
		logMsg("creating SurfaceTexture with id %d", tid);
		surfaceTex = eEnv()->NewObject(surfaceTextureConf.jSurfaceTextureCls, surfaceTextureConf.jSurfaceTexture.m, tid);
		assert(surfaceTex);
		//jEnv->CallVoidMethod(surfaceTex, jSetDefaultBufferSize.m, x, y);

		surface = eEnv()->NewObject(surfaceTextureConf.jSurfaceCls, surfaceTextureConf.jSurface.m, surfaceTex);
		assert(surface);

		// ANativeWindow_fromSurfaceTexture was removed from Android 4.1
		//nativeWin = surfaceTextureConf.ANativeWindow_fromSurfaceTexture(eEnv(), surfaceTex);
		nativeWin = ANativeWindow_fromSurface(eEnv(), surface);
		assert(nativeWin);
		logMsg("got native window %p from Surface %p", nativeWin, surface);
		replace(pixmap, 0);
	}

	/*void write()
	{
		write(*backingPix, 1);
	}*/

	void write(Pixmap &p, uint hints)
	{
		Pixmap *texturePix = lock(0, 0, p.x, p.y);
		if(!texturePix)
		{
			logWarn("unable to lock texture");
			return;
		}
		p.copy(0, 0, 0, 0, texturePix, 0, 0);
		unlock();
	}

	void replace(Pixmap &pixmap, uint hints)
	{
		int winFormat = pixelFormatToDirectAndroidFormat(pixmap.format);
		assert(winFormat);
		if(ANativeWindow_setBuffersGeometry(nativeWin, pixmap.x, pixmap.y, winFormat) < 0)
		{
			logErr("error in ANativeWindow_setBuffersGeometry");
		}
	}

	Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback = nullptr)
	{
		ANativeWindow_Buffer buffer;
		//ARect rect = { x, y, xlen, ylen };
		ANativeWindow_lock(nativeWin, &buffer, 0/*&rect*/);
		pix.pitch = buffer.stride * pix.format.bytesPerPixel;
		pix.data = (uchar*)buffer.bits;
		//logMsg("locked buffer %p with pitch %d", buffer.bits, buffer.stride);
		return &pix;
	}

	void unlock(Pixmap *pix = nullptr, uint hints = 0)
	{
		using namespace Base;
		using namespace Gfx;
		ANativeWindow_unlockAndPost(nativeWin);
		surfaceTextureConf.jUpdateTexImage(eEnv(), surfaceTex);
		// texture implicitly bound in updateTexImage()
		glState.bindTextureState.GL_TEXTURE_EXTERNAL_OES_state = tid;
	}

	void deinit()
	{
		using namespace Base;
		using namespace Gfx;
		logMsg("deinit SurfaceTexture, releasing window");
		ANativeWindow_release(nativeWin);
		surfaceTextureConf.jSurfaceRelease(eEnv(), surface);
		eEnv()->DeleteLocalRef(surface);
		surfaceTextureConf.jSurfaceTextureRelease(eEnv(), surfaceTex);
		eEnv()->DeleteLocalRef(surfaceTex);
		Gfx::freeTexRef(tid);
		tid = 0;

		if(surfaceTextureConf.texture2dBindingHack)
		{
			GLint realTexture = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &realTexture);
			if(glState.bindTextureState.GL_TEXTURE_2D_state != (GLuint)realTexture)
			{
				logMsg("setting GL_TEXTURE_2D binding state to %d, should be %d", realTexture, glState.bindTextureState.GL_TEXTURE_2D_state);
			}
			glState.bindTextureState.GL_TEXTURE_2D_state = realTexture;
		}
	}
};

#endif

bool GfxBufferImage::setupTexture(Pixmap &pix, bool upload, uint internalFormat, int xWrapType, int yWrapType,
	uint usedX, uint usedY, uint hints, uint filter)
{
	//logMsg("createGLTexture");
	GLenum texTarget = GL_TEXTURE_2D;
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	if(Gfx::surfaceTextureConf.use && (hints & GfxBufferImage::HINT_STREAM))
	{
		texTarget = GL_TEXTURE_EXTERNAL_OES;
	}
	#endif

	auto texRef = Gfx::newTexRef();
	if(texRef == 0)
	{
		logMsg("error getting new texture reference");
		return 0;
	}

	//logMsg("binding texture %d", texRef);
	glcBindTexture(texTarget, texRef);
	setDefaultImageTextureParams(filter, hasMipmaps() ? MIPMAP_LINEAR : MIPMAP_NONE, xWrapType, yWrapType, usedX, usedY, texTarget);

	bool includePadding = 0; //include extra bytes when x != pitch ?
	if(hints & GfxBufferImage::HINT_STREAM)
	{
		#if defined(CONFIG_BASE_PS3)
		logMsg("optimizing texture for frequent updates");
		glTexParameteri(texTarget, GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_LINEAR_SYSTEM_SCE);
		#endif
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		if(Gfx::surfaceTextureConf.use)
		{
			logMsg("using SurfaceTexture, %dx%d %s", usedX, usedY, pix.format.name);
			pix.x = usedX;
			pix.y = usedY;
			auto *surfaceTex = new SurfaceTextureGfxBufferImage;
			surfaceTex->init(texRef, pix);
			impl = surfaceTex;
			textureDesc().target = GL_TEXTURE_EXTERNAL_OES;
			textureDesc().tid = texRef;
			return 1;
		}
		#endif
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		if(directTextureConf.useEGLImageKHR)
		{
			logMsg("using EGL image for texture, %dx%d %s", usedX, usedY, pix.format.name);
			auto *directTex = new DirectTextureGfxBufferImage;
			if(directTex->init(pix, texRef, usedX, usedY))
			{
				pix.x = usedX;
				pix.y = usedY;
				impl = directTex;
				textureDesc().tid = texRef;
				return 1;
			}
			else
			{
				logWarn("failed to create EGL image, falling back to normal texture");
				delete directTex;
			}
		}
		#endif
		#ifdef CONFIG_GFX_OPENGL_ES
		includePadding = 1; // avoid slow OpenGL ES upload case
		#endif
	}

	if(hasMipmaps())
	{
		logMsg("auto-generating mipmaps");
		#ifndef CONFIG_GFX_OPENGL_ES
			glTexParameteri(texTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
		#elif !defined(CONFIG_BASE_PS3)
			glTexParameteri(texTarget, GL_GENERATE_MIPMAP, GL_TRUE);
		#else
			// TODO: find PS3 version
		#endif
	}
	{
		GLenum format = pixelFormatToOGLFormat(pix.format);
		GLenum dataType = pixelFormatToOGLDataType(pix.format);
		logMsg("%s texture %dx%d with internal format %s from image %s:%s", upload ? "uploading" : "creating", pix.x, pix.y, glImageFormatToString(internalFormat), glImageFormatToString(format), glDataTypeToString(dataType));
	}
	if(replaceGLTexture(pix, upload, internalFormat, includePadding, texTarget))
	{
		//logMsg("success");
	}

	#ifndef CONFIG_GFX_OPENGL_ES
	if(upload && useFBOFuncs)
	{
		logMsg("generating mipmaps");
		glGenerateMipmapEXT(texTarget);
	}
	#endif

	#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
		impl = new TextureGfxBufferImage;
	#endif
	textureDesc().tid = texRef;
	return 1;
}

#if defined(CONFIG_RESOURCE_IMAGE)
/*CallResult GfxBufferImage::subInit(ResourceImage &img, int x, int y, int xSize, int ySize)
{
	using namespace Gfx;
	uint texX, texY;
	textureSizeSupport.findBufferXYPixels(texX, texY, img.width(), img.height());
	tid = img.gfxD.tid;
	xStart = pixelToTexC((uint)x, texX);
	yStart = pixelToTexC((uint)y, texY);
	xEnd = pixelToTexC((uint)x+xSize, texX);
	yEnd = pixelToTexC((uint)y+ySize, texY);
	return OK;
}*/

CallResult GfxBufferImage::init(ResourceImage &img, uint filter, uint hints, bool textured)
{
	using namespace Gfx;
	var_selfs(hints);
	testMipmapSupport(img.width(), img.height());
	//logMsg("GfxBufferImage::init");
	int wrapMode = bestClampMode(textured);

	uint texX, texY;
	textureSizeSupport.findBufferXYPixels(texX, texY, img.width(), img.height());

	auto pixFmt = swapRGBToPreferedOrder(img.pixelFormat());
	Pixmap texPix(*pixFmt);
	uint uploadPixStoreSize = texX * texY * pixFmt->bytesPerPixel;
	#if defined(CONFIG_BASE_PS3)
	//logMsg("alloc in heap"); // PS3 has 1MB stack limit
	uchar *uploadPixStore = (uchar*)mem_alloc(uploadPixStoreSize);
	if(!uploadPixStore)
		return OUT_OF_MEMORY;
	#else
	uchar uploadPixStore[uploadPixStoreSize] __attribute__ ((aligned (8)));
	#endif
	mem_zero(uploadPixStore, uploadPixStoreSize);
	texPix.init(uploadPixStore, texX, texY, 0);
	img.getImage(&texPix);
	if(!setupTexture(texPix, 1, pixelToOGLInternalFormat(texPix.format), wrapMode,
			wrapMode, img.width(), img.height(), hints, filter))
	{
		#if defined(CONFIG_BASE_PS3)
		mem_free(uploadPixStore);
		#endif
		return INVALID_PARAMETER;
	}

	textureDesc().xStart = pixelToTexC((uint)0, texPix.x);
	textureDesc().yStart = pixelToTexC((uint)0, texPix.y);
	textureDesc().xEnd = pixelToTexC(img.width(), texPix.x);
	textureDesc().yEnd = pixelToTexC(img.height(), texPix.y);

	#if defined(CONFIG_BASE_PS3)
	mem_free(uploadPixStore);
	#endif
	backingImg = &img;
	//logMsg("set backing resource %p", backingImg);
	return OK;
}
#endif

void GfxBufferImage::testMipmapSupport(uint x, uint y)
{
	hasMipmaps_ = usingMipmaping() &&
			!(hints & HINT_STREAM) && !(hints & HINT_NO_MINIFY)
			&& Gfx::textureSizeSupport.supportsMipmaps(x, y);
}

CallResult GfxBufferImage::init(Pixmap &pix, bool upload, uint filter, uint hints, bool textured)
{
	using namespace Gfx;
	if(isInit())
		deinit();

	var_selfs(hints);
	testMipmapSupport(pix.x, pix.y);

	int wrapMode = bestClampMode(textured);

	uint xSize = (hints & HINT_STREAM) ? pix.pitchPixels() : pix.x;
	uint texX, texY;
	textureSizeSupport.findBufferXYPixels(texX, texY, xSize, pix.y,
		(hints & HINT_STREAM) ? TextureSizeSupport::streamHint : 0);

	Pixmap texPix(pix.format);
	texPix.init(0, texX, texY, 0);

	/*uchar uploadPixStore[texX * texY * pix.format->bytesPerPixel];

	if(upload && pix.pitch != uploadPix.pitch)
	{
		mem_zero(uploadPixStore);
		pix.copy(0, 0, 0, 0, &uploadPix, 0, 0);
	}*/
	assert(upload == 0);
	if(!setupTexture(texPix, upload, pixelToOGLInternalFormat(pix.format),
			wrapMode, wrapMode, pix.x, pix.y, hints, filter))
	{
		return INVALID_PARAMETER;
	}

	textureDesc().xStart = pixelToTexC((uint)0, texPix.x);
	textureDesc().yStart = pixelToTexC((uint)0, texPix.y);
	textureDesc().xEnd = pixelToTexC(pix.x, texPix.x);
	textureDesc().yEnd = pixelToTexC(pix.y, texPix.y);

	return OK;
}

void GfxBufferImage::write(Pixmap &p) { GfxBufferImageImpl::write(p, hints); }
void GfxBufferImage::replace(Pixmap &p)
{
	GfxBufferImageImpl::replace(p, hints);
}
void GfxBufferImage::unlock(Pixmap *p) { GfxBufferImageImpl::unlock(p, hints); }

void GfxBufferImage::deinit()
{
	if(!isInit())
		return;

	if(backingImg)
	{
		logMsg("deinit via backing texture resource");
		backingImg->deinit(); // backingImg set to 0 before real deinit
	}
	else
		GfxBufferImageImpl::deinit();
}

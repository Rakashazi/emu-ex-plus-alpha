#pragma once

#include <algorithm>
#include <gfx/GfxBufferImage.hh>
#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	#include "android/SurfaceTextureBufferImage.hh"
#endif

namespace Gfx
{

GfxTextureHandle newTexRef()
{
	GLuint ref;
	glGenTextures(1, &ref);
	//logMsg("created texture %u", ref);
	return ref;
}

void freeTexRef(GfxTextureHandle texRef)
{
	//logMsg("deleting texture %u", texRef);
	glcDeleteTextures(1, &texRef);
}

static uint unpackAlignForPitch(uint pitch)
{
	uint alignment = 1;
	if(!Config::envIsPS3 && (pitch % 8 == 0)) alignment = 8;
	else if(pitch % 4 == 0) alignment = 4;
	else if(pitch % 2 == 0) alignment = 2;
	return alignment;
}

static uint unpackAlignForAddr(void *srcAddr)
{
	auto addr = (ptrsize)srcAddr;
	uint alignment = 1;
	if(addr % 8 == 0) alignment = 8;
	else if(addr % 4 == 0) alignment = 4;
	else if(addr % 2 == 0) alignment = 2;
	return alignment;
}

static uint unpackAlignForAddrAndPitch(void *srcAddr, uint pitch)
{
	uint alignmentForAddr = unpackAlignForAddr(srcAddr);
	uint alignmentForPitch = unpackAlignForPitch(pitch);
	if(alignmentForAddr < alignmentForPitch)
	{
		logMsg("using lowest alignment of address %p (%d) and pitch %d (%d)",
				srcAddr, alignmentForAddr, pitch, alignmentForPitch);
	}
	uint alignment = std::min(alignmentForPitch, alignmentForAddr);
	return alignment;
}

uint BufferImage::bestAlignment(const Pixmap &p)
{
	return unpackAlignForAddrAndPitch(p.data, p.pitch);
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
	return GL_CLAMP_TO_EDGE;
	//return GL_MIRRORED_REPEAT;
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

enum { MIPMAP_NONE, MIPMAP_LINEAR, MIPMAP_NEAREST };
static GLint minFilterType(uint imgFilter, uchar mipmapType)
{
	if(imgFilter == BufferImage::nearest)
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

static GLint magFilterType(uint imgFilter)
{
	return imgFilter == BufferImage::nearest ? GL_NEAREST : GL_LINEAR;
}

static void setDefaultImageTextureParams(uint imgFilter, uchar mipmapType, int xWrapType, int yWrapType, uint usedX, uint usedY, GLenum target)
{
	//mipmapType = MIPMAP_NONE;
	glTexParameteri(target, GL_TEXTURE_WRAP_S, xWrapType);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, yWrapType);
	auto magFilter = magFilterType(imgFilter);
	if(magFilter != GL_LINEAR) // GL_LINEAR is the default
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterType(imgFilter, mipmapType));
	#ifndef CONFIG_ENV_WEBOS
	if(useAnisotropicFiltering)
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	#endif
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//float col[] = {1, 0.5, 0.5, 1};
	//glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, col);
}

static uint writeGLTexture(Pixmap &pix, bool includePadding, GLenum target, uint srcAlign)
{
	//logMsg("writeGLTexture");
	//logMsg("setting source pixel row alignment: %d", srcAlign);
	glcPixelStorei(GL_UNPACK_ALIGNMENT, srcAlign);
	if((ptrsize)pix.data % (ptrsize)srcAlign != 0)
	{
		bug_exit("expected data from address %p to be aligned to %u bytes", pix.data, srcAlign);
	}
	GLenum format = pixelFormatToOGLFormat(pix.format);
	GLenum dataType = pixelFormatToOGLDataType(pix.format);
	uint xSize = includePadding ? pix.pitchPixels() : pix.x;
	#ifndef CONFIG_GFX_OPENGL_ES
		glcPixelStorei(GL_UNPACK_ROW_LENGTH, (!includePadding && pix.isPadded()) ? pix.pitchPixels() : 0);
		//logMsg("writing %s %dx%d to %dx%d, xline %d", glImageFormatToString(format), 0, 0, pix->x, pix->y, pix->pitch / pix->format->bytesPerPixel);
		handleGLErrors();
		glTexSubImage2D(target, 0, 0, 0,
				xSize, pix.y, format, dataType, pix.data);
		if(handleGLErrors([](GLenum, const char *err) { logErr("%s in glTexSubImage2D", err); }))
		{
			return 0;
		}
	#else
		handleGLErrors();
		if(includePadding || pix.pitch == pix.x * pix.format.bytesPerPixel)
		{
			//logMsg("pitch equals x size optimized case");
			glTexSubImage2D(target, 0, 0, 0,
					xSize, pix.y, format, dataType, pix.data);
			if(handleGLErrors([](GLenum, const char *err) { logErr("%s in glTexSubImage2D", err); }))
			{
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
				if(handleGLErrors([](GLenum, const char *err) { logErr("%s in glTexSubImage2D", err); }))
				{
					logErr("error in line %d", y);
					return 0;
				}
				row += pix.pitch;
			}
		}
	#endif

	return 1;
}

static uint replaceGLTexture(Pixmap &pix, bool upload, uint internalFormat, bool includePadding, GLenum target, uint srcAlign)
{
	glcPixelStorei(GL_UNPACK_ALIGNMENT, srcAlign);
	if((ptrsize)pix.data % (ptrsize)srcAlign != 0)
	{
		bug_exit("expected data from address %p to be aligned to %u bytes", pix.data, srcAlign);
	}
	#ifndef CONFIG_GFX_OPENGL_ES
		glcPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	#endif
	GLenum format = pixelFormatToOGLFormat(pix.format);
	GLenum dataType = pixelFormatToOGLDataType(pix.format);
	uint xSize = includePadding ? pix.pitchPixels() : pix.x;
	if(includePadding && pix.pitchPixels() != pix.x)
		logMsg("including padding in texture size, %d", pix.pitchPixels());
	handleGLErrors();
	glTexImage2D(target, 0, internalFormat, xSize, pix.y,
				0, format, dataType, upload ? pix.data : 0);
	if(handleGLErrors([](GLenum, const char *err) { logErr("%s in glTexImage2D", err); }))
	{
		return 0;
	}
	return 1;
}

static uint replaceGLTexture(Pixmap &pix, bool upload, uint internalFormat, bool includePadding, GLenum target)
{
	uint alignment = unpackAlignForAddrAndPitch(pix.data, pix.pitch);
	return replaceGLTexture(pix, upload, internalFormat, includePadding, target, alignment);
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

bool BufferImage::hasMipmaps()
{
	return hasMipmaps_;
}

void BufferImage::setFilter(uint filter)
{
	logMsg("setting texture filter %s", filter == BufferImage::nearest ? "nearest" : "linear");
	#if !defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	GLenum target = textureDesc().target;
	#endif
	Gfx::setActiveTexture(textureDesc().tid, target);
	auto magFilter = magFilterType(filter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
	if(handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexParameteri with GL_TEXTURE_MAG_FILTER", err); }))
		logWarn("error with mag filter %d", magFilter);
	auto minFilter = minFilterType(filter, hasMipmaps() ? MIPMAP_NEAREST : MIPMAP_NONE);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
	if(handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexParameteri with GL_TEXTURE_MIN_FILTER", err); }))
		logWarn("error with min filter %d", minFilter);
}

void BufferImage::setRepeatMode(uint xMode, uint yMode)
{
	#if !defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	GLenum target = textureDesc().target;
	#endif
	Gfx::setActiveTexture(textureDesc().tid, target);
	auto wrapS = xMode ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapS);
	if(handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexParameteri with GL_TEXTURE_WRAP_S", err); }))
		logWarn("error with wrap s %d", wrapS);
	auto wrapT = yMode ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapT);
	if(handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexParameteri with GL_TEXTURE_WRAP_T", err); }))
		logWarn("error with wrap t %d", wrapT);
}

void TextureBufferImage::write(Pixmap &p, uint hints, uint alignment)
{
	glcBindTexture(GL_TEXTURE_2D, tid);
	writeGLTexture(p, hints, GL_TEXTURE_2D, alignment);

	#ifdef CONFIG_BASE_ANDROID
		if(unlikely(glSyncHackEnabled)) glFinish();
	#endif
}

void TextureBufferImage::write(Pixmap &p, uint hints)
{
	uint alignment = unpackAlignForAddrAndPitch(p.data, p.pitch);
	write(p, hints, alignment);
}

void TextureBufferImage::replace(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);
	replaceGLTexture(p, 1, pixelToOGLInternalFormat(p.format), hints, GL_TEXTURE_2D);
}

Pixmap *TextureBufferImage::lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback) { return fallback; }

void TextureBufferImage::unlock(Pixmap *pix, uint hints) { write(*pix, hints); }

void TextureBufferImage::deinit()
{
	logMsg("freeing texture 0x%X", tid);
	freeTexRef(tid);
	tid = 0;
}

bool BufferImage::setupTexture(Pixmap &pix, bool upload, uint internalFormat, int xWrapType, int yWrapType,
	uint usedX, uint usedY, uint hints, uint filter)
{
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	xSize = usedX;
	ySize = usedY;
	#endif
	//logMsg("createGLTexture");
	GLenum texTarget = GL_TEXTURE_2D;
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	if(surfaceTextureConf.use && (hints & BufferImage::HINT_STREAM))
	{
		texTarget = GL_TEXTURE_EXTERNAL_OES;
	}
	#endif

	auto texRef = newTexRef();
	if(texRef == 0)
	{
		logMsg("error getting new texture reference");
		return 0;
	}

	//logMsg("binding texture %u to target %d after creation", texRef, texTarget);
	glcBindTexture(texTarget, texRef);
	setDefaultImageTextureParams(filter, hasMipmaps() ? MIPMAP_NEAREST : MIPMAP_NONE, xWrapType, yWrapType, usedX, usedY, texTarget);

	bool includePadding = 0; //include extra bytes when x != pitch ?
	if(hints & BufferImage::HINT_STREAM)
	{
		#if defined(CONFIG_BASE_PS3)
		logMsg("optimizing texture for frequent updates");
		glTexParameteri(texTarget, GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_LINEAR_SYSTEM_SCE);
		#endif
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		if(surfaceTextureConf.use)
		{
			logMsg("using SurfaceTexture, %dx%d %s", usedX, usedY, pix.format.name);
			pix.x = usedX;
			pix.y = usedY;
			auto *surfaceTex = new SurfaceTextureBufferImage;
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
			auto *directTex = new DirectTextureBufferImage;
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
		logMsg("%s texture 0x%X with size %dx%d, internal format %s, from image %s:%s", upload ? "uploading" : "creating", texRef, pix.x, pix.y, glImageFormatToString(internalFormat), glImageFormatToString(format), glDataTypeToString(dataType));
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
		impl = new TextureBufferImage;
	#endif
	textureDesc().tid = texRef;
	return 1;
}

#if defined(CONFIG_RESOURCE_IMAGE)
/*CallResult BufferImage::subInit(ResourceImage &img, int x, int y, int xSize, int ySize)
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

/*CallResult BufferImage::init(ResourceImage &img, uint filter, uint hints, bool textured)
{
	using namespace Gfx;
	var_selfs(hints);
	testMipmapSupport(img.width(), img.height());
	//logMsg("BufferImage::init");
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
}*/
#endif

CallResult BufferImage::init(GfxImageSource &img, uint filter, uint hints, bool textured)
{
	deinit();

	var_selfs(hints);
	testMipmapSupport(img.width(), img.height());
	//logMsg("BufferImage::init");
	int wrapMode = bestClampMode(textured);

	uint texX, texY;
	textureSizeSupport.findBufferXYPixels(texX, texY, img.width(), img.height());

	auto pixFmt = swapRGBToPreferedOrder(img.pixelFormat());
	Pixmap texPix(*pixFmt);
	uint uploadPixStoreSize = texX * texY * pixFmt->bytesPerPixel;
	#if __APPLE__
	//logMsg("alloc in heap"); // for low stack limits
	uchar *uploadPixStore = (uchar*)mem_calloc(uploadPixStoreSize);
	if(!uploadPixStore)
		return OUT_OF_MEMORY;
	#else
	uchar uploadPixStore[uploadPixStoreSize] __attribute__ ((aligned (8)));
	mem_zero(uploadPixStore, uploadPixStoreSize);
	#endif
	texPix.init(uploadPixStore, texX, texY, 0);
	img.getImage(texPix);
	if(!setupTexture(texPix, 1, pixelToOGLInternalFormat(texPix.format), wrapMode,
			wrapMode, img.width(), img.height(), hints, filter))
	{
		#if __APPLE__
		mem_free(uploadPixStore);
		#endif
		return INVALID_PARAMETER;
	}
	#if __APPLE__
	mem_free(uploadPixStore);
	#endif

	textureDesc().xStart = pixelToTexC((uint)0, texPix.x);
	textureDesc().yStart = pixelToTexC((uint)0, texPix.y);
	textureDesc().xEnd = pixelToTexC(img.width(), texPix.x);
	textureDesc().yEnd = pixelToTexC(img.height(), texPix.y);

	return OK;
}

void BufferImage::testMipmapSupport(uint x, uint y)
{
	hasMipmaps_ = usingMipmaping() &&
			!(hints & HINT_STREAM) && !(hints & HINT_NO_MINIFY)
			&& Gfx::textureSizeSupport.supportsMipmaps(x, y);
}

CallResult BufferImage::init(Pixmap &pix, bool upload, uint filter, uint hints, bool textured)
{
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

void BufferImage::write(Pixmap &p) { BufferImageImpl::write(p, hints); }
void BufferImage::write(Pixmap &p, uint assumeAlign) { BufferImageImpl::write(p, hints, assumeAlign); }
void BufferImage::replace(Pixmap &p)
{
	BufferImageImpl::replace(p, hints);
}
void BufferImage::unlock(Pixmap *p) { BufferImageImpl::unlock(p, hints); }

void BufferImage::deinit()
{
	if(!isInit())
		return;
	BufferImageImpl::deinit();
}

}

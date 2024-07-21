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

#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math.hh>
#include <imagine/util/bit.hh>
#include <imagine/data-type/image/PixmapSource.hh>
#include "utils.hh"
#include <cstdlib>
#include <algorithm>

#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#endif

#ifndef GL_TEXTURE_SWIZZLE_G
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#endif

#ifndef GL_TEXTURE_SWIZZLE_B
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#endif

#ifndef GL_TEXTURE_SWIZZLE_A
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#endif

#ifndef GL_TEXTURE_SWIZZLE_RGBA
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

#ifndef GL_RGB5
#define GL_RGB5 0x8050
#endif

namespace IG::Gfx
{

constexpr SystemLogger log{"GLTexture"};

static int makeUnpackAlignment(uintptr_t addr)
{
	// find best alignment with lower 3 bits
	static constexpr int map[]
	{
		8, 1, 2, 1, 4, 1, 2, 1
	};
	return map[addr & 7];
}

static int unpackAlignForAddrAndPitch(const void *srcAddr, uint32_t pitch)
{
	int alignmentForAddr = makeUnpackAlignment((uintptr_t)srcAddr);
	int alignmentForPitch = makeUnpackAlignment(pitch);
	if(alignmentForAddr < alignmentForPitch)
	{
		/*log.info("using lowest alignment of address {} ({}) and pitch {} ({})",
			srcAddr, alignmentForAddr, pitch, alignmentForPitch);*/
	}
	return std::min(alignmentForPitch, alignmentForAddr);
}

static GLenum makeGLDataType(PixelFormat format)
{
	switch(format)
	{
		case PixelFmtRGBA8888:
		case PixelFmtBGRA8888:
			if constexpr(!Config::Gfx::OPENGL_ES)
			{
				return GL_UNSIGNED_INT_8_8_8_8_REV;
			} [[fallthrough]];
		case PixelFmtRGB888:
		case PixelFmtI8:
		case PixelFmtIA88:
		case PixelFmtA8:
			return GL_UNSIGNED_BYTE;
		case PixelFmtRGB565:
			return GL_UNSIGNED_SHORT_5_6_5;
		case PixelFmtRGBA5551:
			return GL_UNSIGNED_SHORT_5_5_5_1;
		case PixelFmtRGBA4444:
			return GL_UNSIGNED_SHORT_4_4_4_4;
		default: std::unreachable();
	}
}

static GLenum makeGLFormat(const Renderer &r, PixelFormat format)
{
	switch(format)
	{
		case PixelFmtI8:
			return r.support.luminanceFormat;
		case PixelFmtIA88:
			return r.support.luminanceAlphaFormat;
		case PixelFmtA8:
			return r.support.alphaFormat;
		case PixelFmtRGB888:
		case PixelFmtRGB565:
			return GL_RGB;
		case PixelFmtRGBA8888:
		case PixelFmtRGBA5551:
		case PixelFmtRGBA4444:
			return GL_RGBA;
		case PixelFmtBGRA8888:
			assert(r.support.hasBGRPixels);
			return GL_BGRA;
		default: std::unreachable();
	}
}

static GLenum makeGLESInternalFormat(const Renderer &r, PixelFormat format)
{
	if(Config::envIsIOS && format == PixelFmtBGRA8888) // Apple's BGRA extension loosens the internalformat match requirement
		return GL_RGBA;
	return makeGLFormat(r, format); // OpenGL ES manual states internalformat always equals format
}

static GLenum makeGLSizedInternalFormat(const Renderer &r, PixelFormat format, bool isSrgb)
{
	switch(format)
	{
		case PixelFmtBGRA8888:
		case PixelFmtRGBA8888:
			return isSrgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		case PixelFmtRGB565:
			return Config::Gfx::OPENGL_ES ? GL_RGB565 : GL_RGB5;
		case PixelFmtRGBA5551:
			return GL_RGB5_A1;
		case PixelFmtRGBA4444:
			return GL_RGBA4;
		case PixelFmtI8:
			return r.support.luminanceInternalFormat;
		case PixelFmtIA88:
			return r.support.luminanceAlphaInternalFormat;
		case PixelFmtA8:
			return r.support.alphaInternalFormat;
		default: std::unreachable();
	}
}

static int makeGLInternalFormat(const Renderer &r, PixelFormat format, bool isSrgb)
{
	return Config::Gfx::OPENGL_ES ? makeGLESInternalFormat(r, format)
		: makeGLSizedInternalFormat(r, format, isSrgb);
}

static TextureType typeForPixelFormat(PixelFormat format)
{
	return (format == PixelFmtA8) ? TextureType::T2D_1 :
		(format == PixelFmtIA88) ? TextureType::T2D_2 :
		TextureType::T2D_4;
}

static TextureConfig configWithLoadedImagePixmap(PixmapDesc desc, bool makeMipmaps, TextureSamplerConfig samplerConf)
{
	TextureConfig config{desc, samplerConf};
	config.setWillGenerateMipmaps(makeMipmaps);
	return config;
}

static bool loadImageSource(Texture &texture, Data::PixmapSource img, bool makeMipmaps)
{
	auto imgPix = img.pixmapView();
	TextureWriteFlags writeFlags{.makeMipmaps = makeMipmaps};
	if(imgPix)
	{
		//log.debug("writing image source pixmap to texture");
		texture.write(0, imgPix, {}, writeFlags);
	}
	else
	{
		auto lockBuff = texture.lock(0);
		if(!lockBuff) [[unlikely]]
			return false;
		//log.debug("writing image source into texture pixel buffer");
		img.write(lockBuff.pixmap());
		texture.unlock(lockBuff, writeFlags);
	}
	return true;
}

MutablePixmapView LockedTextureBuffer::pixmap() const
{
	return pix;
}

IG::WindowRect LockedTextureBuffer::sourceDirtyRect() const
{
	return srcDirtyRect;
}

LockedTextureBuffer::operator bool() const
{
	return (bool)pix;
}

Texture::Texture(RendererTask &r, TextureConfig config):
	GLTexture{r}
{
	init(r, config);
}

Texture::Texture(RendererTask &r, IG::Data::PixmapSource img, TextureSamplerConfig samplerConf, bool makeMipmaps):
	GLTexture{r}
{
	init(r, configWithLoadedImagePixmap(img.pixmapView().desc(), makeMipmaps, samplerConf));
	loadImageSource(*static_cast<Texture*>(this), img, makeMipmaps);
}

TextureConfig GLTexture::baseInit(RendererTask &r, TextureConfig config)
{
	if(config.willGenerateMipmaps() && !r.renderer().support.hasImmutableTexStorage)
	{
		// when using glGenerateMipmaps exclusively, there is no need to define
		// all texture levels with glTexImage2D beforehand
		config.levels = 1;
	}
	return config;
}

void GLTexture::init(RendererTask &r, TextureConfig config)
{
	config = baseInit(r, config);
	static_cast<Texture*>(this)->setFormat(config.pixmapDesc, config.levels, config.colorSpace, config.samplerConfig);
}

void destroyGLTextureRef(RendererTask &task, TextureRef texName)
{
	log.info("deleting texture:0x{:X}", texName);
	task.run(
		[texName]()
		{
			glDeleteTextures(1, &texName);
		});
}

int Texture::bestAlignment(PixmapView p)
{
	return unpackAlignForAddrAndPitch(p.data(), p.pitchBytes());
}

bool GLTexture::canUseMipmaps(const Renderer &r) const
{
	return r.support.textureSizeSupport.supportsMipmaps(pixDesc.w(), pixDesc.h());
}

bool Texture::canUseMipmaps() const
{
	return GLTexture::canUseMipmaps(renderer());
}

GLenum GLTexture::target() const
{
	return Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL && type_ == TextureType::T2D_EXTERNAL ?
			GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
}

bool Texture::generateMipmaps()
{
	if(!texName()) [[unlikely]]
	{
		log.error("called generateMipmaps() on uninitialized texture");
		return false;
	}
	if(!canUseMipmaps())
		return false;
	task().run(
		[texName = texName()]()
		{
			glBindTexture(GL_TEXTURE_2D, texName);
			log.info("generating mipmaps for texture:0x{:X}", texName);
			glGenerateMipmap(GL_TEXTURE_2D);
		});
	updateLevelsForMipmapGeneration();
	return true;
}

int Texture::levels() const
{
	return levels_;
}

bool Texture::setFormat(PixmapDesc desc, int levels, ColorSpace colorSpace, TextureSamplerConfig samplerConf)
{
	assumeExpr(desc.w());
	assumeExpr(desc.h());
	if(renderer().support.textureSizeSupport.supportsMipmaps(desc.w(), desc.h()))
	{
		if(!levels)
			levels = fls(static_cast<unsigned>(desc.w() | desc.h()));
	}
	else
	{
		levels = 1;
	}
	SamplerParams samplerParams = asSamplerParams(samplerConf);
	if(renderer().support.hasImmutableTexStorage)
	{
		bool isSrgb = renderer().supportedColorSpace(desc.format, colorSpace) == ColorSpace::SRGB;
		task().runSync(
			[=, &r = std::as_const(renderer()), &texNameRef = texName_.get()](GLTask::TaskContext ctx)
			{
				auto texName = makeGLTextureName(texNameRef);
				texNameRef = texName;
				ctx.notifySemaphore();
				glBindTexture(GL_TEXTURE_2D, texName);
				auto internalFormat = makeGLSizedInternalFormat(r, desc.format, isSrgb);
				log.info("texture:0x{:X} storage size:{}x{} levels:{} internal format:{} {}",
					texName, desc.w(), desc.h(), levels, glImageFormatToString(internalFormat),
					desc.format == IG::PixelFmtBGRA8888 ? "write format:BGRA" : "");
				runGLChecked(
					[&]()
					{
						r.support.glTexStorage2D(GL_TEXTURE_2D, levels, internalFormat, desc.w(), desc.h());
					}, "glTexStorage2D()");
				setSwizzleForFormatInGL(r, desc.format, texName);
				setSamplerParamsInGL(samplerParams);
			});
	}
	else
	{
		bool remakeTexName = levels != levels_; // make new texture name whenever number of levels changes
		task().GLTask::run(
			[=, &r = std::as_const(renderer()), &texNameRef = texName_.get(), currTexName = texName()](GLTask::TaskContext ctx)
			{
				auto texName = currTexName; // a copy of texName_ is passed by value for the async case to avoid accessing this->texName_
				if(remakeTexName)
				{
					texName = makeGLTextureName(texName);
					texNameRef = texName;
					ctx.notifySemaphore();
				}
				glBindTexture(GL_TEXTURE_2D, texName);
				auto format = makeGLFormat(r, desc.format);
				auto dataType = makeGLDataType(desc.format);
				auto internalFormat = makeGLInternalFormat(r, desc.format, false);
				log.info("texture:0x{:X} storage size:{}x{} levels:{} internal format:{} image format:{}:{} {}",
					texName, desc.w(), desc.h(), levels, glImageFormatToString(internalFormat),
					glImageFormatToString(format), glDataTypeToString(dataType),
					desc.format == IG::PixelFmtBGRA8888 && internalFormat != GL_BGRA ? "write format:BGRA" : "");
				int w = desc.w(), h = desc.h();
				for(auto i : iotaCount(levels))
				{
					runGLChecked(
						[&]()
						{
							glTexImage2D(GL_TEXTURE_2D, i, internalFormat, w, h, 0, format, dataType, nullptr);
						}, "glTexImage2D()");
					w = std::max(1, (w / 2));
					h = std::max(1, (h / 2));
				}
				setSwizzleForFormatInGL(r, desc.format, texName);
				if(remakeTexName)
					setSamplerParamsInGL(samplerParams);
			}, remakeTexName ? MessageReplyMode::wait : MessageReplyMode::none);
	}
	updateFormatInfo(desc, levels);
	return {};
}

void Texture::writeAligned(int level, PixmapView pixmap, WPt destPos, int assumeAlign, TextureWriteFlags writeFlags)
{
	//log.debug("writing pixmap {}x{} to pos {}x{}", pixmap.x, pixmap.y, destPos.x, destPos.y);
	if(!texName()) [[unlikely]]
	{
		log.error("called writeAligned() on uninitialized texture");
		return;
	}
	auto &r = renderer();
	assumeExpr(destPos.x + pixmap.w() <= size(level).x);
	assumeExpr(destPos.y + pixmap.h() <= size(level).y);
	assumeExpr(pixmap.format().bytesPerPixel() == pixDesc.format.bytesPerPixel());
	if(!assumeAlign)
		assumeAlign = unpackAlignForAddrAndPitch(pixmap.data(), pixmap.pitchBytes());
	if((uintptr_t)pixmap.data() % (uintptr_t)assumeAlign != 0)
	{
		bug_unreachable("expected data from address %p to be aligned to %u bytes", pixmap.data(), assumeAlign);
	}
	auto hasUnpackRowLength = r.support.hasUnpackRowLength;
	bool makeMipmaps = writeFlags.makeMipmaps && canUseMipmaps();
	if(hasUnpackRowLength || !pixmap.isPadded())
	{
		task().run(
			[=, &r = std::as_const(r), texName = texName()]()
			{
				glBindTexture(GL_TEXTURE_2D, texName);
				glPixelStorei(GL_UNPACK_ALIGNMENT, assumeAlign);
				if(hasUnpackRowLength)
					glPixelStorei(GL_UNPACK_ROW_LENGTH, pixmap.pitchPx());
				GLenum format = makeGLFormat(r, pixmap.format());
				GLenum dataType = makeGLDataType(pixmap.format());
				runGLCheckedVerbose(
					[&]()
					{
						glTexSubImage2D(GL_TEXTURE_2D, level, destPos.x, destPos.y,
							pixmap.w(), pixmap.h(), format, dataType, pixmap.data());
					}, "glTexSubImage2D()");
				if(makeMipmaps)
				{
					log.info("generating mipmaps for texture:0x{:X}", texName);
					glGenerateMipmap(GL_TEXTURE_2D);
				}
			}, writeFlags.async ? MessageReplyMode::none : MessageReplyMode::wait);
		if(makeMipmaps)
		{
			updateLevelsForMipmapGeneration();
		}
	}
	else
	{
		// must copy to buffer without extra pitch pixels
		log.debug("texture:%u needs temporary buffer to copy pixmap with width:{} pitch:{}", texName(), pixmap.w(), pixmap.pitchPx());
		IG::WindowRect lockRect{{}, pixmap.size()};
		lockRect += destPos;
		auto lockBuff = lock(level, lockRect);
		if(!lockBuff) [[unlikely]]
		{
			log.error("error getting buffer for writeAligned()");
			return;
		}
		assumeExpr(pixmap.format().bytesPerPixel() == lockBuff.pixmap().format().bytesPerPixel());
		lockBuff.pixmap().write(pixmap);
		unlock(lockBuff, writeFlags);
	}
}

void Texture::write(int level, PixmapView pixmap, WPt destPos, TextureWriteFlags commitFlags)
{
	writeAligned(level, pixmap, destPos, bestAlignment(pixmap), commitFlags);
}

void Texture::clear(int level)
{
	auto lockBuff = lock(level, {.clear = true});
	if(!lockBuff) [[unlikely]]
	{
		log.error("error getting buffer for clear()");
		return;
	}
	unlock(lockBuff);
}

LockedTextureBuffer Texture::lock(int level, TextureBufferFlags bufferFlags)
{
	return lock(level, {{}, size(level)}, bufferFlags);
}

LockedTextureBuffer Texture::lock(int level, IG::WindowRect rect, TextureBufferFlags bufferFlags)
{
	if(!texName()) [[unlikely]]
	{
		log.error("called lock() on uninitialized texture");
		return {};
	}
	assumeExpr(rect.x2  <= size(level).x);
	assumeExpr(rect.y2 <= size(level).y);
	const auto bufferBytes = pixDesc.format.pixelBytes(rect.xSize() * rect.ySize());
	char *data;
	if(bufferFlags.clear)
		data = (char*)std::calloc(1, bufferBytes);
	else
		data = (char*)std::malloc(bufferBytes);
	if(!data) [[unlikely]]
	{
		log.error("failed allocating {} bytes for pixel buffer", bufferBytes);
		return {};
	}
	MutablePixmapView pix{{rect.size(), pixDesc.format}, data};
	return {data, pix, rect, level, true};
}

void Texture::unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags)
{
	if(!lockBuff) [[unlikely]]
		return;
	if(lockBuff.pbo())
	{
		assert(renderer().support.hasPBOFuncs);
	}
	bool makeMipmaps = writeFlags.makeMipmaps && canUseMipmaps();
	if(makeMipmaps)
	{
		updateLevelsForMipmapGeneration();
	}
	task().run(
		[&r = std::as_const(renderer()), pix = lockBuff.pixmap(), bufferOffset = lockBuff.bufferOffset(),
		 texName = texName(), destPos = WPt{lockBuff.sourceDirtyRect().x, lockBuff.sourceDirtyRect().y},
		 pbo = lockBuff.pbo(), level = lockBuff.level(),
		 shouldFreeBuffer = lockBuff.shouldFreeBuffer(), makeMipmaps]()
		{
			glBindTexture(GL_TEXTURE_2D, texName);
			glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignForAddrAndPitch(nullptr, pix.pitchBytes()));
			if(pbo)
			{
				assumeExpr(r.support.hasUnpackRowLength);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				r.support.glFlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER, (GLintptr)bufferOffset, pix.bytes());
			}
			else
			{
				if(r.support.hasUnpackRowLength)
					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			}
			GLenum format = makeGLFormat(r, pix.format());
			GLenum dataType = makeGLDataType(pix.format());
			runGLCheckedVerbose(
				[&]()
				{
					glTexSubImage2D(GL_TEXTURE_2D, level, destPos.x, destPos.y,
						pix.w(), pix.h(), format, dataType, bufferOffset);
				}, "glTexSubImage2D()");
			if(pbo)
			{
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			}
			else if(shouldFreeBuffer)
			{
				std::free(pix.data());
			}
			if(makeMipmaps)
			{
				log.info("generating mipmaps for texture:0x{:X}", texName);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
		});
}

WSize Texture::size(int level) const
{
	assert(levels_);
	int w = pixDesc.w(), h = pixDesc.h();
	for([[maybe_unused]] auto i : iotaCount(level))
	{
		w = std::max(1, (w / 2));
		h = std::max(1, (h / 2));
	}
	return {(int)w, (int)h};
}

PixmapDesc Texture::pixmapDesc() const
{
	return pixDesc;
}

void Texture::setSampler(TextureSamplerConfig samplerConf)
{
	if(!texName()) [[unlikely]]
		return;
	task().run(
		[target = target(), texName = texName(), params = asSamplerParams(samplerConf)]()
		{
			glBindTexture(target, texName);
			setSamplerParamsInGL(params, target);
		});
}

Texture::operator bool() const
{
	return texName();
}

Renderer &Texture::renderer() const
{
	return GLTexture::renderer();
}

RendererTask &Texture::task() const
{
	return GLTexture::task();
}

Texture::operator TextureSpan() const
{
	return {this};
}

GLuint GLTexture::texName() const
{
	return texName_.get();
}

RendererTask *GLTexture::taskPtr() const
{
	return texName_.get_deleter().rTaskPtr;
}

Renderer &GLTexture::renderer() const
{
	return task().renderer();
}

RendererTask &GLTexture::task() const
{
	assumeExpr(taskPtr());
	return *taskPtr();
}

static void verifyCurrentTexture2D(TextureRef tex)
{
	if(!Config::DEBUG_BUILD)
		return;
	GLint realTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &realTexture);
	if(tex != (GLuint)realTexture)
	{
		bug_unreachable("out of sync, expected %u but got %u, TEXTURE_2D", tex, realTexture);
	}
}

void GLTexture::setSwizzleForFormatInGL(const Renderer &r, PixelFormatId format, GLuint tex)
{
	if(!r.support.hasTextureSwizzle)
		return;
	verifyCurrentTexture2D(tex);
	const GLint swizzleMaskRGBA[] {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
	const GLint swizzleMaskIA88[] {GL_RED, GL_RED, GL_RED, GL_GREEN};
	const GLint swizzleMaskA8[] {GL_ONE, GL_ONE, GL_ONE, GL_RED};
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		auto &swizzleMask = (format == PixelFmtIA88) ? swizzleMaskIA88
				: (format == PixelFmtA8) ? swizzleMaskA8
				: swizzleMaskRGBA;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swizzleMask[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swizzleMask[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swizzleMask[2]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swizzleMask[3]);
	}
	else
	{
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (format == PixelFmtIA88) ? swizzleMaskIA88
				: (format == PixelFmtA8) ? swizzleMaskA8
				: swizzleMaskRGBA);
	}
}

static void setTexParameteri(GLenum target, GLenum pname, GLint param)
{
	runGLCheckedVerbose(
		[&]()
		{
			glTexParameteri(target, pname, param);
		}, "glTexParameteri()");
}

void GLTexture::setSamplerParamsInGL(SamplerParams params, GLenum target)
{
	assert(params.magFilter);
	setTexParameteri(target, GL_TEXTURE_MAG_FILTER, params.magFilter);
	setTexParameteri(target, GL_TEXTURE_MIN_FILTER, params.minFilter);
	setTexParameteri(target, GL_TEXTURE_WRAP_S, params.xWrapMode);
	setTexParameteri(target, GL_TEXTURE_WRAP_T, params.yWrapMode);
}

void GLTexture::updateFormatInfo(PixmapDesc desc, int8_t levels, GLenum target)
{
	assert(levels);
	levels_ = levels;
	pixDesc = desc;
	if(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL && target == GL_TEXTURE_EXTERNAL_OES)
		type_ = TextureType::T2D_EXTERNAL;
	else
		type_ = typeForPixelFormat(desc.format);
}

#ifdef __ANDROID__
void GLTexture::initWithEGLImage(EGLImageKHR eglImg, PixmapDesc desc, SamplerParams samplerParams, bool isMutable)
{
	auto &r = renderer();
	if(r.support.hasEGLTextureStorage() && !isMutable)
	{
		task().runSync(
			[=, &r = std::as_const(r), &texNameRef = texName_.get(), formatID = desc.format.id](GLTask::TaskContext ctx)
			{
				auto texName = makeGLTextureName(texNameRef);
				texNameRef = texName;
				glBindTexture(GL_TEXTURE_2D, texName);
				if(eglImg)
				{
					log.info("setting immutable texture:{} with EGL image:{}", texName, eglImg);
					runGLChecked(
						[&]()
						{
							r.support.glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, (GLeglImageOES)eglImg, nullptr);
						}, "glEGLImageTargetTexStorageEXT()");
				}
				ctx.notifySemaphore();
				setSwizzleForFormatInGL(r, formatID, texName);
				setSamplerParamsInGL(samplerParams);
			});
	}
	else
	{
		task().runSync(
			[=, &r = std::as_const(r), &texNameRef = texName_.get(), formatID = desc.format.id](GLTask::TaskContext ctx)
			{
				auto texName = texNameRef;
				bool madeTexName = false;
				if(!texName) [[unlikely]] // texture storage is mutable, only need to make name once
				{
					glGenTextures(1, &texName);
					texNameRef = texName;
					madeTexName = true;
				}
				glBindTexture(GL_TEXTURE_2D, texName);
				if(eglImg)
				{
					log.info("setting texture:0x{:X} with EGL image:{}", texName, eglImg);
					runGLChecked(
						[&]()
						{
							glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)eglImg);
						}, "glEGLImageTargetTexture2DOES()");
				}
				ctx.notifySemaphore();
				setSwizzleForFormatInGL(r, formatID, texName);
				if(madeTexName)
					setSamplerParamsInGL(samplerParams);
			});
	}
	updateFormatInfo(desc, 1);
}

void GLTexture::updateWithEGLImage(EGLImageKHR eglImg)
{
	task().GLTask::run(
		[=, texName = texName()](GLTask::TaskContext)
		{
			glBindTexture(GL_TEXTURE_2D, texName);
			assumeExpr(eglImg);
			runGLChecked(
				[&]()
				{
					glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)eglImg);
				}, "glEGLImageTargetTexture2DOES()");
		});
}

#endif

void GLTexture::updateLevelsForMipmapGeneration()
{
	if(!renderer().support.hasImmutableTexStorage)
	{
		// all possible levels generated by glGenerateMipmap
		levels_ = fls(static_cast<unsigned>(pixDesc.w() | pixDesc.h()));
	}
}

LockedTextureBuffer GLTexture::lockedBuffer(void *data, int pitchBytes, TextureBufferFlags bufferFlags)
{
	auto &tex = *static_cast<Texture*>(this);
	IG::WindowRect fullRect{{}, tex.size(0)};
	MutablePixmapView pix{tex.pixmapDesc(), data, {pitchBytes, MutablePixmapView::Units::BYTE}};
	if(bufferFlags.clear)
		pix.clear();
	return {nullptr, pix, fullRect, 0, false};
}

TextureSpan::operator bool() const
{
	return texturePtr && (bool)*texturePtr;
}

TextureSpan::operator TextureBinding() const
{
	if(!texturePtr)
		return {};
	return texturePtr->binding();
}

}

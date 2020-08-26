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

#define LOGTAG "GLTexture"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math/int.hh>
#include <imagine/data-type/image/GfxImageSource.hh>
#include "private.hh"
#include <cstdlib>
#include <algorithm>

using namespace IG;

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

namespace Gfx
{

static uint8_t makeUnpackAlignment(uintptr_t addr)
{
	// find best alignment with lower 3 bits
	constexpr uint8_t map[]
	{
		8, 1, 2, 1, 4, 1, 2, 1
	};
	return map[addr & 7];
}

static uint8_t unpackAlignForAddrAndPitch(void *srcAddr, uint32_t pitch)
{
	uint8_t alignmentForAddr = makeUnpackAlignment((uintptr_t)srcAddr);
	uint8_t alignmentForPitch = makeUnpackAlignment(pitch);
	if(alignmentForAddr < alignmentForPitch)
	{
		/*logMsg("using lowest alignment of address %p (%d) and pitch %d (%d)",
			srcAddr, alignmentForAddr, pitch, alignmentForPitch);*/
	}
	return std::min(alignmentForPitch, alignmentForAddr);
}

static GLenum makeGLDataType(IG::PixelFormatID format)
{
	switch(format)
	{
		case PIXEL_RGBA8888:
		case PIXEL_BGRA8888:
		#if !defined CONFIG_GFX_OPENGL_ES
			return GL_UNSIGNED_INT_8_8_8_8_REV;
		#endif
		case PIXEL_ARGB8888:
		case PIXEL_ABGR8888:
		#if !defined CONFIG_GFX_OPENGL_ES
			return GL_UNSIGNED_INT_8_8_8_8;
		#endif
		case PIXEL_RGB888:
		case PIXEL_BGR888:
		case PIXEL_I8:
		case PIXEL_IA88:
		case PIXEL_A8:
			return GL_UNSIGNED_BYTE;
		case PIXEL_RGB565:
			return GL_UNSIGNED_SHORT_5_6_5;
		case PIXEL_RGBA5551:
			return GL_UNSIGNED_SHORT_5_5_5_1;
		case PIXEL_RGBA4444:
			return GL_UNSIGNED_SHORT_4_4_4_4;
		#if !defined CONFIG_GFX_OPENGL_ES
		case PIXEL_ABGR4444:
			return GL_UNSIGNED_SHORT_4_4_4_4_REV;
		case PIXEL_ABGR1555:
			return GL_UNSIGNED_SHORT_1_5_5_5_REV;
		#endif
		default: bug_unreachable("format == %d", format); return 0;
	}
}

static GLenum makeGLFormat(Renderer &r, IG::PixelFormatID format)
{
	switch(format)
	{
		case PIXEL_I8:
			return r.support.luminanceFormat;
		case PIXEL_IA88:
			return r.support.luminanceAlphaFormat;
		case PIXEL_A8:
			return r.support.alphaFormat;
		case PIXEL_RGB888:
		case PIXEL_RGB565:
			return GL_RGB;
		case PIXEL_RGBA8888:
		case PIXEL_ARGB8888:
		case PIXEL_RGBA5551:
		case PIXEL_RGBA4444:
			return GL_RGBA;
		#if !defined CONFIG_GFX_OPENGL_ES
		case PIXEL_BGR888:
			assert(r.support.hasBGRPixels);
			return GL_BGR;
		case PIXEL_ABGR8888:
		case PIXEL_BGRA8888:
		case PIXEL_ABGR4444:
		case PIXEL_ABGR1555:
			assert(r.support.hasBGRPixels);
			return GL_BGRA;
		#endif
		default: bug_unreachable("format == %d", format); return 0;
	}
}

static int makeGLESInternalFormat(Renderer &r, IG::PixelFormatID format)
{
	if(format == PIXEL_BGRA8888) // Apple's BGRA extension loosens the internalformat match requirement
		return r.support.bgrInternalFormat;
	else return makeGLFormat(r, format); // OpenGL ES manual states internalformat always equals format
}

static int makeGLSizedInternalFormat(Renderer &r, IG::PixelFormatID format)
{
	switch(format)
	{
		case PIXEL_BGRA8888:
		case PIXEL_ARGB8888:
		case PIXEL_ABGR8888:
		case PIXEL_RGBA8888:
			return GL_RGBA8;
		case PIXEL_RGB888:
		case PIXEL_BGR888:
			return GL_RGB8;
		case PIXEL_RGB565:
			#if defined CONFIG_GFX_OPENGL_ES
			return GL_RGB565;
			#else
			return GL_RGB5;
			#endif
		case PIXEL_ABGR1555:
		case PIXEL_RGBA5551:
			return GL_RGB5_A1;
		case PIXEL_RGBA4444:
		case PIXEL_ABGR4444:
			return GL_RGBA4;
		case PIXEL_I8:
			return r.support.luminanceInternalFormat;
		case PIXEL_IA88:
			return r.support.luminanceAlphaInternalFormat;
		case PIXEL_A8:
			return r.support.alphaInternalFormat;
		default: bug_unreachable("format == %d", format); return 0;
	}
}

static int makeGLInternalFormat(Renderer &r, PixelFormatID format)
{
	return Config::Gfx::OPENGL_ES ? makeGLESInternalFormat(r, format)
		: makeGLSizedInternalFormat(r, format);
}

static uint32_t typeForPixelFormat(PixelFormatID format)
{
	return (format == PIXEL_A8) ? TEX_2D_1 :
		(format == PIXEL_IA88) ? TEX_2D_2 :
		TEX_2D_4;
}

static TextureConfig configWithLoadedImagePixmap(IG::PixmapDesc desc, bool makeMipmaps)
{
	TextureConfig config{desc};
	config.setWillGenerateMipmaps(makeMipmaps);
	return config;
}

static IG::ErrorCode loadImageSource(Texture &texture, GfxImageSource &img, bool makeMipmaps)
{
	auto imgPix = img.pixmapView();
	if(imgPix)
	{
		//logDMsg("writing image source pixmap to texture");
		texture.write(0, imgPix, {}, makeMipmaps ? Texture::WRITE_FLAG_MAKE_MIPMAPS : 0);
	}
	else
	{
		auto lockBuff = texture.lock(0);
		if(unlikely(!lockBuff))
			return {ENOMEM};
		//logDMsg("writing image source into texture pixel buffer");
		img.write(lockBuff.pixmap());
		img.freePixmap();
		texture.unlock(lockBuff, makeMipmaps ? Texture::WRITE_FLAG_MAKE_MIPMAPS : 0);
	}
	return {};
}

IG::Pixmap LockedTextureBuffer::pixmap() const
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

GLTexture::GLTexture(Renderer &r, TextureConfig config, IG::ErrorCode *errorPtr):
	r{&r}
{
	IG::ErrorCode err = init(r, config);
	if(unlikely(err && errorPtr))
	{
		*errorPtr = err;
	}
}

GLTexture::GLTexture(Renderer &r, GfxImageSource &img, bool makeMipmaps, IG::ErrorCode *errorPtr):
	r{&r}
{
	IG::ErrorCode err;
	auto setError = IG::scopeGuard([&](){ if(unlikely(err && errorPtr)) { *errorPtr = err; } });
	if(err = init(r, configWithLoadedImagePixmap(img.pixmapView(), makeMipmaps));
		unlikely(err))
	{
		return;
	}
	err = loadImageSource(*static_cast<Texture*>(this), img, makeMipmaps);
}

Texture::Texture(Texture &&o)
{
	*this = std::move(o);
}

Texture &Texture::operator=(Texture &&o)
{
	deinit();
	GLTexture::operator=(o);
	o.r = nullptr;
	o.texName_ = 0;
	return *this;
}

GLTexture::~GLTexture()
{
	deinit();
}

TextureConfig GLTexture::baseInit(Renderer &r, TextureConfig config)
{
	if(config.willGenerateMipmaps() && !r.support.hasImmutableTexStorage)
	{
		// when using glGenerateMipmaps exclusively, there is no need to define
		// all texture levels with glTexImage2D beforehand
		config.setLevels(1);
	}
	return config;
}

IG::ErrorCode GLTexture::init(Renderer &r, TextureConfig config)
{
	config = baseInit(r, config);
	return static_cast<Texture*>(this)->setFormat(config.pixmapDesc(), config.levels());
}

void GLTexture::deinit()
{
	if(!texName_ || !r->hasGLTask())
		return;
	logMsg("deinit texture:0x%X", texName_);
	assumeExpr(r);
	r->runGLTaskSync(
		[this]()
		{
			glDeleteTextures(1, &texName_);
			texName_ = 0;
		});
}

uint8_t Texture::bestAlignment(IG::Pixmap p)
{
	return unpackAlignForAddrAndPitch(p.pixel({}), p.pitchBytes());
}

bool GLTexture::canUseMipmaps(Renderer &r) const
{
	return r.support.textureSizeSupport.supportsMipmaps(pixDesc.w(), pixDesc.h());
}

bool Texture::canUseMipmaps() const
{
	assumeExpr(r);
	return GLTexture::canUseMipmaps(*r);
}

GLenum GLTexture::target() const
{
	return Config::Gfx::OPENGL_MULTIPLE_TEXTURE_TARGETS && type_ == TEX_2D_EXTERNAL ?
			GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
}

bool Texture::generateMipmaps()
{
	if(unlikely(!texName_))
	{
		logErr("called generateMipmaps() on uninitialized texture");
		return false;
	}
	if(!canUseMipmaps())
		return false;
	assumeExpr(r);
	r->resourceUpdate = true;
	r->runGLTask(
		[r = this->r, texName_ = this->texName_]()
		{
			glBindTexture(GL_TEXTURE_2D, texName_);
			logMsg("generating mipmaps for texture:0x%X", texName_);
			r->support.generateMipmaps(GL_TEXTURE_2D);
		});
	updateLevelsForMipmapGeneration();
	return true;
}

uint32_t Texture::levels() const
{
	return levels_;
}

IG::ErrorCode Texture::setFormat(IG::PixmapDesc desc, uint16_t levels)
{
	assumeExpr(r);
	if(r->support.textureSizeSupport.supportsMipmaps(desc.w(), desc.h()))
	{
		if(!levels)
			levels = fls(desc.w() | desc.h());
	}
	else
	{
		levels = 1;
	}
	r->runGLTaskSync(
		[this, desc, levels]()
		{
			if(r->support.hasImmutableTexStorage)
			{
				sampler = 0;
				texName_ = makeGLTextureName(texName_);
				glBindTexture(GL_TEXTURE_2D, texName_);
				auto internalFormat = makeGLSizedInternalFormat(*r, desc.format());
				logMsg("texture:0x%X storage size:%dx%d levels:%d internal format:%s",
					texName_, desc.w(), desc.h(), levels, glImageFormatToString(internalFormat));
				runGLChecked(
					[&]()
					{
						r->support.glTexStorage2D(GL_TEXTURE_2D, levels, internalFormat, desc.w(), desc.h());
					}, "glTexStorage2D()");
			}
			else
			{
				if(levels != levels_) // make new texture name whenever number of levels changes
				{
					sampler = 0;
					texName_ = makeGLTextureName(texName_);
				}
				glBindTexture(GL_TEXTURE_2D, texName_);
				auto format = makeGLFormat(*r, desc.format());
				auto dataType = makeGLDataType(desc.format());
				auto internalFormat = makeGLInternalFormat(*r, desc.format());
				logMsg("texture:0x%X storage size:%dx%d levels:%d internal format:%s image format:%s:%s",
					texName_, desc.w(), desc.h(), levels, glImageFormatToString(internalFormat), glImageFormatToString(format), glDataTypeToString(dataType));
				uint32_t w = desc.w(), h = desc.h();
				iterateTimes(levels, i)
				{
					runGLChecked(
						[&]()
						{
							glTexImage2D(GL_TEXTURE_2D, i, internalFormat, w, h, 0, format, dataType, nullptr);
						}, "glTexImage2D()");
					w = std::max(1u, (w / 2));
					h = std::max(1u, (h / 2));
				}
			}
		});
	updateFormatInfo(desc, levels);
	return {};
}

void GLTexture::bindTex(RendererCommands &cmds, const TextureSampler &bindSampler) const
{
	if(!texName_)
	{
		logErr("called bindTex() on uninitialized texture");
		return;
	}
	cmds.glcBindTexture(target(), texName_);
	if(!cmds.renderer().support.hasSamplerObjects && bindSampler.name() != sampler)
	{
		logMsg("setting sampler:0x%X for texture:0x%X", (int)bindSampler.name(), texName_);
		sampler = bindSampler.name();
		bindSampler.setTexParams(target());
	}
}

void Texture::writeAligned(uint16_t level, IG::Pixmap pixmap, IG::WP destPos, uint8_t assumeAlign, uint32_t writeFlags)
{
	//logDMsg("writing pixmap %dx%d to pos %dx%d", pixmap.x, pixmap.y, destPos.x, destPos.y);
	if(unlikely(!texName_))
	{
		logErr("called writeAligned() on uninitialized texture");
		return;
	}
	assumeExpr(r);
	assumeExpr(destPos.x + pixmap.w() <= (uint32_t)size(level).x);
	assumeExpr(destPos.y + pixmap.h() <= (uint32_t)size(level).y);
	assumeExpr(pixmap.format() == pixDesc.format());
	if(!assumeAlign)
		assumeAlign = unpackAlignForAddrAndPitch(pixmap.pixel({}), pixmap.pitchBytes());
	if((uintptr_t)pixmap.pixel({}) % (uintptr_t)assumeAlign != 0)
	{
		bug_unreachable("expected data from address %p to be aligned to %u bytes", pixmap.pixel({}), assumeAlign);
	}
	GLenum format = makeGLFormat(*r, pixmap.format());
	GLenum dataType = makeGLDataType(pixmap.format());
	auto hasUnpackRowLength = r->support.hasUnpackRowLength;
	bool makeMipmaps = writeFlags & WRITE_FLAG_MAKE_MIPMAPS && canUseMipmaps();
	if(hasUnpackRowLength || !pixmap.isPadded())
	{
		r->resourceUpdate = true;
		r->runGLTaskSyncConditional(
			[=, r = this->r, texName_ = this->texName_]()
			{
				glBindTexture(GL_TEXTURE_2D, texName_);
				glPixelStorei(GL_UNPACK_ALIGNMENT, assumeAlign);
				if(hasUnpackRowLength)
					glPixelStorei(GL_UNPACK_ROW_LENGTH, pixmap.pitchPixels());
				runGLCheckedVerbose(
					[&]()
					{
						glTexSubImage2D(GL_TEXTURE_2D, level, destPos.x, destPos.y,
							pixmap.w(), pixmap.h(), format, dataType, pixmap.pixel({}));
					}, "glTexSubImage2D()");
				if(makeMipmaps)
				{
					logMsg("generating mipmaps for texture:0x%X", texName_);
					r->support.generateMipmaps(GL_TEXTURE_2D);
				}
			}, !(writeFlags & WRITE_FLAG_ASYNC));
		if(makeMipmaps)
		{
			updateLevelsForMipmapGeneration();
		}
	}
	else
	{
		// must copy to buffer without extra pitch pixels
		IG::WindowRect lockRect{0, 0, pixmap.size().x, pixmap.size().y};
		lockRect += destPos;
		auto lockBuff = lock(level, lockRect);
		if(unlikely(!lockBuff))
		{
			logErr("error getting buffer for writeAligned()");
			return;
		}
		lockBuff.pixmap().write(pixmap);
		unlock(lockBuff, writeFlags);
	}
}

void Texture::write(uint16_t level, IG::Pixmap pixmap, IG::WP destPos, uint32_t commitFlags)
{
	writeAligned(level, pixmap, destPos, bestAlignment(pixmap), commitFlags);
}

void Texture::clear(uint16_t level)
{
	auto lockBuff = lock(level, BUFFER_FLAG_CLEARED);
	if(unlikely(!lockBuff))
	{
		logErr("error getting buffer for clear()");
		return;
	}
	unlock(lockBuff);
}

LockedTextureBuffer Texture::lock(uint16_t level, uint32_t bufferFlags)
{
	return lock(level, {0, 0, size(level).x, size(level).y}, bufferFlags);
}

LockedTextureBuffer Texture::lock(uint16_t level, IG::WindowRect rect, uint32_t bufferFlags)
{
	if(unlikely(!texName_))
	{
		logErr("called lock() on uninitialized texture");
		return {};
	}
	assumeExpr(r);
	assumeExpr(rect.x2  <= size(level).x);
	assumeExpr(rect.y2 <= size(level).y);
	const unsigned bufferBytes = pixDesc.format().pixelBytes(rect.xSize() * rect.ySize());
	char *data;
	if(bufferFlags & BUFFER_FLAG_CLEARED)
		data = (char*)std::calloc(1, bufferBytes);
	else
		data = (char*)std::malloc(bufferBytes);
	if(unlikely(!data))
	{
		logErr("failed allocating %u bytes for pixel buffer", bufferBytes);
		return {};
	}
	r->resourceUpdate = true;
	IG::Pixmap pix{{rect.size(), pixDesc.format()}, data};
	return {data, pix, rect, level, true};
}

void Texture::unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags)
{
	if(unlikely(!lockBuff))
		return;
	assumeExpr(r);
	if(lockBuff.pbo())
		assert(r->support.hasPBOFuncs);
	bool makeMipmaps = writeFlags & WRITE_FLAG_MAKE_MIPMAPS && canUseMipmaps();
	if(makeMipmaps)
	{
		updateLevelsForMipmapGeneration();
	}
	r->runGLTask(
		[=, r = this->r, texName_ = this->texName_, pix = lockBuff.pixmap(), bufferOffset = lockBuff.bufferOffset(),
		 destPos = IG::WP{lockBuff.sourceDirtyRect().x, lockBuff.sourceDirtyRect().y},
		 level = lockBuff.level(), pbo = lockBuff.pbo(), shouldFreeBuffer = lockBuff.shouldFreeBuffer()]()
		{
			glBindTexture(GL_TEXTURE_2D, texName_);
			glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignForAddrAndPitch(nullptr, pix.pitchBytes()));
			if(pbo)
			{
				assumeExpr(r->support.hasUnpackRowLength);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				r->support.glFlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER, (GLintptr)bufferOffset, pix.bytes());
			}
			else
			{
				if(r->support.hasUnpackRowLength)
					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			}
			GLenum format = makeGLFormat(*r, pix.format());
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
				logMsg("generating mipmaps for texture:0x%X", texName_);
				r->support.generateMipmaps(GL_TEXTURE_2D);
			}
		});
}

IG::WP Texture::size(uint16_t level) const
{
	assert(levels_);
	uint32_t w = pixDesc.w(), h = pixDesc.h();
	iterateTimes(level, i)
	{
		w = std::max(1u, (w / 2));
		h = std::max(1u, (h / 2));
	}
	return {(int)w, (int)h};
}

IG::PixmapDesc Texture::pixmapDesc() const
{
	return pixDesc;
}

bool Texture::compileDefaultProgram(uint32_t mode) const
{
	assumeExpr(r);
	switch(mode)
	{
		bcase IMG_MODE_REPLACE:
			switch(type_)
			{
				case TEX_2D_1 : return r->texAlphaReplaceProgram.compile(*r);
				case TEX_2D_2 : return r->texReplaceProgram.compile(*r);
				case TEX_2D_4 : return r->texReplaceProgram.compile(*r);
				#ifdef __ANDROID__
				case TEX_2D_EXTERNAL : return r->texExternalReplaceProgram.compile(*r);
				#endif
				default: bug_unreachable("type == %d", type_); return false;
			}
		bcase IMG_MODE_MODULATE:
			switch(type_)
			{
				case TEX_2D_1 : return r->texAlphaProgram.compile(*r);
				case TEX_2D_2 : return r->texProgram.compile(*r);
				case TEX_2D_4 : return r->texProgram.compile(*r);
				#ifdef __ANDROID__
				case TEX_2D_EXTERNAL : return r->texExternalProgram.compile(*r);
				#endif
				default: bug_unreachable("type == %d", type_); return false;
			}
		bdefault: bug_unreachable("type == %d", type_); return false;
	}
}

bool Texture::compileDefaultProgramOneShot(uint32_t mode) const
{
	assumeExpr(r);
	auto compiled = compileDefaultProgram(mode);
	if(compiled)
		r->autoReleaseShaderCompiler();
	return compiled;
}

void Texture::useDefaultProgram(RendererCommands &cmds, uint32_t mode, const Mat4 *modelMat) const
{
	#ifndef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	const uint32_t type_ = TEX_2D_4;
	#endif
	switch(mode)
	{
		bcase IMG_MODE_REPLACE:
			switch(type_)
			{
				bcase TEX_2D_1 : r->texAlphaReplaceProgram.use(cmds, modelMat);
				bcase TEX_2D_2 : r->texReplaceProgram.use(cmds, modelMat);
				bcase TEX_2D_4 : r->texReplaceProgram.use(cmds, modelMat);
				#ifdef __ANDROID__
				bcase TEX_2D_EXTERNAL : r->texExternalReplaceProgram.use(cmds, modelMat);
				#endif
			}
		bcase IMG_MODE_MODULATE:
			switch(type_)
			{
				bcase TEX_2D_1 : r->texAlphaProgram.use(cmds, modelMat);
				bcase TEX_2D_2 : r->texProgram.use(cmds, modelMat);
				bcase TEX_2D_4 : r->texProgram.use(cmds, modelMat);
				#ifdef __ANDROID__
				bcase TEX_2D_EXTERNAL : r->texExternalProgram.use(cmds, modelMat);
				#endif
			}
	}
}

Texture::operator bool() const
{
	return texName();
}

Renderer &Texture::renderer()
{
	assumeExpr(r);
	return *r;
}

Texture::operator TextureSpan() const
{
	return {this};
}

GLuint GLTexture::texName() const
{
	return texName_;
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

void GLTexture::setSwizzleForFormat(Renderer &r, PixelFormatID format, GLuint tex, GLenum target)
{
	assert(tex);
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(r.support.useFixedFunctionPipeline)
		return;
	if(r.support.hasTextureSwizzle && target == GL_TEXTURE_2D)
	{
		r.runGLTask(
			[=]()
			{
				verifyCurrentTexture2D(tex);
				const GLint swizzleMaskRGBA[] {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
				const GLint swizzleMaskIA88[] {GL_RED, GL_RED, GL_RED, GL_GREEN};
				const GLint swizzleMaskA8[] {GL_ONE, GL_ONE, GL_ONE, GL_RED};
				#ifdef CONFIG_GFX_OPENGL_ES
				auto &swizzleMask = (format == PIXEL_IA88) ? swizzleMaskIA88
						: (format == PIXEL_A8) ? swizzleMaskA8
						: swizzleMaskRGBA;
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swizzleMask[0]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swizzleMask[1]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swizzleMask[2]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swizzleMask[3]);
				#else
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (format == PIXEL_IA88) ? swizzleMaskIA88
						: (format == PIXEL_A8) ? swizzleMaskA8
						: swizzleMaskRGBA);
				#endif
			});
	}
	#endif
}

void GLTexture::updateFormatInfo(IG::PixmapDesc desc, uint16_t levels, GLenum target)
{
	assert(levels);
	levels_ = levels;
	pixDesc = desc;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(Config::Gfx::OPENGL_ES && target == GL_TEXTURE_EXTERNAL_OES)
		type_ = TEX_2D_EXTERNAL;
	else
		type_ = typeForPixelFormat(desc.format());
	#endif
	setSwizzleForFormat(*r, desc.format(), texName_, target);
}

void GLTexture::updateLevelsForMipmapGeneration()
{
	if(!r->support.hasImmutableTexStorage)
	{
		// all possible levels generated by glGenerateMipmap
		levels_ = fls(pixDesc.w() | pixDesc.h());
	}
}

bool GLTexture::isExternal() const
{
	return target() == GL_TEXTURE_EXTERNAL_OES;
}

PixmapTexture::PixmapTexture(Renderer &r, TextureConfig config, IG::ErrorCode *errorPtr)
{
	this->r = &r;
	IG::ErrorCode err = GLPixmapTexture::init(*this, config);
	if(unlikely(err && errorPtr))
	{
		*errorPtr = err;
	}
}

PixmapTexture::PixmapTexture(Renderer &r, GfxImageSource &img, bool makeMipmaps, IG::ErrorCode *errorPtr)
{
	this->r = &r;
	IG::ErrorCode err;
	auto setError = IG::scopeGuard([&](){ if(unlikely(err && errorPtr)) { *errorPtr = err; } });
	if(img)
	{
		if(err = GLPixmapTexture::init(*this, configWithLoadedImagePixmap(img.pixmapView(), makeMipmaps));
			unlikely(err))
		{
			return;
		}
		err = loadImageSource(*this, img, makeMipmaps);
	}
	else
	{
		err = GLPixmapTexture::init(*this, {{{1, 1}, Base::PIXEL_FMT_A8}});
	}
}

IG::ErrorCode GLPixmapTexture::init(PixmapTexture &self, TextureConfig config)
{
	auto &r = self.renderer();
	config = self.baseInit(r, config);
	if(auto err = self.setFormat(config.pixmapDesc(), config.levels());
		unlikely(err))
	{
		return err;
	}
	return {};
}

IG::ErrorCode PixmapTexture::setFormat(IG::PixmapDesc desc, uint32_t levels)
{
	assumeExpr(r);
	IG::PixmapDesc fullPixDesc = r->support.textureSizeSupport.makePixmapDescWithSupportedSize(desc);
	if(auto err = Texture::setFormat(fullPixDesc, levels);
		unlikely(err))
	{
		return err;
	}
	if(desc != fullPixDesc)
		clear(0);
	updateUsedPixmapSize(desc, pixmapDesc());
	return {};
}

IG::Rect2<GTexC> PixmapTexture::uvBounds() const
{
	return {0, 0, uv.x, uv.y};
}

IG::PixmapDesc PixmapTexture::usedPixmapDesc() const
{
	return {usedSize, pixmapDesc().format()};
}

PixmapTexture::operator TextureSpan() const
{
	return {this, uvBounds()};
}

void GLPixmapTexture::updateUsedPixmapSize(IG::PixmapDesc usedDesc, IG::PixmapDesc fullDesc)
{
	usedSize = usedDesc.size();
	uv.x = pixelToTexC((uint32_t)(usedDesc.size().x), fullDesc.w());
	uv.y = pixelToTexC((uint32_t)(usedDesc.size().y), fullDesc.h());
}

IG::PixmapDesc TextureSizeSupport::makePixmapDescWithSupportedSize(IG::PixmapDesc desc) const
{
	return {makeSupportedSize(desc.size()), desc.format()};
}

IG::WP TextureSizeSupport::makeSupportedSize(IG::WP size) const
{
	using namespace IG;
	IG::WP supportedSize;
	if(nonPow2 && !forcePow2)
	{
		supportedSize = size;
	}
	else if(nonSquare)
	{
		supportedSize = {(int)roundUpPowOf2((uint32_t)size.x), (int)roundUpPowOf2((uint32_t)size.y)};
	}
	else
	{
		supportedSize.x = supportedSize.y = roundUpPowOf2((uint32_t)std::max(size.x, size.y));
	}
	if(Config::MACHINE_IS_PANDORA && (supportedSize.x <= 16 || supportedSize.y <= 16))
	{
		// force small textures as square due to PowerVR driver bug
		supportedSize.x = supportedSize.y = std::max(supportedSize.x, supportedSize.y);
	}
	return supportedSize;
}

bool TextureSizeSupport::supportsMipmaps(uint32_t imageX, uint32_t imageY) const
{
	return imageX && imageY &&
		(nonPow2CanMipmap || (IG::isPowerOf2(imageX) && IG::isPowerOf2(imageY)));
}

}

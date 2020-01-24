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
#include <algorithm>
#include <imagine/gfx/Gfx.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math/int.hh>
#include <imagine/mem/mem.h>
#include "private.hh"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#include "android/GraphicBufferStorage.hh"
#include "android/SurfaceTextureStorage.hh"
#endif

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

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif

#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif

#ifndef GL_MAP_UNSYNCHRONIZED_BIT
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#endif

namespace Gfx
{

Texture::Texture(Texture &&o)
{
	*this = o;
	o.texName_ = 0;
}

Texture &Texture::operator=(Texture &&o)
{
	deinit();
	*this = o;
	o.texName_ = 0;
	return *this;
}

#ifdef __ANDROID__
// set to actual implementation when androidStorageImpl() or setAndroidStorageImpl() is called
GLTexture::AndroidStorageImpl GLTexture::androidStorageImpl_ = GLTexture::ANDROID_AUTO;
#endif

static uint32_t makeUnpackAlignment(uintptr_t addr)
{
	// find best alignment with lower 3 bits
	constexpr uint32_t map[]
	{
		8, 1, 2, 1, 4, 1, 2, 1
	};
	return map[addr & 7];
}

static uint32_t unpackAlignForAddrAndPitch(void *srcAddr, uint32_t pitch)
{
	uint32_t alignmentForAddr = makeUnpackAlignment((uintptr_t)srcAddr);
	uint32_t alignmentForPitch = makeUnpackAlignment(pitch);
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

static void setTexParameteriImpl(GLenum target, GLenum pname, GLint param, const char *pnameStr)
{
	runGLCheckedVerbose(
		[&]()
		{
			glTexParameteri(target, pname, param);
		}, "glTexParameteri()");
}

#define setTexParameteri(target, pname, param) setTexParameteriImpl(target, pname, param, #pname);

static void setSamplerParameteriImpl(Renderer &r, GLuint sampler, GLenum pname, GLint param, const char *pnameStr)
{
	runGLCheckedVerbose(
		[&]()
		{
			r.support.glSamplerParameteri(sampler, pname, param);
		}, "glSamplerParameteri()");
}

#define setSamplerParameteri(r, sampler, pname, param) setSamplerParameteriImpl(r, sampler, pname, param, #pname);

static GLint makeMinFilter(bool linearFiltering, MipFilterMode mipFiltering)
{
	switch(mipFiltering)
	{
		case MIP_FILTER_NONE: return linearFiltering ? GL_LINEAR : GL_NEAREST;
		case MIP_FILTER_NEAREST: return linearFiltering ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
		case MIP_FILTER_LINEAR: return linearFiltering ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
		default: bug_unreachable("mipFiltering == %d", (int)mipFiltering); return 0;
	}
}

static GLint makeMagFilter(bool linearFiltering)
{
	return linearFiltering ? GL_LINEAR : GL_NEAREST;
}

static GLint makeWrapMode(WrapMode mode)
{
	return mode == WRAP_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT;
}

static TextureConfig configWithLoadedImagePixmap(IG::PixmapDesc desc, bool makeMipmaps)
{
	TextureConfig config{desc};
	config.setWillGenerateMipmaps(makeMipmaps);
	return config;
}

static LockedTextureBuffer makeLockedTextureBuffer(IG::Pixmap pix, IG::WindowRect srcDirtyBounds, uint32_t lockedLevel, GLuint pbo = 0)
{
	LockedTextureBuffer lockBuff;
	lockBuff.set(pix, srcDirtyBounds, lockedLevel, pbo);
	return lockBuff;
}

static bool supportsDirectStorage(Renderer &r)
{
	#ifdef __ANDROID__
	if(Texture::androidStorageImpl(r) != Texture::ANDROID_NONE)
		return true;
	#endif
	return false;
}

template<class T>
static Error initTextureCommon(Renderer &r, T &texture, GfxImageSource &img, bool makeMipmaps)
{
	auto imgPix = img.lockPixmap();
	auto unlockImgPixmap = IG::scopeGuard([&](){ img.unlockPixmap(); });
	if(auto err = texture.init2(r, configWithLoadedImagePixmap(imgPix, makeMipmaps));
		err)
	{
		return err;
	}
	auto lockBuff = texture.lock(0);
	if(lockBuff)
	{
		if(imgPix)
		{
			//logDMsg("copying locked image pixmap to locked texture");
			lockBuff.pixmap().write(imgPix, {});
		}
		else
		{
			//logDMsg("writing image to locked texture");
			img.write(lockBuff.pixmap());
		}
		texture.unlock(lockBuff);
	}
	else
	{
		if(imgPix)
		{
			//logDMsg("writing locked image pixmap to texture");
			texture.write(0, imgPix, {});
		}
		else
		{
			//logDMsg("writing image to texture");
			IG::MemPixmap texPix{imgPix};
			if(!texPix)
				return std::runtime_error{"Out of memory"};
			img.write(texPix);
			texture.write(0, texPix, {});
		}
	}
	unlockImgPixmap();
	if(makeMipmaps)
		texture.generateMipmaps();
	return {};
}

void TextureSampler::init2(Renderer &r, TextureSamplerConfig config)
{
	deinit(r);
	magFilter = makeMagFilter(config.minLinearFilter());
	minFilter = makeMinFilter(config.magLinearFilter(), config.mipFilter());
	xWrapMode_ = makeWrapMode(config.xWrapMode());
	yWrapMode_ = makeWrapMode(config.yWrapMode());
	if(r.support.hasSamplerObjects)
	{
		r.runGLTaskSync(
			[this, &r]()
			{
				r.support.glGenSamplers(1, &name_);
				if(magFilter != GL_LINEAR) // GL_LINEAR is the default
					setSamplerParameteri(r, name_, GL_TEXTURE_MAG_FILTER, magFilter);
				if(minFilter != GL_NEAREST_MIPMAP_LINEAR) // GL_NEAREST_MIPMAP_LINEAR is the default
					setSamplerParameteri(r, name_, GL_TEXTURE_MIN_FILTER, minFilter);
				if(xWrapMode_ != GL_REPEAT) // GL_REPEAT is the default
					setSamplerParameteri(r, name_, GL_TEXTURE_WRAP_S, xWrapMode_);
				if(yWrapMode_ != GL_REPEAT) // GL_REPEAT​​ is the default
					setSamplerParameteri(r, name_, GL_TEXTURE_WRAP_T, yWrapMode_);
			});
	}
	else
	{
		r.samplerNames++;
		name_ = r.samplerNames;
	}
	logMsg("created sampler:0x%X", name_);
}

void TextureSampler::deinit(Renderer &r)
{
	if(r.support.hasSamplerObjects && name_)
	{
		r.runGLTaskSync(
			[this, &r]()
			{
				r.support.glDeleteSamplers(1, &name_);
			});
	}
	*this = {};
}

TextureSampler::operator bool() const
{
	return name_;
}

void TextureSampler::initDefaultClampSampler(Renderer &r)
{
	if(!r.defaultClampSampler)
	{
		r.defaultClampSampler = r.makeTextureSampler({});
	}
}

void TextureSampler::initDefaultNearestMipClampSampler(Renderer &r)
{
	if(!r.defaultNearestMipClampSampler)
	{
		TextureSamplerConfig conf;
		conf.setMipFilter(MIP_FILTER_NEAREST);
		r.defaultNearestMipClampSampler = r.makeTextureSampler(conf);
	}
}

void TextureSampler::initDefaultNoMipClampSampler(Renderer &r)
{
	if(!r.defaultNoMipClampSampler)
	{
		TextureSamplerConfig conf;
		conf.setMipFilter(MIP_FILTER_NONE);
		r.defaultNoMipClampSampler = r.makeTextureSampler(conf);
	}
}

void TextureSampler::initDefaultNoLinearNoMipClampSampler(Renderer &r)
{
	if(!r.defaultNoLinearNoMipClampSampler)
	{
		TextureSamplerConfig conf;
		conf.setLinearFilter(false);
		conf.setMipFilter(MIP_FILTER_NONE);
		r.defaultNoLinearNoMipClampSampler = r.makeTextureSampler(conf);
	}
}

void TextureSampler::initDefaultRepeatSampler(Renderer &r)
{
	if(!r.defaultRepeatSampler)
	{
		TextureSamplerConfig conf;
		conf.setWrapMode(WRAP_REPEAT);
		r.defaultRepeatSampler = r.makeTextureSampler(conf);
	}
}

void TextureSampler::initDefaultNearestMipRepeatSampler(Renderer &r)
{
	if(!r.defaultNearestMipRepeatSampler)
	{
		TextureSamplerConfig conf;
		conf.setMipFilter(MIP_FILTER_NEAREST);
		conf.setWrapMode(WRAP_REPEAT);
		r.defaultNearestMipRepeatSampler = r.makeTextureSampler(conf);
	}
}

void TextureSampler::bindDefaultClampSampler(RendererCommands &cmds)
{
	assert(cmds.renderer().defaultClampSampler);
	cmds.setTextureSampler(cmds.renderer().defaultClampSampler);
}

void TextureSampler::bindDefaultNearestMipClampSampler(RendererCommands &cmds)
{
	assert(cmds.renderer().defaultNearestMipClampSampler);
	cmds.setTextureSampler(cmds.renderer().defaultNearestMipClampSampler);
}

void TextureSampler::bindDefaultNoMipClampSampler(RendererCommands &cmds)
{
	assert(cmds.renderer().defaultNoMipClampSampler);
	cmds.setTextureSampler(cmds.renderer().defaultNoMipClampSampler);
}

void TextureSampler::bindDefaultNoLinearNoMipClampSampler(RendererCommands &cmds)
{
	assert(cmds.renderer().defaultNoLinearNoMipClampSampler);
	cmds.setTextureSampler(cmds.renderer().defaultNoLinearNoMipClampSampler);
}

void TextureSampler::bindDefaultRepeatSampler(RendererCommands &cmds)
{
	assert(cmds.renderer().defaultRepeatSampler);
	cmds.setTextureSampler(cmds.renderer().defaultRepeatSampler);
}

void TextureSampler::bindDefaultNearestMipRepeatSampler(RendererCommands &cmds)
{
	assert(cmds.renderer().defaultNearestMipRepeatSampler);
	cmds.setTextureSampler(cmds.renderer().defaultNearestMipRepeatSampler);
}

void GLTextureSampler::setTexParams(Renderer &r, GLenum target)
{
	assert(!r.support.hasSamplerObjects);
	setTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
	setTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
	setTexParameteri(target, GL_TEXTURE_WRAP_S, xWrapMode_);
	setTexParameteri(target, GL_TEXTURE_WRAP_T, yWrapMode_);
}

DirectTextureStorage::~DirectTextureStorage() {}

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

void GLLockedTextureBuffer::set(IG::Pixmap pix, IG::WindowRect srcDirtyRect, uint32_t lockedLevel, GLuint pbo)
{
	this->pix = pix;
	this->srcDirtyRect = srcDirtyRect;
	this->lockedLevel = lockedLevel;
	this->pbo_ = pbo;
}

Error Texture::init2(Renderer &r, TextureConfig config)
{
	deinit();
	this->r = &r;
	r.runGLTaskSync(
		[this]()
		{
			texName_ = makeGLTexture();
		});
	if(config.willWriteOften())
	{
		#ifdef __ANDROID__
		if(config.levels() == 1)
		{
			if(androidStorageImpl_ == ANDROID_AUTO)
			{
				setAndroidStorageImpl(r, ANDROID_AUTO);
			}
			if(androidStorageImpl_ == ANDROID_SURFACE_TEXTURE)
			{
				Error err{};
				auto *surfaceTex = new SurfaceTextureStorage(r, texName_, false, err);
				if(err)
				{
					logWarn("SurfaceTexture error with texture:0x%X (%s)", texName_, err->what());
					delete surfaceTex;
				}
				else
				{
					target = GL_TEXTURE_EXTERNAL_OES;
					directTex = surfaceTex;
				}
			}
			else if(androidStorageImpl_ == ANDROID_GRAPHIC_BUFFER)
			{
				auto *gbTex = new GraphicBufferStorage();
				directTex = gbTex;
			}
		}
		#endif
	}
	if(config.willGenerateMipmaps() && !r.support.hasImmutableTexStorage)
	{
		// when using glGenerateMipmaps exclusively, there is no need to define
		// all texture levels with glTexImage2D beforehand
		config.setLevels(1);
	}
	return setFormat(config.pixmapDesc(), config.levels());
}

Error Texture::init2(Renderer &r, GfxImageSource &img, bool makeMipmaps)
{
	return initTextureCommon(r, *this, img, makeMipmaps);
}

void Texture::deinit()
{
	if(!texName_)
		return;
	logMsg("deinit texture:0x%X", texName_);
	assumeExpr(r);
	r->runGLTaskSync(
		[this]()
		{
			glDeleteTextures(1, &texName_);
			texName_ = 0;
		});
	delete directTex;
}

uint32_t Texture::bestAlignment(const IG::Pixmap &p)
{
	return unpackAlignForAddrAndPitch(p.pixel({}), p.pitchBytes());
}

bool GLTexture::canUseMipmaps(Renderer &r) const
{
	return !directTex && r.support.textureSizeSupport.supportsMipmaps(pixDesc.w(), pixDesc.h());
}

bool Texture::canUseMipmaps() const
{
	assumeExpr(r);
	return GLTexture::canUseMipmaps(*r);
}

bool Texture::generateMipmaps()
{
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
	if(!r->support.hasImmutableTexStorage)
	{
		// all possible levels generated by glGenerateMipmap
		levels_ = fls(pixDesc.w() | pixDesc.h());
	}
	return true;
}

uint32_t Texture::levels() const
{
	return levels_;
}

Error Texture::setFormat(IG::PixmapDesc desc, uint32_t levels)
{
	if(unlikely(!texName_))
		return std::runtime_error("texture not initialized");
	assumeExpr(r);
	Error err{};
	if(directTex)
	{
		levels = 1;
		if(pixDesc == desc)
			logWarn("resizing with same dimensions %dx%d, should optimize caller code", desc.w(), desc.h());
		err = directTex->setFormat(*r, desc, texName_);
	}
	else
	{
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
					if(levels_) // texture format was previously set
					{
						sampler = 0;
						glDeleteTextures(1, &texName_);
						texName_ = makeGLTexture();
					}
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
					if(unlikely(levels_ && levels != levels_)) // texture format was previously set
					{
						sampler = 0;
						glDeleteTextures(1, &texName_);
						texName_ = makeGLTexture();
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
	}
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
	return err;
}

void GLTexture::bindTex(RendererCommands &cmds, TextureSampler &bindSampler)
{
	if(!texName_)
	{
		logErr("tried to bind uninitialized texture");
		return;
	}
	cmds.glcBindTexture(target, texName_);
	if(!cmds.renderer().support.hasSamplerObjects && bindSampler.name() != sampler)
	{
		logMsg("setting sampler:0x%X for texture:0x%X", (int)bindSampler.name(), texName_);
		sampler = bindSampler.name();
		bindSampler.setTexParams(cmds.renderer(), target);
	}
}

void Texture::writeAligned(uint32_t level, const IG::Pixmap &pixmap, IG::WP destPos, uint32_t assumeAlign, uint32_t commitFlags)
{
	//logDMsg("writing pixmap %dx%d to pos %dx%d", pixmap.x, pixmap.y, destPos.x, destPos.y);
	if(unlikely(!texName_))
	{
		logErr("can't write to uninitialized texture");
		return;
	}
	assumeExpr(r);
	assumeExpr(destPos.x + pixmap.w() <= (uint32_t)size(level).x);
	assumeExpr(destPos.y + pixmap.h() <= (uint32_t)size(level).y);
	assumeExpr(pixmap.format() == pixDesc.format());
	r->resourceUpdate = true;
	if(!assumeAlign)
		assumeAlign = unpackAlignForAddrAndPitch(pixmap.pixel({}), pixmap.pitchBytes());
	if(directTex)
	{
		assert(level == 0);
		if(destPos != IG::WP{0, 0} || pixmap.w() != (uint32_t)size(0).x || pixmap.h() != (uint32_t)size(0).y)
		{
			logErr("partial write of direct texture unsupported, use lock()");
			return;
		}
		auto lockBuff = lock(0);
		if(!lockBuff)
		{
			return;
		}
		lockBuff.pixmap().write(pixmap, {});
		unlock(lockBuff);
	}
	else
	{
		if((uintptr_t)pixmap.pixel({}) % (uintptr_t)assumeAlign != 0)
		{
			bug_unreachable("expected data from address %p to be aligned to %u bytes", pixmap.pixel({}), assumeAlign);
		}
		GLenum format = makeGLFormat(*r, pixmap.format());
		GLenum dataType = makeGLDataType(pixmap.format());
		auto hasUnpackRowLength = r->support.hasUnpackRowLength;
		if(hasUnpackRowLength || !pixmap.isPadded())
		{
			r->runGLTaskSyncConditional(
				[texName_ = this->texName_, hasUnpackRowLength, level, pixmap, destPos, assumeAlign, format, dataType]()
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
				}, !(commitFlags & COMMIT_FLAG_ASYNC));
		}
		else
		{
			// must copy to temp pixmap without extra pitch pixels
			static uint32_t prevPixmapX = 0, prevPixmapY = 0;
			if(pixmap.w() != prevPixmapX || pixmap.h() != prevPixmapY) // don't spam log with repeated calls of same size pixmap
			{
				prevPixmapX = pixmap.w();
				prevPixmapY = pixmap.h();
				logDMsg("non-optimal texture write operation with %ux%u pixmap", pixmap.w(), pixmap.h());
			}
			void *tempPixData;
			if(posix_memalign(&tempPixData, (size_t)__BIGGEST_ALIGNMENT__, pixmap.pixelBytes()))
			{
				logErr("posix_memalign failed allocating %zu bytes", pixmap.pixelBytes());
				return;
			}
			IG::Pixmap tempPix{pixmap, tempPixData};
			tempPix.write(pixmap, {});
			r->runGLTask(
				[texName_ = this->texName_, level, tempPix, destPos, assumeAlign, format, dataType]()
				{
					glBindTexture(GL_TEXTURE_2D, texName_);
					glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignForAddrAndPitch(nullptr, tempPix.pitchBytes()));
					runGLCheckedVerbose(
						[&]()
						{
							glTexSubImage2D(GL_TEXTURE_2D, level, destPos.x, destPos.y,
								tempPix.w(), tempPix.h(), format, dataType, tempPix.pixel({}));
						}, "glTexSubImage2D()");
					free(tempPix.pixel({}));
				});
		}
	}
}

void Texture::write(uint32_t level, const IG::Pixmap &pixmap, IG::WP destPos, uint32_t commitFlags)
{
	writeAligned(level, pixmap, destPos, bestAlignment(pixmap), commitFlags);
}

void Texture::clear(uint32_t level)
{
	auto lockBuff = lock(level);
	if(lockBuff)
	{
		lockBuff.pixmap().clear();
		unlock(lockBuff);
	}
	else
	{
		IG::PixmapDesc levelPixDesc{size(level), pixDesc.format()};
		void *tempPixData = mem_calloc(levelPixDesc.pixelBytes());
		IG::Pixmap blankPix{levelPixDesc, tempPixData};
		writeAligned(level, blankPix, {}, unpackAlignForAddrAndPitch(nullptr, blankPix.pitchBytes()));
		mem_free(tempPixData);
	}
}

LockedTextureBuffer Texture::lock(uint32_t level)
{
	assumeExpr(r);
	if(directTex)
	{
		assert(level == 0);
		auto buff = directTex->lock(*r, nullptr);
		IG::Pixmap pix{pixDesc, buff.data, {buff.pitch, IG::Pixmap::BYTE_UNITS}};
		return makeLockedTextureBuffer(pix, {}, 0);
	}
	else if(r->support.hasPBOFuncs)
	{
		return lock(level, {0, 0, size(level).x, size(level).y});
	}
	else
		return {}; // lock() not supported
}

LockedTextureBuffer Texture::lock(uint32_t level, IG::WindowRect rect)
{
	assumeExpr(r);
	assert(rect.x2  <= size(level).x);
	assert(rect.y2 <= size(level).y);
	if(directTex)
	{
		assert(level == 0);
		auto buff = directTex->lock(*r, &rect);
		IG::Pixmap pix{pixDesc, buff.data, {buff.pitch, IG::Pixmap::BYTE_UNITS}};
		return makeLockedTextureBuffer(pix, rect, 0);
	}
	else if(r->support.hasPBOFuncs)
	{
		void *data;
		GLuint pbo = 0;
		r->runGLTaskSync(
			[this, &data, &pbo, rect]()
			{
				uint32_t rangeBytes = pixDesc.format().pixelBytes(rect.xSize() * rect.ySize());
				glGenBuffers(1, &pbo);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				glBufferData(GL_PIXEL_UNPACK_BUFFER, rangeBytes, nullptr, GL_STREAM_DRAW);
				data = r->support.glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, rangeBytes,
					GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				//logDMsg("mapped PBO at addr:%p", data);
			});
		if(!data)
		{
			logErr("error mapping buffer");
			return {};
		}
		IG::Pixmap pix{{rect.size(), pixDesc.format()}, data};
		return makeLockedTextureBuffer(pix, rect, level, pbo);
	}
	else
		return {}; // lock() not supported
}

void Texture::unlock(LockedTextureBuffer lockBuff)
{
	assumeExpr(r);
	r->resourceUpdate = true;
	if(directTex)
		directTex->unlock(*r, texName_);
	else if(r->support.hasPBOFuncs)
	{
		r->runGLTask(
			[r = this->r, texName_ = this->texName_, pix = lockBuff.pixmap(),
			 destPos = IG::WP{lockBuff.sourceDirtyRect().x, lockBuff.sourceDirtyRect().y},
			 level = lockBuff.level(), pbo = lockBuff.pbo()]()
			{
				//logDMsg("unmapped PBO");
				r->support.glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
				glBindTexture(GL_TEXTURE_2D, texName_);
				glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignForAddrAndPitch(nullptr, pix.pitchBytes()));
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				GLenum format = makeGLFormat(*r, pix.format());
				GLenum dataType = makeGLDataType(pix.format());
				runGLCheckedVerbose(
					[&]()
					{
						glTexSubImage2D(GL_TEXTURE_2D, level, destPos.x, destPos.y,
							pix.w(), pix.h(), format, dataType, nullptr);
					}, "glTexSubImage2D()");
				//logDMsg("deleting temporary PBO:%u", pbo);
				glDeleteBuffers(1, &pbo);
			});
	}
}

IG::WP Texture::size(uint32_t level) const
{
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

bool Texture::compileDefaultProgram(uint32_t mode)
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

bool Texture::compileDefaultProgramOneShot(uint32_t mode)
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

Error PixmapTexture::init2(Renderer &r, TextureConfig config)
{
	if(config.willWriteOften() && config.levels() == 1 && supportsDirectStorage(r))
	{
		// try without changing size when using direct pixel storage
		if(auto err = Texture::init2(r, config);
			!err)
		{
			usedSize = config.pixmapDesc().size();
			return {};
		}
	}
	auto origPixDesc = config.pixmapDesc();
	config.setPixmapDesc(r.support.textureSizeSupport.makePixmapDescWithSupportedSize(origPixDesc));
	if(auto err = Texture::init2(r, config);
		err)
	{
		return err;
	}
	if(origPixDesc != config.pixmapDesc())
		clear(0);
	usedSize = origPixDesc.size();
	updateUV({}, origPixDesc.size());
	return {};
}

Error PixmapTexture::init2(Renderer &r, GfxImageSource &img, bool makeMipmaps)
{
	if(img)
		return initTextureCommon(r, *this, img, makeMipmaps);
	else
		return init2(r, {{{1, 1}, Base::PIXEL_FMT_A8}});
}

Error PixmapTexture::setFormat(IG::PixmapDesc desc, uint32_t levels)
{
	assumeExpr(r);
	if(directTex)
	{
		usedSize = desc.size();
		return Texture::setFormat(desc, levels);
	}
	else
	{
		IG::PixmapDesc fullPixDesc = r->support.textureSizeSupport.makePixmapDescWithSupportedSize(desc);
		auto result = Texture::setFormat(fullPixDesc, levels);
		if(result)
		{
			return result;
		}
		if(desc != fullPixDesc)
			clear(0);
		usedSize = desc.size();
		updateUV({}, desc.size());
		return {};
	}
}

IG::Rect2<GTexC> PixmapTexture::uvBounds() const
{
	return uv;
}

IG::PixmapDesc PixmapTexture::usedPixmapDesc() const
{
	return {usedSize, pixmapDesc().format()};
}

void PixmapTexture::updateUV(IG::WP pixPos, IG::WP pixSize)
{
	uv.x = pixelToTexC((uint32_t)pixPos.x, pixDesc.w());
	uv.y = pixelToTexC((uint32_t)pixPos.y, pixDesc.h());
	uv.x2 = pixelToTexC((uint32_t)(pixPos.x + pixSize.x), pixDesc.w());
	uv.y2 = pixelToTexC((uint32_t)(pixPos.y + pixSize.y), pixDesc.h());
}

GLuint GLTexture::texName() const
{
	return texName_;
}

void GLTexture::setSwizzleForFormat(Renderer &r, PixelFormatID format, GLuint tex, GLenum target)
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(r.support.useFixedFunctionPipeline)
		return;
	if(r.support.hasTextureSwizzle && target == GL_TEXTURE_2D)
	{
		r.runGLTask(
			[&r, format, tex]()
			{
				r.verifyCurrentTexture2D(tex);
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

#ifdef __ANDROID__
GLTexture::AndroidStorageImpl GLTexture::androidStorageImpl(Renderer &r)
{
	if(unlikely(androidStorageImpl_ == ANDROID_AUTO))
	{
		setAndroidStorageImpl(r, ANDROID_AUTO);
	}
	return androidStorageImpl_;
}

static const char *rendererGLStr(Renderer &r)
{
	const char *str;
	r.runGLTaskSync(
		[&str]()
		{
			str = (const char*)glGetString(GL_RENDERER);
		});
	return str;
}

bool GLTexture::setAndroidStorageImpl(Renderer &r, AndroidStorageImpl impl)
{
	switch(impl)
	{
		bcase ANDROID_AUTO:
		{
			logMsg("auto-detecting Android storage implementation");
			androidStorageImpl_ = ANDROID_NONE;
			if(GraphicBufferStorage::isRendererWhitelisted(rendererGLStr(r)))
			{
				// use GraphicBuffer if whitelisted
				androidStorageImpl_ = ANDROID_GRAPHIC_BUFFER;
				logMsg("using Android GraphicBuffers as texture storage (white-listed)");
			}
			else
			{
				// try SurfaceTexture if supported
				setAndroidStorageImpl(r, ANDROID_SURFACE_TEXTURE);
			}
			if(androidStorageImpl_ == ANDROID_NONE)
			{
				logMsg("not using Android-specific texture storage");
			}
			return true;
		}
		bcase ANDROID_NONE:
		{
			androidStorageImpl_ = ANDROID_NONE;
			logMsg("not using Android-specific texture storage");
			return true;
		}
		bcase ANDROID_GRAPHIC_BUFFER:
		{
			auto rendererStr = rendererGLStr(r);
			if(Config::ARM_ARCH == 7 && r.support.useLegacyGLSL && strstr(rendererStr, "Tegra"))
			{
				logMsg("not using GraphicBuffer due to app lockup on Tegra 4 and older");
				return false;
			}
			if(GraphicBufferStorage::testPassed)
			{
				androidStorageImpl_ = ANDROID_GRAPHIC_BUFFER;
				logMsg("using Android GraphicBuffer as texture storage (skipping test)");
				return true;
			}
			else
			{
				// test before enabling
				if(!r.support.hasEGLImages)
				{
					logErr("Can't use GraphicBuffer without OES_EGL_image extension");
					return false;
				}
				Base::GraphicBuffer gb{};
				if(!gb.hasBufferMapper())
				{
					logErr("failed GraphicBuffer mapper initialization");
					return false;
				}
				if(!gb.reallocate(256, 256, HAL_PIXEL_FORMAT_RGB_565, GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE))
				{
					logErr("failed GraphicBuffer allocation test");
					return false;
				}
				void *addr;
				if(!gb.lock(GRALLOC_USAGE_SW_WRITE_OFTEN, &addr))
				{
					logErr("failed GraphicBuffer lock test");
					return false;
				}
				gb.unlock();
				androidStorageImpl_ = ANDROID_GRAPHIC_BUFFER;
				GraphicBufferStorage::testPassed = true;
				logMsg("using Android GraphicBuffer as texture storage");
				return true;
			}
		}
		bcase ANDROID_SURFACE_TEXTURE:
		{
			if(Base::androidSDK() < 14)
				return false;
			if(!r.support.hasExternalEGLImages)
			{
				logErr("can't use SurfaceTexture without OES_EGL_image_external");
				return false;
			}
			// check if external textures work in GLSL
			if(r.texExternalReplaceProgram.compile(r))
				r.autoReleaseShaderCompiler();
			if(!r.texExternalReplaceProgram)
			{
				logErr("can't use SurfaceTexture due to test shader compilation error");
				return false;
			}
			androidStorageImpl_ = ANDROID_SURFACE_TEXTURE;
			logMsg("using Android SurfaceTexture as texture storage");
			return true;
		}
		bdefault:
			bug_unreachable("impl == %d", (int)impl);
			return false;
	}
}

bool GLTexture::isAndroidGraphicBufferStorageWhitelisted(Renderer &r)
{
	return GraphicBufferStorage::isRendererWhitelisted(rendererGLStr(r));
}

bool GLTexture::isExternal()
{
	return target == GL_TEXTURE_EXTERNAL_OES;
}

const char *GLTexture::androidStorageImplStr(AndroidStorageImpl impl)
{
	switch(impl)
	{
		case ANDROID_AUTO: return "Auto";
		case ANDROID_NONE: return "Standard";
		case ANDROID_GRAPHIC_BUFFER: return "Graphic Buffer";
		case ANDROID_SURFACE_TEXTURE: return "Surface Texture";
	}
	return "Unknown";
}

const char *GLTexture::androidStorageImplStr(Renderer &r)
{
	return androidStorageImplStr(androidStorageImpl(r));
}
#endif

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

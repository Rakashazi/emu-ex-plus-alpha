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

#define LOGTAG "GLPixmapBufferTexture"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math/int.hh>
#include "private.hh"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#include "android/AHardwareBufferStorage.hh"
#include "android/GraphicBufferStorage.hh"
#include "android/SurfaceTextureStorage.hh"
#endif
#include <cstdlib>
#include <algorithm>

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif

#ifndef GL_MAP_FLUSH_EXPLICIT_BIT
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#endif

#ifndef GL_DYNAMIC_STORAGE_BIT
#define GL_DYNAMIC_STORAGE_BIT 0x0100
#endif

#ifndef GL_CLIENT_STORAGE_BIT
#define GL_CLIENT_STORAGE_BIT 0x0200
#endif

#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif

#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT 0x0080
#endif

namespace Gfx
{

PixmapBufferTexture::PixmapBufferTexture(Renderer &r, TextureConfig config, TextureBufferMode mode, bool singleBuffer, IG::ErrorCode *errorPtr)
{
	this->r = &r;
	IG::ErrorCode err = GLPixmapBufferTexture::init(*this, config, mode, texName_, singleBuffer);
	if(unlikely(err && errorPtr))
	{
		*errorPtr = err;
	}
}

PixmapBufferTexture::PixmapBufferTexture(PixmapBufferTexture &&o)
{
	*this = std::move(o);
}

PixmapBufferTexture &PixmapBufferTexture::operator=(PixmapBufferTexture &&o)
{
	GLPixmapBufferTexture::deinit(r);
	Texture::operator=((Texture &&)o);
	GLPixmapTexture::operator=(o);
	directTex = std::move(o.directTex);
	buffer = std::exchange(o.buffer, {});
	pbo = std::exchange(o.pbo, 0);
	bufferIdx = std::exchange(o.bufferIdx, 0);
	return *this;
}

PixmapBufferTexture::~PixmapBufferTexture()
{
	GLPixmapBufferTexture::deinit(r);
}

void GLPixmapBufferTexture::deinit(Renderer *r)
{
	if(!r)
		return;
	if(pbo)
	{
		assert(r->support.hasPBOFuncs);
		logMsg("deleting PBO:%u", pbo);
		r->runGLTask(
			[pbo = this->pbo]()
			{
				glDeleteBuffers(1, &pbo);
			});
		pbo = 0;
	}
	else
	{
		r->waitAsyncCommands();
		std::free(buffer[0].data);
	}
	buffer = {};
	bufferIdx = 0;
}

IG::ErrorCode GLPixmapBufferTexture::init(PixmapBufferTexture &self, TextureConfig config, TextureBufferMode mode, GLuint &texNameMember, bool singleBuffer)
{
	auto &r = self.renderer();
	mode = self.renderer().makeValidTextureBufferMode(mode);
	switch(mode)
	{
		default:
			return initWithPixelBuffer(self, config, false, singleBuffer);
		case TextureBufferMode::PBO:
			return initWithPixelBuffer(self, config, true, singleBuffer);
		#ifdef __ANDROID__
		case TextureBufferMode::ANDROID_HARDWARE_BUFFER:
			return initWithHardwareBuffer(self, config, singleBuffer);
		case TextureBufferMode::ANDROID_SURFACE_TEXTURE:
			return initWithSurfaceTexture(self, config, texNameMember, singleBuffer);
		#endif
	}
}

static std::array<GLPixmapBufferTexture::BufferInfo, 2> makeSystemMemoryPixelBuffer(unsigned bytes, void *oldBuffer, bool singleBuffer)
{
	std::free(oldBuffer);
	unsigned fullBytes = singleBuffer ? bytes : bytes * 2;
	auto bufferPtr = (char*)std::malloc(fullBytes);
	logMsg("allocated system memory pixel buffer with size:%u", fullBytes);
	return {bufferPtr, singleBuffer ? nullptr : bufferPtr + bytes};
}

static bool hasPersistentBufferMapping(Renderer &r)
{
	return r.support.hasImmutableBufferStorage();
}

void GLPixmapBufferTexture::initPixelBuffer(Renderer &r, IG::PixmapDesc desc, bool usePBO, bool singleBuffer)
{
	assert(!directTex);
	if(singleBuffer)
		bufferIdx = SINGLE_BUFFER_VALUE;
	const unsigned bufferBytes = desc.pixelBytes();
	if(usePBO)
	{
		assert(hasPersistentBufferMapping(r));
		char *bufferPtr;
		const unsigned fullBufferBytes = singleBuffer ? bufferBytes : bufferBytes * 2;
		r.runGLTaskSync(
			[=, &r, &bufferPtr, this]()
			{
				if(pbo)
				{
					glDeleteBuffers(1, &pbo);
					pbo = 0;
				}
				GLuint pbo;
				glGenBuffers(1, &pbo);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				r.support.glBufferStorage(GL_PIXEL_UNPACK_BUFFER, fullBufferBytes, nullptr,
					GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
				bufferPtr = (char*)r.support.glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, fullBufferBytes,
					GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
				if(unlikely(!bufferPtr))
				{
					logErr("PBO:%u mapping failed", pbo);
					glDeleteBuffers(1, &pbo);
				}
				else
				{
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					this->pbo = pbo;
				}
			});
		if(bufferPtr)
		{
			logMsg("allocated PBO:%u with size:%u, data @ %p", pbo, fullBufferBytes, bufferPtr);
			buffer[0] = {bufferPtr, nullptr};
			if(singleBuffer)
				buffer[1] = {nullptr};
			else
				buffer[1] = {bufferPtr + bufferBytes, (void *)(uintptr_t)bufferBytes};
		}
		else // fallback to system memory if PBO fails
			buffer = makeSystemMemoryPixelBuffer(bufferBytes, nullptr, singleBuffer);
	}
	else
	{
		r.waitAsyncCommands();
		buffer = makeSystemMemoryPixelBuffer(bufferBytes, buffer[0].data, singleBuffer);
	}
}

GLPixmapBufferTexture::BufferInfo GLPixmapBufferTexture::swapBuffer()
{
	if(isSingleBuffered())
	{
		return buffer[0];
	}
	else
	{
		auto info = buffer[bufferIdx];
		bufferIdx = (bufferIdx + 1) % 2;
		return info;
	}
}

bool GLPixmapBufferTexture::isSingleBuffered() const
{
	return bufferIdx == SINGLE_BUFFER_VALUE;
}

IG::ErrorCode GLPixmapBufferTexture::initWithPixelBuffer(PixmapBufferTexture &self, TextureConfig config, bool usePBO, bool singleBuffer)
{
	initPixelBuffer(self.renderer(), config.pixmapDesc(), usePBO, singleBuffer);
	return self.GLPixmapTexture::init(self, config);
}

#ifdef __ANDROID__
IG::ErrorCode GLPixmapBufferTexture::initWithHardwareBuffer(PixmapBufferTexture &self, TextureConfig config, bool singleBuffer)
{
	auto &r = self.renderer();
	config = self.baseInit(r, config);
	auto androidSDK = Base::androidSDK();
	if(androidSDK >= 26)
	{
		directTex = std::make_unique<AHardwareBufferStorage>();
	}
	else
	{
		directTex = std::make_unique<GraphicBufferStorage>();
	}
	return self.setFormat(config.pixmapDesc());
}

IG::ErrorCode GLPixmapBufferTexture::initWithSurfaceTexture(PixmapBufferTexture &self, TextureConfig config, GLuint &texNameMember, bool singleBuffer)
{
	auto &r = self.renderer();
	config = self.baseInit(r, config);
	IG::ErrorCode err{};
	auto surfaceTex = std::make_unique<SurfaceTextureStorage>(r, texNameMember, singleBuffer, err);
	if(unlikely(err))
	{
		return initWithPixelBuffer(self, config, false, singleBuffer);
	}
	else
	{
		directTex = std::move(surfaceTex);
	}
	return self.setFormat(config.pixmapDesc());
}
#endif

IG::ErrorCode PixmapBufferTexture::setFormat(IG::PixmapDesc desc)
{
	assumeExpr(r);
	if(directTex)
	{
		if(pixDesc == desc)
			logWarn("resizing with same dimensions %dx%d, should optimize caller code", desc.w(), desc.h());
		auto oldTexName = texName_;
		if(auto err = directTex->setFormat(*r, desc, texName_);
			unlikely(err))
		{
			return err;
		}
		if(oldTexName != texName_)
		{
			sampler = 0; // invalidate sampler settings from deleted texture name
		}
		updateUsedPixmapSize(desc, desc);
		updateFormatInfo(desc, 1, directTex->target);
		return {};
	}
	else
	{
		initPixelBuffer(renderer(), desc, pbo, isSingleBuffered());
		return PixmapTexture::setFormat(desc, 1);
	}
}

void PixmapBufferTexture::writeAligned(IG::Pixmap pixmap, uint8_t assumeAlign, uint32_t writeFlags)
{
	//logDMsg("writing pixmap %dx%d to pos %dx%d", pixmap.x, pixmap.y, destPos.x, destPos.y);
	if(unlikely(!texName_))
	{
		logErr("called writeAligned() on uninitialized texture");
		return;
	}
	if(directTex)
	{
		assumeExpr(r);
		assumeExpr(pixmap.format() == pixDesc.format());
		/*if(destPos != IG::WP{0, 0} || pixmap.w() != (uint32_t)size().x || pixmap.h() != (uint32_t)size().y)
		{
			logErr("partial write of direct texture unsupported, use lock()");
			return;
		}*/
		auto lockBuff = lock();
		if(unlikely(!lockBuff))
		{
			return;
		}
		lockBuff.pixmap().write(pixmap, {});
		unlock(lockBuff);
	}
	else
	{
		Texture::writeAligned(0, pixmap, {}, assumeAlign, writeFlags);
	}
}

void PixmapBufferTexture::write(IG::Pixmap pixmap, uint32_t writeFlags)
{
	writeAligned(pixmap, bestAlignment(pixmap), writeFlags);
}

void PixmapBufferTexture::clear()
{
	auto lockBuff = lock(BUFFER_FLAG_CLEARED);
	if(unlikely(!lockBuff))
	{
		logErr("error getting buffer for clear()");
		return;
	}
	unlock(lockBuff);
}

LockedTextureBuffer PixmapBufferTexture::lock(uint32_t bufferFlags)
{
	assumeExpr(r);
	const unsigned fullBufferBytes = pixDesc.pixelBytes();
	IG::WindowRect fullRect{0, 0, size().x, size().y};
	if(directTex)
	{
		auto buff = directTex->lock(*r);
		if(unlikely(!buff.data))
		{
			logWarn("falling back to system memory due to direct storage lock failure");
			return Texture::lock(0, fullRect, bufferFlags);
		}
		r->resourceUpdate = true;
		IG::Pixmap pix{pixDesc, buff.data, {buff.pitchBytes, IG::Pixmap::BYTE_UNITS}};
		if(bufferFlags & BUFFER_FLAG_CLEARED)
			pix.clear();
		return {nullptr, pix, fullRect, 0, false};
	}
	r->resourceUpdate = true;
	auto bufferInfo = swapBuffer();
	IG::Pixmap pix{{fullRect.size(), pixDesc.format()}, bufferInfo.data};
	if(bufferFlags & BUFFER_FLAG_CLEARED)
		pix.clear();
	return {bufferInfo.bufferOffset, pix, fullRect, 0, false, pbo};
}

void PixmapBufferTexture::unlock(LockedTextureBuffer lockBuff, uint32_t)
{
	if(unlikely(!lockBuff))
		return;
	if(!lockBuff.shouldFreeBuffer() && directTex)
	{
		assumeExpr(r);
		directTex->unlock(*r);
	}
	else
	{
		Texture::unlock(lockBuff, 0);
	}
}

IG::WP PixmapBufferTexture::size() const
{
	return Texture::size(0);
}

#ifdef __ANDROID__
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

static bool hasSurfaceTexture(Renderer &r)
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
	return true;
}

static bool hasHardwareBuffer(Renderer &r)
{
	auto androidSDK = Base::androidSDK();
	if(androidSDK >= 26)
		return true;
	if(!r.support.hasEGLImages)
	{
		logErr("Can't use GraphicBuffer without OES_EGL_image extension");
		return false;
	}
	if(GraphicBufferStorage::isSupported())
		return true;
	auto rendererStr = rendererGLStr(r);
	if(!GraphicBufferStorage::canSupport(rendererStr))
		return false;
	return GraphicBufferStorage::testSupport();
}
#endif

DirectTextureStorage::~DirectTextureStorage() {}

std::vector<TextureBufferModeDesc> Renderer::textureBufferModes()
{
	std::vector<TextureBufferModeDesc> methodDesc;
	methodDesc.reserve(Config::envIsAndroid ? 4 : 2);
	methodDesc.emplace_back("System Memory", TextureBufferMode::SYSTEM_MEMORY);
	if(hasPersistentBufferMapping(*this))
	{
		methodDesc.emplace_back("OpenGL PBO", TextureBufferMode::PBO);
	}
	#ifdef __ANDROID__
	if(hasHardwareBuffer(*this))
	{
		methodDesc.emplace_back("Hardware Buffer", TextureBufferMode::ANDROID_HARDWARE_BUFFER);
	}
	if(hasSurfaceTexture(*this))
	{
		methodDesc.emplace_back("Surface Texture", TextureBufferMode::ANDROID_SURFACE_TEXTURE);
	}
	#endif
	return methodDesc;
}

TextureBufferMode Renderer::makeValidTextureBufferMode(TextureBufferMode mode)
{
	switch(mode)
	{
		case TextureBufferMode::DEFAULT:
			#ifdef __ANDROID__
			if(hasHardwareBuffer(*this))
			{
				return TextureBufferMode::ANDROID_HARDWARE_BUFFER;
			}
			#endif
			if(hasPersistentBufferMapping(*this))
			{
				return TextureBufferMode::PBO;
			}
			return TextureBufferMode::SYSTEM_MEMORY;
		case TextureBufferMode::PBO:
			return hasPersistentBufferMapping(*this) ? TextureBufferMode::PBO : TextureBufferMode::SYSTEM_MEMORY;
		#ifdef __ANDROID__
		case TextureBufferMode::ANDROID_HARDWARE_BUFFER:
			return hasHardwareBuffer(*this) ? TextureBufferMode::ANDROID_HARDWARE_BUFFER : TextureBufferMode::SYSTEM_MEMORY;
		case TextureBufferMode::ANDROID_SURFACE_TEXTURE:
			return hasSurfaceTexture(*this) ? TextureBufferMode::ANDROID_SURFACE_TEXTURE : TextureBufferMode::SYSTEM_MEMORY;
		#endif
		default:
			return TextureBufferMode::SYSTEM_MEMORY;
	}
}

}

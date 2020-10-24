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
	mode = r.makeValidTextureBufferMode(mode);
	IG::ErrorCode err = init(r, config, mode, singleBuffer);
	if(unlikely(err && mode != TextureBufferMode::SYSTEM_MEMORY))
	{
		logErr("falling back to system memory");
		err = init(r, config, TextureBufferMode::SYSTEM_MEMORY, singleBuffer);
	}
	if(unlikely(err && errorPtr))
	{
		*errorPtr = err;
	}
}

IG::ErrorCode GLPixmapBufferTexture::init(Renderer &r, TextureConfig config, TextureBufferMode mode, bool singleBuffer)
{
	switch(mode)
	{
		default:
			return initWithPixelBuffer(r, config, false, singleBuffer);
		case TextureBufferMode::PBO:
			return initWithPixelBuffer(r, config, true, singleBuffer);
		#ifdef __ANDROID__
		case TextureBufferMode::ANDROID_HARDWARE_BUFFER:
			return initWithHardwareBuffer(r, config, singleBuffer);
		case TextureBufferMode::ANDROID_SURFACE_TEXTURE:
			return initWithSurfaceTexture(r, config, singleBuffer);
		#endif
	}
}

static bool hasPersistentBufferMapping(Renderer &r)
{
	return r.support.hasImmutableBufferStorage();
}

IG::ErrorCode GLPixmapBufferTexture::initWithPixelBuffer(Renderer &r, TextureConfig config, bool usePBO, bool singleBuffer)
{
	IG::ErrorCode err;
	directTex = std::make_unique<GLTextureStorage>(r, config, usePBO, singleBuffer, &err);
	return err;
}

#ifdef __ANDROID__
IG::ErrorCode GLPixmapBufferTexture::initWithHardwareBuffer(Renderer &r, TextureConfig config, bool singleBuffer)
{
	IG::ErrorCode err{};
	auto androidSDK = Base::androidSDK();
	if(androidSDK >= 26)
	{
		directTex = std::make_unique<AHardwareBufferStorage>(r, config, &err);
	}
	else
	{
		directTex = std::make_unique<GraphicBufferStorage>(r, config, &err);
	}
	return err;
}

IG::ErrorCode GLPixmapBufferTexture::initWithSurfaceTexture(Renderer &r, TextureConfig config, bool singleBuffer)
{
	IG::ErrorCode err{};
	directTex = std::make_unique<SurfaceTextureStorage>(r, config, singleBuffer, &err);
	return err;
}
#endif

IG::ErrorCode PixmapBufferTexture::setFormat(IG::PixmapDesc desc)
{
	if(unlikely(!directTex))
		return {EINVAL};
	if(Config::DEBUG_BUILD && directTex->pixmapDesc() == desc)
		logWarn("resizing with same dimensions %dx%d, should optimize caller code", desc.w(), desc.h());
	return directTex->setFormat(desc);
}

void PixmapBufferTexture::writeAligned(IG::Pixmap pixmap, uint8_t assumeAlign, uint32_t writeFlags)
{
	assumeExpr(directTex);
	return directTex->writeAligned(pixmap, assumeAlign, writeFlags);
}

void PixmapBufferTexture::write(IG::Pixmap pixmap, uint32_t writeFlags)
{
	writeAligned(pixmap, Texture::bestAlignment(pixmap), writeFlags);
}

void PixmapBufferTexture::clear()
{
	auto lockBuff = lock(Texture::BUFFER_FLAG_CLEARED);
	if(unlikely(!lockBuff))
	{
		logErr("error getting buffer for clear()");
		return;
	}
	unlock(lockBuff);
}

LockedTextureBuffer PixmapBufferTexture::lock(uint32_t bufferFlags)
{
	assumeExpr(directTex);
	return directTex->lock(bufferFlags);
}

void PixmapBufferTexture::unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags)
{
	if(unlikely(!lockBuff))
		return;
	directTex->unlock(lockBuff, writeFlags);
}

IG::WP PixmapBufferTexture::size() const
{
	if(unlikely(!directTex))
		return {};
	return directTex->size(0);
}

IG::PixmapDesc PixmapBufferTexture::pixmapDesc() const
{
	if(unlikely(!directTex))
		return {};
	return directTex->pixmapDesc();
}

IG::PixmapDesc PixmapBufferTexture::usedPixmapDesc() const
{
	if(unlikely(!directTex))
		return {};
	return directTex->usedPixmapDesc();
}

bool PixmapBufferTexture::compileDefaultProgram(uint32_t mode) const
{
	assumeExpr(directTex);
	return directTex->compileDefaultProgram(mode);
}

PixmapBufferTexture::operator bool() const
{
	return directTex && (bool)*directTex;
}

Renderer &PixmapBufferTexture::renderer()
{
	return directTex->renderer();
}

PixmapBufferTexture::operator TextureSpan() const
{
	if(unlikely(!directTex))
		return {};
	return (TextureSpan)*directTex;
}

PixmapBufferTexture::operator const Texture&() const
{
	assumeExpr(directTex);
	return *directTex;
}

bool PixmapBufferTexture::isExternal() const
{
	return Config::envIsAndroid && directTex->isExternal();
}

static std::array<GLTextureStorage::BufferInfo, 2> makeSystemMemoryPixelBuffer(unsigned bytes, void *oldBuffer, bool singleBuffer)
{
	std::free(oldBuffer);
	unsigned fullBytes = singleBuffer ? bytes : bytes * 2;
	auto bufferPtr = (char*)std::malloc(fullBytes);
	logMsg("allocated system memory pixel buffer with size:%u", fullBytes);
	return {bufferPtr, singleBuffer ? nullptr : bufferPtr + bytes};
}

GLTextureStorage::GLTextureStorage(Renderer &r, TextureConfig config, bool usePBO, bool singleBuffer, IG::ErrorCode *errorPtr):
	TextureBufferStorage{r}
{
	initPixelBuffer(config.pixmapDesc(), usePBO, singleBuffer);
	auto err = GLPixmapTexture::init(r, config);
	if(unlikely(err && errorPtr))
	{
		*errorPtr = err;
	}
}

GLTextureStorage::~GLTextureStorage()
{
	deinit();
}

GLTextureStorage::GLTextureStorage(GLTextureStorage &&o)
{
	*this = std::move(o);
}

GLTextureStorage &GLTextureStorage::operator=(GLTextureStorage &&o)
{
	deinit();
	TextureBufferStorage::operator=(std::move(o));
	buffer = std::exchange(o.buffer, {});
	pbo = std::exchange(o.pbo, {});
	bufferIdx = std::exchange(o.bufferIdx, {});
	return *this;
}

void GLTextureStorage::initPixelBuffer(IG::PixmapDesc desc, bool usePBO, bool singleBuffer)
{
	if(singleBuffer)
		bufferIdx = SINGLE_BUFFER_VALUE;
	const unsigned bufferBytes = desc.pixelBytes();
	auto &r = renderer();
	if(usePBO)
	{
		assert(hasPersistentBufferMapping(r));
		char *bufferPtr;
		const unsigned fullBufferBytes = singleBuffer ? bufferBytes : bufferBytes * 2;
		r.runGLTaskSync(
			[=, &r, &bufferPtr, &pbo = pbo]()
			{
				if(pbo)
				{
					glDeleteBuffers(1, &pbo);
					pbo = 0;
				}
				GLuint newPbo;
				glGenBuffers(1, &newPbo);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, newPbo);
				r.support.glBufferStorage(GL_PIXEL_UNPACK_BUFFER, fullBufferBytes, nullptr,
					GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
				bufferPtr = (char*)r.support.glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, fullBufferBytes,
					GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
				if(unlikely(!bufferPtr))
				{
					logErr("PBO:%u mapping failed", newPbo);
					glDeleteBuffers(1, &newPbo);
				}
				else
				{
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					pbo = newPbo;
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

GLTextureStorage::BufferInfo GLTextureStorage::swapBuffer()
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

bool GLTextureStorage::isSingleBuffered() const
{
	return bufferIdx == SINGLE_BUFFER_VALUE;
}

IG::ErrorCode GLTextureStorage::setFormat(IG::PixmapDesc desc)
{
	initPixelBuffer(desc, pbo, isSingleBuffered());
	return PixmapTexture::setFormat(desc, 1);
}

LockedTextureBuffer GLTextureStorage::lock(uint32_t bufferFlags)
{
	auto bufferInfo = swapBuffer();
	IG::WindowRect fullRect{0, 0, size(0).x, size(0).y};
	IG::Pixmap pix{{fullRect.size(), pixmapDesc().format()}, bufferInfo.data};
	if(bufferFlags & Texture::BUFFER_FLAG_CLEARED)
		pix.clear();
	return {bufferInfo.bufferOffset, pix, fullRect, 0, false, pbo};
}

void GLTextureStorage::unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags)
{
	Texture::unlock(lockBuff, writeFlags);
}

void GLTextureStorage::writeAligned(IG::Pixmap pixmap, uint8_t assumeAlign, uint32_t writeFlags)
{
	if(unlikely(!texName()))
	{
		logErr("called writeAligned() on uninitialized texture");
		return;
	}
	if(renderer().support.hasUnpackRowLength || !pixmap.isPadded())
	{
		Texture::writeAligned(0, pixmap, {}, assumeAlign, writeFlags);
	}
	else
	{
		assumeExpr(pixmap.format() == pixmapDesc().format());
		auto lockBuff = lock();
		if(unlikely(!lockBuff))
		{
			return;
		}
		lockBuff.pixmap().write(pixmap, {});
		unlock(lockBuff);
	}
}

void GLTextureStorage::deinit()
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

TextureBufferStorage::~TextureBufferStorage() {}

LockedTextureBuffer TextureBufferStorage::makeLockedBuffer(void *data, uint32_t pitchBytes, uint32_t bufferFlags)
{
	IG::WindowRect fullRect{0, 0, size(0).x, size(0).y};
	IG::Pixmap pix{pixmapDesc(), data, {pitchBytes, IG::Pixmap::BYTE_UNITS}};
	if(bufferFlags & Texture::BUFFER_FLAG_CLEARED)
		pix.clear();
	return {nullptr, pix, fullRect, 0, false};
}


void TextureBufferStorage::writeAligned(IG::Pixmap pixmap, uint8_t assumeAlign, uint32_t writeFlags)
{
	if(unlikely(!texName()))
	{
		logErr("called writeAligned() on uninitialized texture");
		return;
	}
	assumeExpr(pixmap.format() == pixmapDesc().format());
	auto lockBuff = lock();
	if(unlikely(!lockBuff))
	{
		return;
	}
	lockBuff.pixmap().write(pixmap, {});
	unlock(lockBuff);
}

bool TextureBufferStorage::isExternal() const
{
	return Config::envIsAndroid && target() == GL_TEXTURE_EXTERNAL_OES;
}

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

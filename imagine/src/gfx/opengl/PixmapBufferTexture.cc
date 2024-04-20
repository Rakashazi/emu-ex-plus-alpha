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
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math.hh>
#ifdef __ANDROID__
#include <imagine/gfx/opengl/android/HardwareBufferStorage.hh>
#include <imagine/gfx/opengl/android/SurfaceTextureStorage.hh>
#endif
#include <imagine/logger/logger.h>
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

#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

namespace IG::Gfx
{

PixmapBufferTexture::PixmapBufferTexture(RendererTask &r, TextureConfig config, TextureBufferMode mode, bool singleBuffer)
{
	mode = r.renderer().evalTextureBufferMode(mode);
	try
	{
		if(mode == TextureBufferMode::SYSTEM_MEMORY)
			initWithSystemMemory(r, config, singleBuffer);
		else if(mode == TextureBufferMode::PBO)
			initWithPixelBuffer(r, config, singleBuffer);
		else if(Config::envIsAndroid && mode == TextureBufferMode::ANDROID_HARDWARE_BUFFER)
			initWithHardwareBuffer(r, config, singleBuffer);
		else if(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL && mode == TextureBufferMode::ANDROID_SURFACE_TEXTURE)
			initWithSurfaceTexture(r, config, singleBuffer);
		else
			bug_unreachable("mode == %d", std::to_underlying(mode));
	}
	catch(std::exception &)
	{
		if(mode != TextureBufferMode::SYSTEM_MEMORY)
		{
			logErr("falling back to system memory");
			initWithSystemMemory(r, config, singleBuffer);
		}
		else
			throw;
	}
}

static bool hasPersistentBufferMapping(const Renderer &r)
{
	return r.support.hasImmutableBufferStorage();
}

void GLPixmapBufferTexture::initWithSystemMemory(RendererTask &r, TextureConfig config, bool singleBuffer)
{
	directTex.emplace<GLSystemMemoryStorage>(r, config, singleBuffer);
}

void GLPixmapBufferTexture::initWithPixelBuffer(RendererTask &r, TextureConfig config, bool singleBuffer)
{
	directTex.emplace<GLPixelBufferStorage>(r, config, singleBuffer);
}

#ifdef __ANDROID__
void GLPixmapBufferTexture::initWithHardwareBuffer(RendererTask &r, TextureConfig config, bool singleBuffer)
{
	auto androidSDK = r.appContext().androidSDK();
	if(androidSDK >= 26)
	{
		if(singleBuffer)
			directTex.emplace<AHardwareSingleBufferStorage>(r, config);
		else
			directTex.emplace<AHardwareBufferStorage>(r, config);
	}
	else
	{
		if(singleBuffer)
			directTex.emplace<GraphicSingleBufferStorage>(r, config);
		else
			directTex.emplace<GraphicBufferStorage>(r, config);
	}
}
#endif

#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
void GLPixmapBufferTexture::initWithSurfaceTexture(RendererTask &r, TextureConfig config, bool singleBuffer)
{
	assert(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL);
	directTex.emplace<SurfaceTextureStorage>(r, config, singleBuffer);
}
#endif

bool PixmapBufferTexture::setFormat(PixmapDesc desc, ColorSpace colorSpace, TextureSamplerConfig samplerConf)
{
	if(Config::DEBUG_BUILD && pixmapDesc() == desc)
		logWarn("resizing with same dimensions %dx%d, should optimize caller code", desc.w(), desc.h());
	return visit([&](auto &t){ return t.setFormat(desc, colorSpace, samplerConf); }, directTex);
}

void PixmapBufferTexture::writeAligned(PixmapView pixmap, int assumeAlign, TextureWriteFlags writeFlags)
{
	visit([&](auto &t)
	{
		if constexpr(requires {t.writeAligned(pixmap, assumeAlign, writeFlags);})
		{
			t.writeAligned(pixmap, assumeAlign, writeFlags);
		}
		else
		{
			auto lockBuff = lock();
			if(!lockBuff) [[unlikely]]
			{
				return;
			}
			lockBuff.pixmap().write(pixmap, {});
			unlock(lockBuff);
		}
	}, directTex);
}

void PixmapBufferTexture::write(PixmapView pixmap, TextureWriteFlags writeFlags)
{
	writeAligned(pixmap, Texture::bestAlignment(pixmap), writeFlags);
}

void PixmapBufferTexture::clear()
{
	auto lockBuff = lock({.clear = true});
	if(!lockBuff) [[unlikely]]
	{
		logErr("error getting buffer for clear()");
		return;
	}
	unlock(lockBuff);
}

LockedTextureBuffer PixmapBufferTexture::lock(TextureBufferFlags bufferFlags)
{
	return visit([&](auto &t){ return t.lock(bufferFlags); }, directTex);
}

void PixmapBufferTexture::unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags)
{
	visit([&](auto &t){ t.unlock(lockBuff, writeFlags); }, directTex);
}

WSize PixmapBufferTexture::size() const
{
	return visit([&](auto &t){ return t.size(0); }, directTex);
}

PixmapDesc PixmapBufferTexture::pixmapDesc() const
{
	return visit([&](auto &t){ return t.pixmapDesc(); }, directTex);
}

void PixmapBufferTexture::setSampler(TextureSamplerConfig samplerConf)
{
	visit([&](auto &t){ t.setSampler(samplerConf); }, directTex);
}

PixmapBufferTexture::operator bool() const
{
	return visit([&](auto &t){ return (bool)t; }, directTex);
}

Renderer &PixmapBufferTexture::renderer() const
{
	return visit([&](auto &t) -> Renderer&{ return t.renderer(); }, directTex);
}

PixmapBufferTexture::operator TextureSpan() const
{
	return visit([&](auto &t) -> TextureSpan{ return t; }, directTex);
}

PixmapBufferTexture::operator const Texture&() const
{
	return visit([&](auto &t)-> const Texture&{ return t; }, directTex);
}

bool PixmapBufferTexture::isExternal() const
{
	return Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL &&
		visit([&](auto &t){ return t.target() == GL_TEXTURE_EXTERNAL_OES; }, directTex);
}

template<class Impl, class BufferInfo>
bool GLTextureStorage<Impl, BufferInfo>::setFormat(PixmapDesc desc, ColorSpace colorSpace, TextureSamplerConfig samplerConf)
{
	static_cast<Impl*>(this)->initBuffer(desc, isSingleBuffered());
	return Texture::setFormat(desc, 1, colorSpace, samplerConf);
}

template<class Impl, class BufferInfo>
LockedTextureBuffer GLTextureStorage<Impl, BufferInfo>::lock(TextureBufferFlags bufferFlags)
{
	if(!texName()) [[unlikely]]
	{
		logErr("called lock when uninitialized");
		return {};
	}
	auto bufferInfo = currentBuffer();
	IG::WindowRect fullRect{{}, size(0)};
	MutablePixmapView pix{{fullRect.size(), pixmapDesc().format}, bufferInfo.data};
	if(bufferFlags.clear)
		pix.clear();
	GLuint pbo{};
	if constexpr(requires {static_cast<Impl*>(this)->pbo();})
	{
		pbo = static_cast<Impl*>(this)->pbo();
	}
	return {bufferInfo.dataStoreOffset(), pix, fullRect, 0, false, pbo};
}

template<class Impl, class BufferInfo>
void GLTextureStorage<Impl, BufferInfo>::unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags)
{
	Texture::unlock(lockBuff, writeFlags);
	swapBuffer();
}

template<class Impl, class BufferInfo>
void GLTextureStorage<Impl, BufferInfo>::writeAligned(PixmapView pixmap, int assumeAlign, TextureWriteFlags writeFlags)
{
	if(renderer().support.hasUnpackRowLength || !pixmap.isPadded())
	{
		Texture::writeAligned(0, pixmap, {}, assumeAlign, writeFlags);
	}
	else
	{
		auto lockBuff = lock();
		if(!lockBuff) [[unlikely]]
		{
			return;
		}
		lockBuff.pixmap().write(pixmap, {});
		unlock(lockBuff);
	}
}

GLSystemMemoryStorage::GLSystemMemoryStorage(RendererTask &rTask, TextureConfig config, bool singleBuffer):
	GLTextureStorage{rTask, config, singleBuffer}
{
	initBuffer(config.pixmapDesc, singleBuffer);
}

void GLSystemMemoryStorage::initBuffer(PixmapDesc desc, bool singleBuffer)
{
	task().awaitPending();
	auto bytes = desc.bytes();
	auto fullBytes = singleBuffer ? bytes : bytes * 2;
	storage = std::make_unique<char[]>(fullBytes);
	logMsg("allocated system memory with buffers:%d size:%d data:%p", singleBuffer ? 1 : 2, bytes, storage.get());
	info[0] = {storage.get()};
	info[1] = {singleBuffer ? nullptr : storage.get() + bytes};
}

GLPixelBufferStorage::GLPixelBufferStorage(RendererTask &rTask, TextureConfig config, bool singleBuffer):
	GLTextureStorage{rTask, config, singleBuffer},
	pixelBuff{GLBufferDeleter{&rTask}}
{
	initBuffer(config.pixmapDesc, singleBuffer);
}

void GLPixelBufferStorage::initBuffer(PixmapDesc desc, bool singleBuffer)
{
	const auto bufferBytes = desc.bytes();
	auto &r = renderer();
	assert(hasPersistentBufferMapping(r));
	char *bufferPtr{};
	const auto fullBufferBytes = singleBuffer ? bufferBytes : bufferBytes * 2;
	task().runSync(
		[=, &r, &bufferPtr, &pbo = pixelBuff.get()](GLTask::TaskContext ctx)
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
			if(!bufferPtr) [[unlikely]]
			{
				logErr("PBO:%u mapping failed", newPbo);
				ctx.notifySemaphore();
				glDeleteBuffers(1, &newPbo);
			}
			else
			{
				pbo = newPbo;
				ctx.notifySemaphore();
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			}
		});
	if(bufferPtr)
	{
		logMsg("allocated PBO:%u with buffers:%u size:%u data:%p", pixelBuff.get(), singleBuffer ? 1 : 2, bufferBytes, bufferPtr);
		info[0] = {bufferPtr, nullptr};
		if(singleBuffer)
			info[1] = {nullptr};
		else
		{
			info[1] = {bufferPtr + bufferBytes, (void *)(uintptr_t)bufferBytes};
		}
	}
	else [[unlikely]]
	{
		throw std::runtime_error("Error creating pixel buffer");
	}
}

template class GLTextureStorage<GLSystemMemoryStorage, GLSystemMemoryBufferInfo>;
template class GLTextureStorage<GLPixelBufferStorage, GLPixelBufferInfo>;

#ifdef __ANDROID__
static const char *rendererGLStr(Renderer &r)
{
	const char *str= "";
	r.task().runSync(
		[&str]()
		{
			str = (const char*)glGetString(GL_RENDERER);
		});
	return str;
}

static bool hasSurfaceTexture(Renderer &r)
{
	if(!Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL)
		return false;
	if(r.appContext().androidSDK() < 14)
		return false;
	if(!r.support.hasExternalEGLImages)
	{
		logErr("can't use SurfaceTexture without OES_EGL_image_external");
		return false;
	}
	return true;
}

static bool hasHardwareBuffer(Renderer &r)
{
	auto androidSDK = r.appContext().androidSDK();
	if(androidSDK >= 26)
		return true;
	if(!r.support.hasEGLImages)
	{
		logErr("Can't use GraphicBuffer without OES_EGL_image extension");
		return false;
	}
	if(GraphicBuffer::isSupported())
		return true;
	auto rendererStr = rendererGLStr(r);
	if(!GraphicBuffer::canSupport(r.appContext(), rendererStr))
		return false;
	return GraphicBuffer::testSupport();
}
#endif

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

TextureBufferMode Renderer::evalTextureBufferMode(TextureBufferMode mode)
{
	switch(mode)
	{
		default:
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
		case TextureBufferMode::SYSTEM_MEMORY:
			return TextureBufferMode::SYSTEM_MEMORY;
		case TextureBufferMode::PBO:
			return hasPersistentBufferMapping(*this) ? TextureBufferMode::PBO : evalTextureBufferMode();
		#ifdef __ANDROID__
		case TextureBufferMode::ANDROID_HARDWARE_BUFFER:
			return hasHardwareBuffer(*this) ? TextureBufferMode::ANDROID_HARDWARE_BUFFER : evalTextureBufferMode();
		case TextureBufferMode::ANDROID_SURFACE_TEXTURE:
			return hasSurfaceTexture(*this) ? TextureBufferMode::ANDROID_SURFACE_TEXTURE : evalTextureBufferMode();
		#endif
	}
}

TextureBufferMode Renderer::validateTextureBufferMode(TextureBufferMode mode)
{
	if(mode == Gfx::TextureBufferMode::DEFAULT || evalTextureBufferMode(mode) == mode)
		return mode;
	return Gfx::TextureBufferMode::DEFAULT;
}

}

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
#include <imagine/gfx/Buffer.hh>
#include "internalDefs.hh"
#include <imagine/logger/logger.h>

#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif

namespace IG::Gfx
{

constexpr SystemLogger log{"GLBuffer"};

constexpr GLenum toGLEnum(BufferType type)
{
	using enum BufferType;
	switch(type)
	{
		case vertex: return GL_ARRAY_BUFFER;
		case index: return GL_ELEMENT_ARRAY_BUFFER;
	}
	std::unreachable();
}

constexpr GLenum toGLEnum(BufferUsageHint type)
{
	using enum BufferUsageHint;
	switch(type)
	{
		case streaming: return GL_STREAM_DRAW;
		case constant: return GL_STATIC_DRAW;
		case dynamic: return GL_DYNAMIC_DRAW;
	}
	std::unreachable();
}

static bool hasBufferMap(const Renderer &r)
{
	return !Config::Gfx::OPENGL_ES || (Config::Gfx::OPENGL_ES && r.support.glMapBufferRange);
}

template<BufferType type>
GLBuffer<type>::GLBuffer(RendererTask &rTask, ByteBufferConfig config):
	buffer{GLBufferRefDeleter{&rTask}}
{
	reset(config);
}

static void allocBufferData(GLenum target, GLuint name, GLsizeiptr size, GLenum usage)
{
	glBindBuffer(target, name);
	glBufferData(target, size, nullptr, usage);
}

template<BufferType type>
void GLBuffer<type>::reset(ByteBufferConfig config)
{
	if(sizeBytes_ == config.size)
		return;
	sizeBytes_ = config.size;
	auto usage = toGLEnum(config.usageHint);
	if(!name())
	{
		task().runSync([&buffer = buffer, size = config.size, usage](GLTask::TaskContext ctx)
		{
			GLuint name;
			glGenBuffers(1, &name);
			buffer.get() = name;
			log.info("created buffer object:0x{:X} size:{}", name, size);
			ctx.notifySemaphore();
			allocBufferData(toGLEnum(type), name, size, usage);
		});
	}
	else
	{
		task().run([name = name(), size = config.size, usage]()
		{
			log.info("reset buffer object:0x{:X} size:{}", name, size);
			allocBufferData(toGLEnum(type), name, size, usage);
		});
	}
}

template<BufferType type>
ByteBuffer GLBuffer<type>::map()
{
	if(hasBufferMap(task().renderer()))
	{
		void *ptr;
		task().run([this, &ptr]()
		{
			auto target = toGLEnum(type);
			glBindBuffer(target, name());
			ptr = task().renderer().support.glMapBufferRange(target,
				0, sizeBytes(), GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT);
			//log.debug("mapped buffer:0x{:X} to {}", name(), ptr);
		}, true);
		return {{static_cast<uint8_t*>(ptr), sizeBytes()}, [this](const uint8_t *ptr, size_t)
		{
			task().run([name = name(), &support = task().renderer().support]()
			{
				//log.debug("unmapping buffer:0x{:X}", name);
				auto target = toGLEnum(type);
				glBindBuffer(target, name);
				support.glUnmapBuffer(target);
			});
		}};
	}
	else
	{
		return {{std::make_unique<uint8_t[]>(sizeBytes()).release(), sizeBytes()}, [this](const uint8_t *ptr, size_t)
		{
			task().run([name = name(), sizeBytes = sizeBytes(), dataPtr = ptr]()
			{
				//log.debug("writing mapped data to buffer:0x{:X}", name);
				writeSubData(name, 0, sizeBytes, dataPtr);
				delete[] dataPtr;
			});
		}};
	}
}

template<BufferType type>
void GLBuffer<type>::writeSubData(ssize_t offset, size_t size, const void *data)
{
	if(offset + size > sizeBytes_)
		log.error("write of offset:{} size:{} larger than buffer size:{}", offset, size, sizeBytes_);
	writeSubData(name(), offset, size, data);
}

template<BufferType type>
void GLBuffer<type>::writeSubData(GLuint name, ssize_t offset, size_t size, const void *data)
{
	auto target = toGLEnum(type);
	glBindBuffer(target, name);
	glBufferSubData(target, offset, size, data);
}

void destroyGLBufferRef(RendererTask &rTask, GLBufferRef name)
{
	rTask.run([name]()
	{
		log.debug("deleting buffer object:0x{:X}", name);
		glDeleteBuffers(1, &name);
	});
}

template class GLBuffer<BufferType::vertex>;
template class GLBuffer<BufferType::index>;

}

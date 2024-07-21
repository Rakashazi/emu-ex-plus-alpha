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

#ifndef GL_MAP_INVALIDATE_RANGE_BIT
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#endif

#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
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
	return !Config::Gfx::OPENGL_ES || (Config::Gfx::OPENGL_ES && (bool)r.support.glMapBufferRange);
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
	assumeExpr(taskPtr());
	if(name() && sizeBytes_ == config.size)
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
			//log.info("created buffer object:0x{:X} size:{}", name, size);
			ctx.notifySemaphore();
			allocBufferData(toGLEnum(type), name, size, usage);
		});
	}
	else
	{
		task().run([name = name(), size = config.size, usage]()
		{
			//log.info("reset buffer object:0x{:X} size:{}", name, size);
			allocBufferData(toGLEnum(type), name, size, usage);
		});
	}
}

template<BufferType type>
MappedByteBuffer GLBuffer<type>::map(ssize_t offset, size_t size, BufferMapMode mode)
{
	assumeExpr(taskPtr());
	if(!size)
		size = sizeBytes() - offset;
	assert(offset + size <= sizeBytes());
	if(!size)
		return {};
	if(mode == BufferMapMode::unset)
		mode = BufferMapMode::direct;
	if(mode == BufferMapMode::direct && hasBufferMap(task().renderer()))
	{
		void *ptr;
		task().runSync([this, &ptr, offset, size]()
		{
			auto target = toGLEnum(type);
			glBindBuffer(target, name());
			ptr = task().renderer().support.glMapBufferRange(target,
				offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			//log.debug("mapped offset:{} size:{} of buffer:0x{:X} to {}", offset, size, name(), ptr);
		});
		return {{static_cast<uint8_t*>(ptr), size}, [this](const uint8_t*, size_t)
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
		return {{std::make_unique<uint8_t[]>(size).release(), size}, [this, offset](const uint8_t *ptr, size_t size)
		{
			task().run([name = name(), offset = offset, sizeBytes = size, dataPtr = ptr]()
			{
				//log.debug("writing mapped data to buffer:0x{:X}", name);
				writeSubData(name, offset, sizeBytes, dataPtr);
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
		//log.debug("deleting buffer object:0x{:X}", name);
		glDeleteBuffers(1, &name);
	});
}

template class GLBuffer<BufferType::vertex>;
template class GLBuffer<BufferType::index>;

void GLVertexArray::initArray(GLBufferRef vbo, GLBufferRef ibo, int stride, VertexLayoutDesc layoutDesc)
{
	assumeExpr(taskPtr());
	if(!task().renderer().support.hasVAOFuncs())
	{
		arr.get() = ibo;
		return;
	}
	if(arr.get())
		return;
	task().runSync([=, &support = task().renderer().support, &arr = arr](GLTask::TaskContext ctx)
	{
		GLuint name;
		support.glGenVertexArrays(1, &name);
		arr.get() = name;
		//log.info("created vertex array object:{:X}", name);
		ctx.notifySemaphore();
		support.glBindVertexArray(name);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(VATTR_POS);
		if(layoutDesc.texCoord.size)
			glEnableVertexAttribArray(VATTR_TEX_UV);
		else
			glDisableVertexAttribArray(VATTR_TEX_UV);
		if(layoutDesc.color.size)
			glEnableVertexAttribArray(VATTR_COLOR);
		else
			glDisableVertexAttribArray(VATTR_COLOR);
		glVertexAttribPointer(VATTR_POS, layoutDesc.pos.size, asGLType(layoutDesc.pos.type),
			layoutDesc.pos.normalize, stride, (const void*)layoutDesc.pos.offset);
		if(layoutDesc.texCoord.size)
		{
			glVertexAttribPointer(VATTR_TEX_UV, layoutDesc.texCoord.size, asGLType(layoutDesc.texCoord.type),
				layoutDesc.texCoord.normalize, stride, (const void*)layoutDesc.texCoord.offset);
		}
		if(layoutDesc.color.size)
		{
			glVertexAttribPointer(VATTR_COLOR, layoutDesc.color.size, asGLType(layoutDesc.color.type),
				layoutDesc.color.normalize, stride, (const void*)layoutDesc.color.offset);
		}
	});
}

void destroyGLVertexArrayRef(RendererTask &rTask, GLVertexArrayRef name)
{
	if(!rTask.renderer().support.hasVAOFuncs()) // name is actually a non-owning index buffer name
		return;
	rTask.run([&support = rTask.renderer().support, name]()
	{
		//log.debug("deleting vertex array object:0x{:X}", name);
		support.glDeleteVertexArrays(1, &name);
	});
}

}

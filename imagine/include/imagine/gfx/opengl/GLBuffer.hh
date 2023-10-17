#pragma once

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

#include <imagine/gfx/defs.hh>
#include <imagine/gfx/BufferConfig.hh>
#include <imagine/util/memory/UniqueResource.hh>
#include <imagine/util/memory/Buffer.hh>
#include <span>

namespace IG::Gfx
{

using GLBufferRef = GLuint;

void destroyGLBufferRef(RendererTask &, GLBufferRef);

struct GLBufferRefDeleter
{
	RendererTask *rTaskPtr{};

	void operator()(GLBufferRef s) const
	{
		destroyGLBufferRef(*rTaskPtr, s);
	}
};
using UniqueGLBufferRef = UniqueResource<GLBufferRef, GLBufferRefDeleter>;

template<BufferType type>
class GLBuffer
{
public:
	constexpr GLBuffer() = default;
	GLBuffer(RendererTask &, ByteBufferConfig);
	explicit operator bool() const { return bool(buffer); }
	RendererTask &task() const { return *buffer.get_deleter().rTaskPtr; }
	void setTask(RendererTask &task) { buffer.get_deleter().rTaskPtr = &task; }
	GLBufferRef name() const { return buffer.get(); }
	void reset(ByteBufferConfig);
	size_t sizeBytes() const { return sizeBytes_; }
	ByteBuffer map();
	void writeSubData(ssize_t offset, size_t size, const void *data);
	static void writeSubData(GLuint name, ssize_t offset, size_t size, const void *data);

protected:
	UniqueGLBufferRef buffer;
	size_t sizeBytes_{};
};

template<BufferType type>
using BufferImpl = GLBuffer<type>;

}

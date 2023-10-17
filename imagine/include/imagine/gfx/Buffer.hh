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

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLBuffer.hh>
#endif

#include <imagine/gfx/BufferConfig.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/memory/Buffer.hh>
#include <span>

namespace IG::Gfx
{

template<class T>
class MappedBuffer
{
public:
	constexpr MappedBuffer(ByteBuffer buff):
		buff{std::move(buff)} {}
	constexpr operator std::span<T>() const { return span(); }
	constexpr std::span<T> span() const { return {data(), size()}; }
	constexpr T *data() const { return reinterpret_cast<T*>(buff.data()); }
	constexpr size_t size() const { return buff.size() / sizeof(T); }
	constexpr T& operator[] (size_t idx) { return data()[idx]; }
	constexpr const T& operator[] (size_t idx) const { return data()[idx]; }
	constexpr explicit operator bool() const { return bool(buff); }
	constexpr auto begin() { return data(); }
	constexpr auto end() { return data() + size(); }
	constexpr auto begin() const { return data(); }
	constexpr auto end() const { return data() + size(); }

protected:
	ByteBuffer buff;
};

template<class T, BufferType type>
class Buffer: public BufferImpl<type>
{
public:
	using BaseBuffer = BufferImpl<type>;
	using Type = T;
	static constexpr size_t elemSize = sizeof(T);

	constexpr Buffer() = default;
	Buffer(RendererTask &rTask, BufferConfig<T> config): BaseBuffer(rTask, config.toByteConfig()) {}
	explicit operator bool() const { return BaseBuffer::operator bool(); }
	Renderer &renderer() const { return task().renderer(); }
	RendererTask &task() const { return BaseBuffer::task(); }
	void setTask(RendererTask &task) { return BaseBuffer::setTask(task); }
	void reset(BufferConfig<T> config) { BaseBuffer::reset(config.toByteConfig()); }
	size_t size() const { return BaseBuffer::sizeBytes() / elemSize; }
	MappedBuffer<T> map() { return {BaseBuffer::map()}; }
};

template<VertexLayout T>
using VertexBuffer = Buffer<T, BufferType::vertex>;

template<class T>
using IndexBuffer = Buffer<T, BufferType::index>;

}

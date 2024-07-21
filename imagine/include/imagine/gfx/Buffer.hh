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
	constexpr MappedBuffer() = default;
	constexpr MappedBuffer(MappedByteBuffer buff):
		buff{std::move(buff)} {}
	constexpr operator std::span<T>() const { return span(); }
	constexpr std::span<T> span() const { return {data(), size()}; }
	constexpr auto data(this auto&& self) { return reinterpret_cast<T*>(self.buff.data()); }
	constexpr size_t size() const { return buff.size() / sizeof(T); }
	constexpr auto& operator[] (this auto&& self, size_t idx) { return self.data()[idx]; }
	constexpr explicit operator bool() const { return bool(buff); }
	constexpr auto begin(this auto&& self) { return self.data(); }
	constexpr auto end(this auto&& self) { return self.data() + self.size(); }

protected:
	MappedByteBuffer buff;
};

template<class T, BufferType type>
class Buffer: public BufferImpl<type>
{
public:
	using Config = BufferConfig<T>;
	using BaseBuffer = BufferImpl<type>;
	using Type = T;
	static constexpr size_t elemSize = sizeof(T);

	Buffer() = default;
	Buffer(RendererTask &rTask, BufferConfig<T> config): BaseBuffer(rTask, config.toByteConfig()) {}
	explicit operator bool() const { return BaseBuffer::operator bool(); }
	Renderer &renderer() const { return task().renderer(); }
	RendererTask &task() const { return *BaseBuffer::taskPtr(); }
	bool hasTask() const { return BaseBuffer::taskPtr(); }
	void reset(BufferConfig<T> config) { BaseBuffer::reset(config.toByteConfig()); }
	size_t size() const { return BaseBuffer::sizeBytes() / elemSize; }
	MappedBuffer<T> map(ssize_t offset, size_t size, BufferMapMode mode = BufferMapMode::unset) { return {BaseBuffer::map(offset * elemSize, size * elemSize, mode)}; }
	MappedBuffer<T> map(BufferMapMode mode) { return map(0, 0, mode); }
	MappedBuffer<T> map() { return map(0, 0); }
};

template<VertexLayout T>
using VertexBuffer = Buffer<T, BufferType::vertex>;

template<class T>
using IndexBuffer = Buffer<T, BufferType::index>;

template<class T>
class ObjectBufferConfig
{
public:
	size_t size;
	BufferUsageHint usageHint{BufferUsageHint::dynamic};

	constexpr BufferConfig<typename T::Vertex> toBufferConfig() const
	{
		return
		{
			.size = size * T::vertexCount,
			.usageHint = usageHint,
		};
	}
};

template<class T>
class ObjectVertexBuffer : public VertexBuffer<typename T::Vertex>
{
public:
	using Type = T;
	using Vertex = T::Vertex;
	using Config = ObjectBufferConfig<T>;
	using BaseBuffer = VertexBuffer<Vertex>;
	using BaseBuffer::map;

	ObjectVertexBuffer() = default;
	ObjectVertexBuffer(RendererTask &rTask, ObjectBufferConfig<T> config): VertexBuffer<Vertex>{rTask, config.toBufferConfig()} {}
	void reset(ObjectBufferConfig<T> config) { VertexBuffer<Vertex>::reset(config.toBufferConfig()); }
	size_t size() const { return BaseBuffer::size() / T::vertexCount; }
	MappedBuffer<Vertex> map(ssize_t offset, size_t size, BufferMapMode mode = BufferMapMode::unset) { return BaseBuffer::map(offset * T::vertexCount, size * T::vertexCount, mode); }
	void write(ssize_t offset, T obj) { obj.write(*this, offset); }
	void write(ssize_t offset, T::InitParams params) { write(offset, T{params}); }
};

class VertexArray: public VertexArrayImpl
{
public:
	VertexArray() = default;
	template<class V>
	VertexArray(RendererTask &rTask, const VertexBuffer<V> &buff, NativeBuffer idxs): VertexArrayImpl(rTask, buff, idxs) {}
};

template<class T>
struct VertexOf { using type = typename T::Vertex; };

template<VertexLayout T>
struct VertexOf<T> { using type = T; };

template<class T>
class ObjectVertexArray : public std::conditional_t<VertexLayout<T>, Buffer<T, BufferType::vertex>, ObjectVertexBuffer<T>>
{
public:
	using Base = std::conditional_t<VertexLayout<T>, Buffer<T, BufferType::vertex>, ObjectVertexBuffer<T>>;
	using Vertex = VertexOf<T>::type;

	ObjectVertexArray() = default;

	ObjectVertexArray(RendererTask &rTask, Base::Config config, NativeBuffer idxsRef = {}):
		Base{rTask, config},
		vao{rTask, *this, idxsRef} {}

	template<class I>
	ObjectVertexArray(RendererTask &rTask, Base::Config config, const IndexBuffer<I> &idxs):
		ObjectVertexArray{rTask, config, idxs.name()} {}

	const VertexArray &array() const { return vao; }
	operator const VertexArray&() const { return vao; }

private:
	VertexArray vao;
};

}

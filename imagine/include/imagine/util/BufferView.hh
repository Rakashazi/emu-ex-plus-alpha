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

#include <memory>

namespace IG
{

template<class T>
class BaseBufferView
{
public:
	BaseBufferView() {}
	BaseBufferView(T *data, size_t size, void(*deleter)(T*)):
		data_{data, deleter}, size_{size} {}

	T *data()
	{
		return data_.get();
	}

	size_t size() const
	{
		return size_;
	}

	explicit operator bool() const
	{
		return data_.get();
	}

protected:
	std::unique_ptr<T[], void(*)(T*)> data_{nullptr, [](T*){}};
	size_t size_ = 0;
};

using BufferView = BaseBufferView<char>;
using ConstBufferView = BaseBufferView<const char>;

}

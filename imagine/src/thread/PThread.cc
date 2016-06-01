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

#include <imagine/logger/logger.h>
#include <imagine/thread/Thread.hh>
#include <assert.h>

namespace IG
{

thread::thread() {}

thread::~thread()
{
	assert(!joinable());
}

thread::thread(thread&& other)
{
	assert(!joinable());
	id_ = other.id_;
	other.id_ = {};
}

bool thread::joinable() const
{
	return get_id() != thread::id{};
}

thread::id thread::get_id() const
{
	return id_;
}

void thread::join()
{
	assert(joinable());
	pthread_join(id_, nullptr);
	id_ = {};
}

void thread::detach()
{
	assert(joinable());
	pthread_detach(id_);
	id_ = {};
}

	namespace this_thread
	{

	thread::id get_id()
	{
		return pthread_self();
	}

	}

}

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

#include <imagine/thread/Semaphore.hh>

namespace IG
{

Semaphore::Semaphore(unsigned int startValue)
{
	sem_init(&sem, 0, startValue);
}

Semaphore::Semaphore(Semaphore &&o)
{
	sem = o.sem;
	o.sem = {};
}

Semaphore &Semaphore::operator=(Semaphore &&o)
{
	deinit();
	sem = o.sem;
	o.sem = {};
	return *this;
}

Semaphore::~Semaphore()
{
	deinit();
}

void Semaphore::wait()
{
	sem_wait(&sem);
}

void Semaphore::notify()
{
	sem_post(&sem);
}

void PosixSemaphore::deinit()
{
	sem_destroy(&sem);
}

}

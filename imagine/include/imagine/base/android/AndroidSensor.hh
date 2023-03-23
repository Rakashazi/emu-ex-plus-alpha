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

#include <imagine/base/baseDefs.hh>
#include <memory>

struct ASensorEventQueue;
struct ASensorManager;
struct ASensor;

namespace IG
{

void destroyASensorEventQueue(ASensorEventQueue *);

struct ASensorEventQueueDeleter
{
	void operator()(ASensorEventQueue *queue) const
	{
		destroyASensorEventQueue(queue);
	}
};
using UniqueASensorEventQueue = std::unique_ptr<ASensorEventQueue, ASensorEventQueueDeleter>;

class AndroidSensorListener
{
public:
	constexpr AndroidSensorListener() = default;
	AndroidSensorListener(ASensorManager *, SensorType, SensorChangedDelegate);

protected:
	struct ControlBlock
	{
		UniqueASensorEventQueue queue{};
		SensorChangedDelegate del{};
	};
	std::unique_ptr<ControlBlock> ctrl;
};

using SensorListenerImpl = AndroidSensorListener;

}

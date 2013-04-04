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

#define thisModuleName "input:ps3"
#include <input/common/common.h>
#include <base/Base.hh>
#include <cell/pad.h>
#include <cell/sysmodule.h>

namespace Input
{

static const PackedInputAccess padDataAccess[] =
{
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_SELECT, PS3::SELECT },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_L3, PS3::L3 },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_R3, PS3::R3 },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_START, PS3::START },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_UP, PS3::UP },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_RIGHT, PS3::RIGHT },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_DOWN, PS3::DOWN },
	{ CELL_PAD_BTN_OFFSET_DIGITAL1, CELL_PAD_CTRL_LEFT, PS3::LEFT },

	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_L2, PS3::L2 },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_R2, PS3::R2 },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_L1, PS3::L1 },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_R1, PS3::R1 },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_TRIANGLE, PS3::TRIANGLE },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_CIRCLE, PS3::CIRCLE },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_CROSS, PS3::CROSS },
	{ CELL_PAD_BTN_OFFSET_DIGITAL2, CELL_PAD_CTRL_SQUARE, PS3::SQUARE },
};

static const uint numPads = 5;
static uint padStatus[numPads] = { 0 };
static CellPadData padData[numPads] = { { 0, { 0 } } };

void update()
{
	CellPadInfo2 padInfo;
	cellPadGetInfo2(&padInfo);

	iterateTimes(sizeofArray(padStatus), i)
	{
		if(padInfo.port_status[i] != 0 && padStatus[i] == 0)
		{
			//logMsg("pad connection %d vendor:%d product:%d", i, padInfo.vendor_id[i], padInfo.product_id[i]);
			logMsg("pad connection %d type:%d capability:%d", i, padInfo.device_type[i], padInfo.device_capability[i]);
			mem_zero(padData[i]);
			addDevice(Device{i, Event::MAP_PS3PAD, Device::TYPE_BIT_GAMEPAD, "PS3 Controller"});
		}
		padStatus[i] = padInfo.port_status[i];

		CellPadData data = { 0, { 0 } };
		cellPadGetData(i, &data);
		if(data.len != 0)
		{
			//logMsg("%d bytes from port", data.len, i);
			forEachInArray(padDataAccess, e)
			{
				bool oldState = padData[i].button[e->byteOffset] & e->mask,
					newState = data.button[e->byteOffset] & e->mask;
				if(oldState != newState)
				{
					//logMsg("%d %s %s", i, buttonName(InputEvent::DEV_PS3PAD, e->keyEvent), newState ? "pushed" : "released");
					Input::onInputEvent(Event(i, Event::MAP_PS3PAD, e->keyEvent, newState ? PUSHED : RELEASED, 0, devList.first()));
				}
			}
			memcpy(&padData[i], &data, sizeof(CellPadData));
		}
	}
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
	if(typeBits & TYPE_BIT_GAMEPAD)
	{
		return true;
	}
	return false;
}

void setKeyRepeat(bool on)
{
	// TODO
}

CallResult init()
{
	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);

	int ret = cellPadInit(numPads);
	if(ret != CELL_OK && ret != CELL_PAD_ERROR_ALREADY_INITIALIZED)
	{
		logErr("error in cellPadInit");
		Base::exit();
	}

	return OK;
}

}

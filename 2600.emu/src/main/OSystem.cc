/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <OSystem.hxx>
#include <FrameBuffer.hxx>
#include <EventHandler.hxx>
#include <stella/emucore/Console.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Random.hxx>
#include <stella/emucore/SerialPort.hxx>
#include <stella/emucore/Settings.hxx>
#include "SoundGeneric.hh"
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include <emuframework/EmuSystem.hh>

OSystem osystem{};
static Random myRandom{osystem};
static EventHandler myEventHandler{osystem};
static SerialPort mySerialPort{};
static FrameBuffer myFrameBuffer{};
static PropertiesSet myPropSet{""};
static Settings mySettings{osystem};
static SoundGeneric mySound{osystem};

EventHandler& OSystem::eventHandler() const
{
	return myEventHandler;
}

Random& OSystem::random() const
{
	return myRandom;
}

FrameBuffer& OSystem::frameBuffer() const
{
	return myFrameBuffer;
}

Sound& OSystem::sound() const
{
	return mySound;
}

SoundGeneric& OSystem::soundGeneric() const
{
	return mySound;
}

Settings& OSystem::settings() const
{
	return mySettings;
}

PropertiesSet& OSystem::propSet() const
{
	return myPropSet;
}

SerialPort& OSystem::serialPort() const
{
	return mySerialPort;
}

void OSystem::makeConsole(unique_ptr<Cartridge>& cart, const Properties& props)
{
	myConsole = std::make_unique<Console>(*this, cart, props);
}

void OSystem::deleteConsole()
{
	myConsole = {};
}

std::string OSystem::stateDir() const
{
	return EmuSystem::savePath();
}

std::string OSystem::nvramDir() const
{
	return EmuSystem::savePath();
}

void OSystem::logMessage(const string& message, uInt8 level)
{
	if(Config::DEBUG_BUILD)
		logMsg("%s", message.c_str());
}

uInt64 OSystem::getTicks() const
{
	return IG::Time::now().uSecs();
}

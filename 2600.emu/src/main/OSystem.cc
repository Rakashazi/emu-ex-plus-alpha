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
#include <FSNodeEmuEx.hh>
#include <SoundEmuEx.hh>
#include <stella/emucore/Console.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Random.hxx>
#include <stella/emucore/Settings.hxx>
#include <stella/common/StateManager.hxx>
#include <stella/common/AudioSettings.hxx>
#include <stella/common/TimerManager.hxx>
#include <stella/emucore/M6532.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <imagine/logger/logger.h>
#include <emuframework/EmuSystem.hh>
#undef Debugger

OSystem::OSystem(EmuApp &app):
	appPtr{&app}
{
	mySettings = std::make_unique<Settings>();
	mySettings->setValue(AudioSettings::SETTING_PRESET, static_cast<int>(AudioSettings::Preset::custom));
	mySettings->setValue(AudioSettings::SETTING_FRAGMENT_SIZE, 128);
	mySettings->setValue(AudioSettings::SETTING_BUFFER_SIZE, 0);
	mySettings->setValue(AudioSettings::SETTING_HEADROOM, 0);
	mySettings->setValue(AudioSettings::SETTING_VOLUME, 100);
	myAudioSettings = std::make_unique<AudioSettings>(*mySettings);
	myRandom = std::make_unique<Random>(uInt32(TimerManager::getTicks()));
	myFrameBuffer = std::make_unique<FrameBuffer>(*this);
	myEventHandler = std::make_unique<EventHandler>(*this);
	myPropSet = std::make_unique<PropertiesSet>();
	myStateManager = std::make_unique<StateManager>(*this);
	mySound = std::make_unique<SoundEmuEx>(*this);
}

EventHandler& OSystem::eventHandler() const
{
	return *myEventHandler;
}

Random& OSystem::random() const
{
	return *myRandom;
}

FrameBuffer& OSystem::frameBuffer() const
{
	return *myFrameBuffer;
}

Sound& OSystem::sound() const
{
	return *mySound;
}

Settings& OSystem::settings() const
{
	return *mySettings;
}

PropertiesSet& OSystem::propSet() const
{
	return *myPropSet;
}

StateManager& OSystem::state() const
{
	return *myStateManager;
}

void OSystem::makeConsole(unique_ptr<Cartridge>& cart, const Properties& props, const char *gamePath)
{
	myConsole = std::make_unique<Console>(*this, cart, props, *myAudioSettings);
	myRomFile = FilesystemNode{gamePath};
	myConsole->riot().update();
}

void OSystem::deleteConsole()
{
	myConsole = {};
}

void OSystem::setFrameTime(double frameTime, int rate)
{
	mySound->setFrameTime(*this, frameTime, rate);
}

void OSystem::setResampleQuality(AudioSettings::ResamplingQuality quality)
{
	mySound->setResampleQuality(quality);
}

void OSystem::processAudio(EmuAudio *audio)
{
	mySound->processAudio(*this, audio);
}

std::string OSystem::stateDir() const
{
	return EmuSystem::savePath();
}

std::string OSystem::nvramDir() const
{
	return EmuSystem::savePath();
}

EmuApp &OSystem::app()
{
	return *appPtr;
}

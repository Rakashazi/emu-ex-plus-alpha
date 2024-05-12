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
#include <FSNode.hxx>
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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#undef Debugger

OSystem::OSystem(EmuEx::EmuApp &app):
	appPtr{&app},
	myRandom{uInt32(TimerManager::getTicks())}
{
	mySettings.setValue(AudioSettings::SETTING_PRESET, static_cast<int>(AudioSettings::Preset::custom));
	mySettings.setValue(AudioSettings::SETTING_FRAGMENT_SIZE, 128);
	mySettings.setValue(AudioSettings::SETTING_BUFFER_SIZE, 0);
	mySettings.setValue(AudioSettings::SETTING_HEADROOM, 0);
	mySettings.setValue(AudioSettings::SETTING_VOLUME, 100);
}

void OSystem::makeConsole(unique_ptr<Cartridge>& cart, const Properties& props, const char *gamePath)
{
	myRomFile = FilesystemNode{gamePath};
	myConsole.emplace(*this, cart, props, myAudioSettings);
	myConsole->riot().update();
}

void OSystem::deleteConsole()
{
	myConsole.reset();
}

void OSystem::setSoundMixRate(int mixRate, AudioSettings::ResamplingQuality resampleQ)
{
	mySound.setMixRate(mixRate, resampleQ);
}

FilesystemNode OSystem::stateDir() const
{
	return FilesystemNode{std::string{appPtr->system().contentSaveDirectory()}};
}

FilesystemNode OSystem::nvramDir(std::string_view name) const
{
	return FilesystemNode{std::string{appPtr->system().contentSaveFilePath(name)}};
}

EmuEx::EmuApp &OSystem::app()
{
	return *appPtr;
}

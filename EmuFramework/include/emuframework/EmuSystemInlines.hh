#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuVideo.hh>
#include <main/MainSystem.hh>
#include <imagine/io/IO.hh>

namespace EmuEx
{

// TODO: Make EmuSystem a template class instead of using static dispatching

FS::FileString EmuSystem::configName() const
{
	if(&MainSystem::configName != &EmuSystem::configName)
		return static_cast<const MainSystem*>(this)->configName();
	return {};
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name) const
{
	return static_cast<const MainSystem*>(this)->stateFilename(slot, name);
}

std::string_view EmuSystem::stateFilenameExt() const
{
	return static_cast<const MainSystem*>(this)->stateFilenameExt();
}

void EmuSystem::onOptionsLoaded()
{
	if(&MainSystem::onOptionsLoaded != &EmuSystem::onOptionsLoaded)
		static_cast<MainSystem*>(this)->onOptionsLoaded();
}

void EmuSystem::loadBackupMemory(EmuApp &app)
{
	if(&MainSystem::loadBackupMemory != &EmuSystem::loadBackupMemory)
		static_cast<MainSystem*>(this)->loadBackupMemory(app);
}

void EmuSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags flags)
{
	if(&MainSystem::onFlushBackupMemory != &EmuSystem::onFlushBackupMemory)
		static_cast<MainSystem*>(this)->onFlushBackupMemory(app, flags);
}

WallClockTimePoint EmuSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	if(&MainSystem::backupMemoryLastWriteTime != &EmuSystem::backupMemoryLastWriteTime)
		return static_cast<const MainSystem*>(this)->backupMemoryLastWriteTime(app);
	return {};
}

bool EmuSystem::usesBackupMemory() const
{
	return &MainSystem::loadBackupMemory != &EmuSystem::loadBackupMemory;
}

void EmuSystem::savePathChanged()
{
	if(&MainSystem::savePathChanged != &EmuSystem::savePathChanged)
		static_cast<MainSystem*>(this)->savePathChanged();
}

WSize EmuSystem::multiresVideoBaseSize() const
{
	if(&MainSystem::multiresVideoBaseSize != &EmuSystem::multiresVideoBaseSize)
		return static_cast<const MainSystem*>(this)->multiresVideoBaseSize();
	return {};
}

double EmuSystem::videoAspectRatioScale() const
{
	if(&MainSystem::videoAspectRatioScale != &EmuSystem::videoAspectRatioScale)
		return static_cast<const MainSystem*>(this)->videoAspectRatioScale();
	return 1.;
}

VideoSystem EmuSystem::videoSystem() const
{
	if(&MainSystem::videoSystem != &EmuSystem::videoSystem)
		return static_cast<const MainSystem*>(this)->videoSystem();
	return VideoSystem::NATIVE_NTSC;
}

bool EmuSystem::onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState now, IG::WindowRect rect)
{
	if(&MainSystem::onPointerInputStart != &EmuSystem::onPointerInputStart)
		return static_cast<MainSystem*>(this)->onPointerInputStart(e, now, rect);
	return {};
}

bool EmuSystem::onPointerInputUpdate(const Input::MotionEvent &e, Input::DragTrackerState now, Input::DragTrackerState previous, IG::WindowRect rect)
{
	if(&MainSystem::onPointerInputUpdate != &EmuSystem::onPointerInputUpdate)
		return static_cast<MainSystem*>(this)->onPointerInputUpdate(e, now, previous, rect);
	return {};
}

bool EmuSystem::onPointerInputEnd(const Input::MotionEvent &e, Input::DragTrackerState now, IG::WindowRect rect)
{
	if(&MainSystem::onPointerInputEnd != &EmuSystem::onPointerInputEnd)
		return static_cast<MainSystem*>(this)->onPointerInputEnd(e, now, rect);
	return {};
}

bool EmuSystem::onVideoRenderFormatChange(EmuVideo &video, IG::PixelFormat fmt)
{
	if(&MainSystem::onVideoRenderFormatChange != &EmuSystem::onVideoRenderFormatChange)
		return static_cast<MainSystem*>(this)->onVideoRenderFormatChange(video, fmt);
	return false;
}

FS::FileString EmuSystem::contentDisplayNameForPath(CStringView path) const
{
	if(&MainSystem::contentDisplayNameForPath != &EmuSystem::contentDisplayNameForPath)
		return static_cast<const MainSystem*>(this)->contentDisplayNameForPath(path);
	return contentDisplayNameForPathDefaultImpl(path);
}

bool EmuSystem::shouldFastForward() const
{
	if(&MainSystem::shouldFastForward != &EmuSystem::shouldFastForward)
		return static_cast<const MainSystem*>(this)->shouldFastForward();
	return {};
}

void EmuSystem::writeConfig(ConfigType type, FileIO &io)
{
	static_cast<MainSystem*>(this)->writeConfig(type, io);
}

bool EmuSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	return static_cast<MainSystem*>(this)->readConfig(type, io, key);
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	if(&MainSystem::resetSessionOptions != &EmuSystem::resetSessionOptions)
		return static_cast<MainSystem*>(this)->resetSessionOptions(app);
	return {};
}

void EmuSystem::onSessionOptionsLoaded(EmuApp &app)
{
	if(&MainSystem::onSessionOptionsLoaded != &EmuSystem::onSessionOptionsLoaded)
		static_cast<MainSystem*>(this)->onSessionOptionsLoaded(app);
}

void EmuSystem::reset(EmuApp &app, ResetMode mode)
{
	static_cast<MainSystem*>(this)->reset(app, mode);
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	if(&MainSystem::renderFramebuffer != &EmuSystem::renderFramebuffer)
		static_cast<MainSystem*>(this)->renderFramebuffer(video);
	else
		video.clear();
}

void EmuSystem::handleInputAction(EmuApp *app, InputAction action)
{
	static_cast<MainSystem*>(this)->handleInputAction(app, action);
}

void EmuSystem::onVKeyboardShown(VControllerKeyboard &kb, bool shown)
{
	if(&MainSystem::onVKeyboardShown != &EmuSystem::onVKeyboardShown)
		static_cast<MainSystem*>(this)->onVKeyboardShown(kb, shown);
}

void EmuSystem::closeSystem()
{
	if(&MainSystem::closeSystem != &EmuSystem::closeSystem)
		static_cast<MainSystem*>(this)->closeSystem();
}

void EmuSystem::runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio)
{
	static_cast<MainSystem*>(this)->runFrame(task, video, audio);
}

size_t EmuSystem::stateSize()
{
	if(&MainSystem::stateSize != &EmuSystem::stateSize)
		return static_cast<MainSystem*>(this)->stateSize();
	return 0;
}

void EmuSystem::readState(EmuApp &app, std::span<uint8_t> buff)
{
	if(&MainSystem::readState != &EmuSystem::readState)
		static_cast<MainSystem*>(this)->readState(app, buff);
}

size_t EmuSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	if(&MainSystem::writeState != &EmuSystem::writeState)
		return static_cast<MainSystem*>(this)->writeState(buff, flags);
}

void EmuSystem::clearInputBuffers(EmuInputView &view)
{
	static_cast<MainSystem*>(this)->clearInputBuffers(view);
}

FrameTime EmuSystem::frameTime() const
{
	return static_cast<const MainSystem*>(this)->frameTime();
}

void EmuSystem::configAudioRate(FrameTime frameTime, int rate)
{
	static_cast<MainSystem*>(this)->configAudioRate(frameTime, rate);
}

std::span<const AspectRatioInfo> EmuSystem::aspectRatioInfos()
{
	return MainSystem::aspectRatioInfos();
}

SystemInputDeviceDesc EmuSystem::inputDeviceDesc(int idx) const
{
	return static_cast<const MainSystem*>(this)->inputDeviceDesc(idx);
}

void EmuSystem::loadContent(IO &io, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadDel)
{
	static_cast<MainSystem*>(this)->loadContent(io, params, onLoadDel);
}

VController::KbMap EmuSystem::vControllerKeyboardMap(VControllerKbMode mode)
{
	if(&MainSystem::vControllerKeyboardMap != &EmuSystem::vControllerKeyboardMap)
		return static_cast<MainSystem*>(this)->vControllerKeyboardMap(mode);
	return {};
}

IG::Rotation EmuSystem::contentRotation() const
{
	if(&MainSystem::contentRotation != &EmuSystem::contentRotation)
		return static_cast<const MainSystem*>(this)->contentRotation();
	return {};
}

void EmuSystem::onStart()
{
	if(&MainSystem::onStart != &EmuSystem::onStart)
		static_cast<MainSystem*>(this)->onStart();
}

void EmuSystem::onStop()
{
	if(&MainSystem::onStop != &EmuSystem::onStop)
		static_cast<MainSystem*>(this)->onStop();
}

void EmuSystem::addThreadGroupIds(std::vector<ThreadId> &ids) const
{
	if(&MainSystem::addThreadGroupIds != &EmuSystem::addThreadGroupIds)
		static_cast<const MainSystem*>(this)->addThreadGroupIds(ids);
}

Cheat* EmuSystem::newCheat(EmuApp& app, const char* name, CheatCodeDesc desc)
{
	if(&MainSystem::newCheat != &EmuSystem::newCheat)
		return static_cast<MainSystem*>(this)->newCheat(app, name, desc);
	return {};
}

bool EmuSystem::setCheatName(Cheat& c, const char* name)
{
	if(&MainSystem::setCheatName != &EmuSystem::setCheatName)
		return static_cast<MainSystem*>(this)->setCheatName(c, name);
	return false;
}

std::string_view EmuSystem::cheatName(const Cheat& c) const
{
	if(&MainSystem::cheatName != &EmuSystem::cheatName)
		return static_cast<const MainSystem*>(this)->cheatName(c);
	return {};
}

void EmuSystem::setCheatEnabled(Cheat& c, bool on)
{
	if(&MainSystem::setCheatEnabled != &EmuSystem::setCheatEnabled)
		static_cast<MainSystem*>(this)->setCheatEnabled(c, on);
}

bool EmuSystem::isCheatEnabled(const Cheat& c) const
{
	if(&MainSystem::isCheatEnabled != &EmuSystem::isCheatEnabled)
		return static_cast<const MainSystem*>(this)->isCheatEnabled(c);
	return false;
}

bool EmuSystem::addCheatCode(EmuApp& app, Cheat*& cPtr, CheatCodeDesc desc)
{
	if(&MainSystem::addCheatCode != &EmuSystem::addCheatCode)
		return static_cast<MainSystem*>(this)->addCheatCode(app, cPtr, desc);
	return false;
}

bool EmuSystem::modifyCheatCode(EmuApp& app, Cheat& c, CheatCode& code, CheatCodeDesc desc)
{
	if(&MainSystem::modifyCheatCode != &EmuSystem::modifyCheatCode)
		return static_cast<MainSystem*>(this)->modifyCheatCode(app, c, code, desc);
	return false;
}

Cheat* EmuSystem::removeCheatCode(Cheat& c, CheatCode& code)
{
	if(&MainSystem::removeCheatCode != &EmuSystem::removeCheatCode)
		return static_cast<MainSystem*>(this)->removeCheatCode(c, code);
	return {};
}

bool EmuSystem::removeCheat(Cheat& c)
{
	if(&MainSystem::removeCheat != &EmuSystem::removeCheat)
		return static_cast<MainSystem*>(this)->removeCheat(c);
	return false;
}

void EmuSystem::forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)> del)
{
	if(&MainSystem::forEachCheat != &EmuSystem::forEachCheat)
		static_cast<MainSystem*>(this)->forEachCheat(del);
}

void EmuSystem::forEachCheatCode(Cheat& c, DelegateFunc<bool(CheatCode&, std::string_view)> del)
{
	if(&MainSystem::forEachCheatCode != &EmuSystem::forEachCheatCode)
		static_cast<MainSystem*>(this)->forEachCheatCode(c, del);
}

}

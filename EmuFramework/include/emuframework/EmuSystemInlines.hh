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

void EmuSystem::onOptionsLoaded()
{
	if(&MainSystem::onOptionsLoaded != &EmuSystem::onOptionsLoaded)
		static_cast<MainSystem*>(this)->onOptionsLoaded();
}

void EmuSystem::onFlushBackupMemory(BackupMemoryDirtyFlags flags)
{
	if(&MainSystem::onFlushBackupMemory != &EmuSystem::onFlushBackupMemory)
		static_cast<MainSystem*>(this)->onFlushBackupMemory(flags);
}

void EmuSystem::savePathChanged()
{
	if(&MainSystem::savePathChanged != &EmuSystem::savePathChanged)
		static_cast<MainSystem*>(this)->savePathChanged();
}

WP EmuSystem::multiresVideoBaseSize() const
{
	if(&MainSystem::multiresVideoBaseSize != &EmuSystem::multiresVideoBaseSize)
		return static_cast<const MainSystem*>(this)->multiresVideoBaseSize();
	return {};
}

double EmuSystem::videoAspectRatioScale() const
{
	if(&MainSystem::videoAspectRatioScale != &EmuSystem::videoAspectRatioScale)
		return static_cast<const MainSystem*>(this)->videoAspectRatioScale();
	return {};
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

FS::FileString EmuSystem::contentDisplayNameForPath(IG::CStringView path) const
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

void EmuSystem::writeConfig(ConfigType type, IO &io)
{
	static_cast<MainSystem*>(this)->writeConfig(type, io);
}

bool EmuSystem::readConfig(ConfigType type, IO &io, unsigned key, size_t readSize)
{
	return static_cast<MainSystem*>(this)->readConfig(type, io, key, readSize);
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

void EmuSystem::loadState(EmuApp &app, IG::CStringView uri)
{
	static_cast<MainSystem*>(this)->loadState(app, uri);
}

void EmuSystem::saveState(IG::CStringView uri)
{
	static_cast<MainSystem*>(this)->saveState(uri);
}

void EmuSystem::clearInputBuffers(EmuInputView &view)
{
	static_cast<MainSystem*>(this)->clearInputBuffers(view);
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	return static_cast<MainSystem*>(this)->translateInputAction(input, turbo);
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, int rate)
{
	static_cast<MainSystem*>(this)->configAudioRate(frameTime, rate);
}

void EmuSystem::loadContent(IO &io, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadDel)
{
	static_cast<MainSystem*>(this)->loadContent(io, params, onLoadDel);
}

VController::Map EmuSystem::vControllerMap(int player)
{
	return static_cast<MainSystem*>(this)->vControllerMap(player);
}

VController::KbMap EmuSystem::vControllerKeyboardMap(unsigned mode)
{
	if(&MainSystem::vControllerKeyboardMap != &EmuSystem::vControllerKeyboardMap)
		return static_cast<MainSystem*>(this)->vControllerKeyboardMap(mode);
	return {};
}

}
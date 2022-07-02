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

#define LOGTAG "EmuSystem"
#include <emuframework/EmuSystem.hh>
#include "EmuOptions.hh"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/string.h>
#include <algorithm>
#include <cstring>
#include "pathUtils.hh"

namespace EmuEx
{

[[gnu::weak]] bool EmuSystem::inputHasKeyboard = false;
[[gnu::weak]] bool EmuSystem::inputHasShortBtnTexture = false;
[[gnu::weak]] int EmuSystem::inputLTriggerIndex = -1;
[[gnu::weak]] int EmuSystem::inputRTriggerIndex = -1;
[[gnu::weak]] bool EmuSystem::hasBundledGames = false;
[[gnu::weak]] bool EmuSystem::hasPALVideoSystem = false;
[[gnu::weak]] bool EmuSystem::canRenderRGB565 = true;
[[gnu::weak]] bool EmuSystem::canRenderRGBA8888 = true;
[[gnu::weak]] bool EmuSystem::hasResetModes = false;
[[gnu::weak]] bool EmuSystem::handlesArchiveFiles = false;
[[gnu::weak]] bool EmuSystem::handlesGenericIO = true;
[[gnu::weak]] bool EmuSystem::hasCheats = false;
[[gnu::weak]] bool EmuSystem::hasSound = true;
[[gnu::weak]] int EmuSystem::forcedSoundRate = 0;
[[gnu::weak]] IG::Audio::SampleFormat EmuSystem::audioSampleFormat = IG::Audio::SampleFormats::i16;
[[gnu::weak]] bool EmuSystem::constFrameRate = false;
[[gnu::weak]] std::array<int, EmuSystem::MAX_FACE_BTNS> EmuSystem::vControllerImageMap{0, 1, 2, 3, 4, 5, 6, 7};

bool EmuSystem::stateExists(int slot) const
{
	return appContext().fileUriExists(statePath(slot));
}

std::string_view EmuSystem::stateSlotName(int slot)
{
	assert(slot >= -1 && slot < 10);
	static constexpr std::string_view str[]{"Auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	return str[slot+1];
}

bool EmuApp::shouldOverwriteExistingState() const
{
	return !optionConfirmOverwriteState || !system().stateExists(system().stateSlot());
}

EmuFrameTimeInfo EmuSystem::advanceFramesWithTime(IG::FrameTime time)
{
	return emuTiming.advanceFramesWithTime(time);
}

void EmuSystem::setSpeedMultiplier(EmuAudio &emuAudio, int8_t speed)
{
	emuTiming.setSpeedMultiplier(speed);
	emuAudio.setSpeedMultiplier(speed);
}

void EmuSystem::setupContentUriPaths(IG::CStringView uri, std::string_view displayName)
{
	contentFileName_ = displayName;
	contentName_ = IG::stringWithoutDotExtension(contentFileName_);
	contentLocation_ = uri;
	contentDirectory_ = FS::dirnameUri(uri);
	updateContentSaveDirectory();
}

void EmuSystem::setupContentFilePaths(IG::CStringView filePath, std::string_view displayName)
{
	contentFileName_ = displayName;
	contentName_ = IG::stringWithoutDotExtension(contentFileName_);
	// find the realpath of the dirname portion separately in case the file is a symlink
	auto fileDir = FS::dirname(filePath);
	if(fileDir == ".")
	{
		// non-absolute path like bundled content name, don't set a content directory
		contentLocation_ = filePath;
	}
	else if(FS::PathStringArray realPath;
		!realpath(fileDir.data(), realPath.data()))
	{
		logErr("error in realpath() while setting content directory");
		contentDirectory_ = fileDir;
		contentLocation_ = filePath;
	}
	else
	{
		contentDirectory_ = realPath.data();
		contentLocation_ = FS::pathString(contentDirectory_, contentFileName_);
		logMsg("set content directory:%s", contentDirectory_.data());
	}
	updateContentSaveDirectory();
}

void EmuSystem::updateContentSaveDirectory()
{
	if(contentName_.empty())
		return;
	if(userSaveDirectory_.size())
	{
		if(userSaveDirectory_ == optionSavePathDefaultToken)
			contentSaveDirectory_ = fallbackSaveDirectory(true);
		else
			contentSaveDirectory_ = userSaveDirectory_;
	}
	else
	{
		if(contentDirectory_.size())
			contentSaveDirectory_ = contentDirectory_;
		else
			contentSaveDirectory_ = fallbackSaveDirectory(true);
	}
	logMsg("updated content save path:%s", contentSaveDirectory_.data());
}

FS::PathString EmuSystem::fallbackSaveDirectory(bool create)
{
	try
	{
		if(create)
			return FS::createDirectorySegments(appContext().storagePath(), "EmuEx", shortSystemName(), "saves");
		else
			return FS::pathString(appContext().storagePath(), "EmuEx", shortSystemName(), "saves");
	}
	catch(...)
	{
		logErr("error making fallback save dir");
		return {};
	}
}

void EmuSystem::clearGamePaths()
{
	contentName_ = {};
	contentDisplayName_ = {};
	contentFileName_ = {};
	contentDirectory_ = {};
	contentLocation_ = {};
	contentSaveDirectory_ = {};
}

FS::PathString EmuSystem::contentSaveDirectory() const
{
	assert(!contentName_.empty());
	return contentSaveDirectory_;
}

FS::PathString EmuSystem::contentSavePath(std::string_view name) const
{
	return FS::uriString(contentSaveDirectory(), name);
}

FS::PathString EmuSystem::contentSaveFilePath(std::string_view ext) const
{
	return FS::uriString(contentSaveDirectory(), contentName().append(ext));
}

FS::PathString EmuSystem::userSaveDirectory() const
{
	return userSaveDirectory_;
}

void EmuSystem::setUserSaveDirectory(IG::CStringView path)
{
	logMsg("set user save path:%s", path.data());
	userSaveDirectory_ = path;
	updateContentSaveDirectory();
	savePathChanged();
}

FS::PathString EmuSystem::firmwarePath() const
{
	return firmwarePath_;
}

void EmuSystem::setFirmwarePath(std::string_view path)
{
	firmwarePath_ = path;
}

FS::PathString EmuSystem::statePath(std::string_view filename, std::string_view basePath) const
{
	return FS::uriString(basePath, filename);
}

FS::PathString EmuSystem::statePath(std::string_view filename) const
{
	return statePath(filename, contentSaveDirectory());
}

FS::PathString EmuSystem::statePath(int slot, std::string_view path) const
{
	return statePath(stateFilename(slot), path);
}

FS::PathString EmuSystem::statePath(int slot) const
{
	return statePath(slot, contentSaveDirectory());
}

void EmuSystem::closeRuntimeSystem(EmuApp &app, bool allowAutosaveState)
{
	if(hasContent())
	{
		app.video().clear();
		app.audio().flush();
		if(allowAutosaveState)
			app.saveAutoState();
		app.saveSessionOptions();
		logMsg("closing game:%s", contentName_.data());
		flushBackupMemory();
		closeSystem();
		app.cancelAutoSaveStateTimer();
		state = State::OFF;
	}
	clearGamePaths();
}

bool EmuSystem::hasContent() const
{
	return contentName_.size();
}

void EmuSystem::resetFrameTime()
{
	emuTiming.reset();
}

void EmuSystem::pause(EmuApp &app)
{
	if(isActive())
		state = State::PAUSED;
	app.audio().stop();
	app.cancelAutoSaveStateTimer();
}

void EmuSystem::start(EmuApp &app)
{
	state = State::ACTIVE;
	if(inputHasKeyboard)
		app.defaultVController().keyboard().setShiftActive(false);
	clearInputBuffers(app.viewController().inputView());
	resetFrameTime();
	app.startAudio();
	app.startAutoSaveStateTimer();
}

IG::Time EmuSystem::benchmark(EmuVideo &video)
{
	auto now = IG::steadyClockTimestamp();
	iterateTimes(180, i)
	{
		runFrame({}, &video, nullptr);
	}
	auto after = IG::steadyClockTimestamp();
	return after-now;
}

void EmuSystem::configFrameTime(int rate)
{
	auto fTime = frameTime();
	configAudioRate(fTime, rate);
	audioFramesPerVideoFrame = std::ceil(rate * fTime.count());
	audioFramesPerVideoFrameFloat = (double)rate * fTime.count();
	currentAudioFramesPerVideoFrame = audioFramesPerVideoFrameFloat;
	emuTiming.setFrameTime(fTime);
}

void EmuSystem::configAudioPlayback(EmuAudio &emuAudio, int rate)
{
	configFrameTime(rate);
	emuAudio.setRate(rate);
}

int EmuSystem::updateAudioFramesPerVideoFrame()
{
	assumeExpr(currentAudioFramesPerVideoFrame < audioFramesPerVideoFrameFloat + 1.);
	double wholeFrames;
	currentAudioFramesPerVideoFrame = std::modf(currentAudioFramesPerVideoFrame, &wholeFrames) + audioFramesPerVideoFrameFloat;
	return wholeFrames;
}

double EmuSystem::frameRate()
{
	double time = frameTime().count();
	return time ? 1. / time : 0.;
}

double EmuSystem::frameRate(VideoSystem system)
{
	return 1. / frameTime(system).count();
}

IG::FloatSeconds EmuSystem::frameTime()
{
	return frameTime(videoSystem());
}

IG::FloatSeconds EmuSystem::frameTime(VideoSystem system)
{
	switch(system)
	{
		case VideoSystem::NATIVE_NTSC: return frameTimeNative;
		case VideoSystem::PAL: return frameTimePAL;
	}
	return {};
}

IG::FloatSeconds EmuSystem::defaultFrameTime(VideoSystem system)
{
	switch(system)
	{
		case VideoSystem::NATIVE_NTSC: return IG::FloatSeconds{1./60.};
		case VideoSystem::PAL: return IG::FloatSeconds{1./50.};
	}
	return {};
}

bool EmuSystem::frameTimeIsValid(VideoSystem system, IG::FloatSeconds time)
{
	auto rate = 1. / time.count(); // convert to frames per second
	switch(system)
	{
		case VideoSystem::NATIVE_NTSC: return rate >= 55 && rate <= 65;
		case VideoSystem::PAL: return rate >= 45 && rate <= 65;
	}
	return false;
}

bool EmuSystem::setFrameTime(VideoSystem system, IG::FloatSeconds time)
{
	if(!frameTimeIsValid(system, time))
		return false;
	switch(system)
	{
		bcase VideoSystem::NATIVE_NTSC: frameTimeNative = time;
		bcase VideoSystem::PAL: frameTimePAL = time;
	}
	return true;
}

[[gnu::weak]] FS::PathString EmuSystem::willLoadContentFromPath(std::string_view path, std::string_view displayName)
{
	return FS::PathString{path};
}

void EmuSystem::closeAndSetupNew(IG::CStringView path, std::string_view displayName)
{
	auto &app = EmuApp::get(appContext());
	closeRuntimeSystem(app, true);
	if(!IG::isUri(path))
		setupContentFilePaths(path, displayName);
	else
		setupContentUriPaths(path, displayName);
	logMsg("set content name:%s location:%s", contentName_.data(), contentLocation_.data());
	app.loadSessionOptions();
}

void EmuSystem::createWithMedia(GenericIO io, IG::CStringView path, std::string_view displayName,
	EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	if(io)
		loadContentFromFile(std::move(io), path, displayName, params, onLoadProgress);
	else
		loadContentFromPath(path, displayName, params, onLoadProgress);
}

void EmuSystem::loadContentFromPath(IG::CStringView pathStr, std::string_view displayName, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	auto path = willLoadContentFromPath(pathStr, displayName);
	if(!handlesGenericIO)
	{
		closeAndSetupNew(path, displayName);
		loadContent(FileIO{}, params, onLoadProgress);
		return;
	}
	logMsg("load from %s:%s", IG::isUri(path) ? "uri" : "path", path.data());
	loadContentFromFile(appContext().openFileUri(path, IO::AccessHint::SEQUENTIAL), path, displayName, params, onLoadProgress);
}

void EmuSystem::loadContentFromFile(GenericIO file, IG::CStringView path, std::string_view displayName, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	if(EmuApp::hasArchiveExtension(displayName))
	{
		ArchiveIO io{};
		FS::FileString originalName{};
		for(auto &entry : FS::ArchiveIterator{std::move(file)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			logMsg("archive file entry:%s", name.data());
			if(EmuSystem::defaultFsFilter(name))
			{
				originalName = name;
				io = entry.moveIO();
				break;
			}
		}
		if(!io)
		{
			throw std::runtime_error("No recognized file extensions in archive");
		}
		closeAndSetupNew(path, displayName);
		contentFileName_ = originalName;
		loadContent(io, params, onLoadProgress);
	}
	else
	{
		closeAndSetupNew(path, displayName);
		loadContent(file, params, onLoadProgress);
	}
}

void EmuSystem::throwFileReadError()
{
	throw std::runtime_error("Error reading file");
}

void EmuSystem::throwFileWriteError()
{
	throw std::runtime_error("Error writing file");
}

void EmuSystem::throwMissingContentDirError()
{
	throw std::runtime_error("This content must be opened with a folder, \"Browse For File\" isn't supported");
}

FS::PathString EmuSystem::contentDirectory(std::string_view name) const
{
	return FS::uriString(contentDirectory(), name);
}

std::string EmuSystem::contentDisplayName() const
{
	if(contentDisplayName_.size())
		return contentDisplayName_;
	return std::string{contentName_};
}

FS::FileString EmuSystem::contentFileName() const
{
	assert(contentFileName_.size());
	return contentFileName_;
}

void EmuSystem::setContentDisplayName(std::string_view name)
{
	logMsg("set content display name:%s", name.data());
	contentDisplayName_ = name;
}

FS::FileString EmuSystem::contentDisplayNameForPathDefaultImpl(IG::CStringView path) const
{
	return IG::stringWithoutDotExtension<FS::FileString>(appContext().fileUriDisplayName(path));
}

void EmuSystem::setInitialLoadPath(IG::CStringView path)
{
	assert(contentName_.empty());
	contentLocation_ = path;
}

char EmuSystem::saveSlotChar(int slot) const
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return '0' + slot;
		default: bug_unreachable("slot == %d", slot);
	}
}

char EmuSystem::saveSlotCharUpper(int slot) const
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_unreachable("slot == %d", slot);
	}
}

void EmuSystem::sessionOptionSet()
{
	if(!hasContent())
		return;
	sessionOptionsSet = true;
}

bool EmuSystem::inputHasTriggers()
{
	return inputLTriggerIndex != -1 && inputRTriggerIndex != -1;
}

void EmuSystem::flushBackupMemory(BackupMemoryDirtyFlags flags)
{
	onFlushBackupMemory(flags);
	backupMemoryDirtyFlags = 0;
	backupMemoryCounter = 0;
}

void EmuSystem::onBackupMemoryWritten(BackupMemoryDirtyFlags flags)
{
	backupMemoryDirtyFlags |= flags;
	backupMemoryCounter = 127;
}

bool EmuSystem::updateBackupMemoryCounter()
{
	if(backupMemoryCounter) [[unlikely]]
	{
		backupMemoryCounter--;
		if(!backupMemoryCounter)
		{
			flushBackupMemory(backupMemoryDirtyFlags);
			return true;
		}
	}
	return false;
}

EmuSystem &gSystem() { return gApp().system(); }

}

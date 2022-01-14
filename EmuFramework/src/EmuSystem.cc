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
#include "EmuTiming.hh"
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

EmuSystem::State EmuSystem::state = EmuSystem::State::OFF;
FS::PathString EmuSystem::contentDirectory_{};
FS::PathString EmuSystem::contentLocation_{};
FS::PathString EmuSystem::contentSaveDirectory_{};
FS::PathString EmuSystem::userSaveDirectory_{};
FS::PathString EmuSystem::firmwarePath_{};
FS::FileString EmuSystem::contentName_{};
std::string EmuSystem::contentDisplayName_{};
FS::FileString EmuSystem::contentFileName_{};
int EmuSystem::saveStateSlot = 0;
[[gnu::weak]] bool EmuSystem::inputHasKeyboard = false;
[[gnu::weak]] bool EmuSystem::inputHasShortBtnTexture = false;
[[gnu::weak]] int EmuSystem::inputLTriggerIndex = -1;
[[gnu::weak]] int EmuSystem::inputRTriggerIndex = -1;
[[gnu::weak]] bool EmuSystem::hasBundledGames = false;
[[gnu::weak]] bool EmuSystem::hasPALVideoSystem = false;
IG::FloatSeconds EmuSystem::frameTimeNative{1./60.};
IG::FloatSeconds EmuSystem::frameTimePAL{1./50.};
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
bool EmuSystem::sessionOptionsSet = false;
double EmuSystem::audioFramesPerVideoFrameFloat = 0;
double EmuSystem::currentAudioFramesPerVideoFrame = 0;
uint32_t EmuSystem::audioFramesPerVideoFrame = 0;
static EmuTiming emuTiming{};
[[gnu::weak]] std::array<int, EmuSystem::MAX_FACE_BTNS> EmuSystem::vControllerImageMap{0, 1, 2, 3, 4, 5, 6, 7};

static IG::Microseconds makeWantedAudioLatencyUSecs(uint8_t buffers)
{
	return buffers * std::chrono::duration_cast<IG::Microseconds>(EmuSystem::frameTime());
}

bool EmuSystem::stateExists(IG::ApplicationContext ctx, int slot)
{
	return ctx.fileUriExists(statePath(ctx, slot));
}

bool EmuSystem::shouldOverwriteExistingState(IG::ApplicationContext ctx)
{
	return !optionConfirmOverwriteState || !EmuSystem::stateExists(ctx, EmuSystem::saveStateSlot);
}

EmuFrameTimeInfo EmuSystem::advanceFramesWithTime(IG::FrameTime time)
{
	return emuTiming.advanceFramesWithTime(time);
}

void EmuSystem::setSpeedMultiplier(EmuAudio &emuAudio, uint8_t speed)
{
	emuTiming.setSpeedMultiplier(speed);
	emuAudio.setSpeedMultiplier(speed);
}

void EmuSystem::setupContentUriPaths(IG::ApplicationContext ctx, IG::CStringView uri, std::string_view displayName)
{
	contentFileName_ = displayName;
	contentName_ = IG::stringWithoutDotExtension(contentFileName_);
	contentLocation_ = uri;
	contentDirectory_ = FS::dirnameUri(uri);
	updateContentSaveDirectory(ctx);
}

void EmuSystem::setupContentFilePaths(IG::ApplicationContext ctx, IG::CStringView filePath, std::string_view displayName)
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
	updateContentSaveDirectory(ctx);
}

void EmuSystem::updateContentSaveDirectory(IG::ApplicationContext ctx)
{
	if(contentName_.empty())
		return;
	if(userSaveDirectory_.size())
	{
		if(userSaveDirectory_ == optionSavePathDefaultToken)
			contentSaveDirectory_ = fallbackSaveDirectory(ctx, true);
		else
			contentSaveDirectory_ = userSaveDirectory_;
	}
	else
	{
		if(contentDirectory_.size())
			contentSaveDirectory_ = contentDirectory_;
		else
			contentSaveDirectory_ = fallbackSaveDirectory(ctx, true);
	}
	logMsg("updated content save path:%s", contentSaveDirectory_.data());
}

FS::PathString EmuSystem::fallbackSaveDirectory(IG::ApplicationContext ctx, bool create)
{
	try
	{
		if(create)
			return FS::createDirectorySegments(ctx.storagePath(), "EmuEx", shortSystemName(), "saves");
		else
			return FS::pathString(ctx.storagePath(), "EmuEx", shortSystemName(), "saves");
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

FS::PathString EmuSystem::contentSaveDirectory()
{
	assert(!contentName_.empty());
	return contentSaveDirectory_;
}

FS::PathString EmuSystem::contentSavePath(IG::ApplicationContext ctx, std::string_view name)
{
	return FS::uriString(contentSaveDirectory(), name);
}

FS::PathString EmuSystem::contentSaveFilePath(IG::ApplicationContext ctx, std::string_view ext)
{
	return FS::uriString(contentSaveDirectory(), contentName().append(ext));
}

FS::PathString EmuSystem::userSaveDirectory()
{
	return userSaveDirectory_;
}

void EmuSystem::setUserSaveDirectory(IG::ApplicationContext ctx, IG::CStringView path)
{
	logMsg("set user save path:%s", path.data());
	userSaveDirectory_ = path;
	updateContentSaveDirectory(ctx);
	savePathChanged();
}

FS::PathString EmuSystem::firmwarePath()
{
	return firmwarePath_;
}

void EmuSystem::setFirmwarePath(std::string_view path)
{
	firmwarePath_ = path;
}

FS::PathString EmuSystem::statePath(IG::ApplicationContext ctx, std::string_view filename, std::string_view basePath)
{
	return FS::uriString(basePath, filename);
}

FS::PathString EmuSystem::statePath(IG::ApplicationContext ctx, int slot, std::string_view path)
{
	return statePath(ctx, stateFilename(slot), path);
}

void EmuSystem::closeRuntimeSystem(EmuApp &app, bool allowAutosaveState)
{
	if(gameIsRunning())
	{
		app.video().clear();
		app.audio().flush();
		if(allowAutosaveState)
			app.saveAutoState();
		app.saveSessionOptions();
		logMsg("closing game:%s", contentName_.data());
		closeSystem(app.appContext());
		app.cancelAutoSaveStateTimer();
		state = State::OFF;
	}
	clearGamePaths();
}

bool EmuSystem::gameIsRunning()
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
	app.audio().start(makeWantedAudioLatencyUSecs(optionSoundBuffers), makeWantedAudioLatencyUSecs(1));
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

void EmuSystem::configFrameTime(uint32_t rate)
{
	auto fTime = frameTime();
	configAudioRate(fTime, rate);
	audioFramesPerVideoFrame = std::ceil(rate * fTime.count());
	audioFramesPerVideoFrameFloat = (double)rate * fTime.count();
	currentAudioFramesPerVideoFrame = audioFramesPerVideoFrameFloat;
	emuTiming.setFrameTime(fTime);
}

void EmuSystem::configAudioPlayback(EmuAudio &emuAudio, uint32_t rate)
{
	configFrameTime(rate);
	emuAudio.setRate(rate);
}

uint32_t EmuSystem::updateAudioFramesPerVideoFrame()
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
	return frameTime(vidSysIsPAL() ? VIDSYS_PAL : VIDSYS_NATIVE_NTSC);
}

IG::FloatSeconds EmuSystem::frameTime(VideoSystem system)
{
	switch(system)
	{
		case VIDSYS_NATIVE_NTSC: return frameTimeNative;
		case VIDSYS_PAL: return frameTimePAL;
	}
	return {};
}

IG::FloatSeconds EmuSystem::defaultFrameTime(VideoSystem system)
{
	switch(system)
	{
		case VIDSYS_NATIVE_NTSC: return IG::FloatSeconds{1./60.};
		case VIDSYS_PAL: return IG::FloatSeconds{1./50.};
	}
	return {};
}

bool EmuSystem::frameTimeIsValid(VideoSystem system, IG::FloatSeconds time)
{
	auto rate = 1. / time.count(); // convert to frames per second
	switch(system)
	{
		case VIDSYS_NATIVE_NTSC: return rate >= 55 && rate <= 65;
		case VIDSYS_PAL: return rate >= 45 && rate <= 65;
	}
	return false;
}

bool EmuSystem::setFrameTime(VideoSystem system, IG::FloatSeconds time)
{
	if(!frameTimeIsValid(system, time))
		return false;
	switch(system)
	{
		bcase VIDSYS_NATIVE_NTSC: frameTimeNative = time;
		bcase VIDSYS_PAL: frameTimePAL = time;
	}
	return true;
}

[[gnu::weak]] FS::PathString EmuSystem::willLoadGameFromPath(IG::ApplicationContext, std::string_view path, std::string_view displayName)
{
	return FS::PathString{path};
}

void EmuSystem::prepareAudio(EmuAudio &audio)
{
	onPrepareAudio(audio);
	configAudioPlayback(audio, optionSoundRate);
}

void EmuSystem::closeAndSetupNew(IG::ApplicationContext ctx, IG::CStringView path, std::string_view displayName)
{
	auto &app = EmuApp::get(ctx);
	EmuSystem::closeRuntimeSystem(app, true);
	if(!IG::isUri(path))
		EmuSystem::setupContentFilePaths(ctx, path, displayName);
	else
		EmuSystem::setupContentUriPaths(ctx, path, displayName);
	logMsg("set content name:%s location:%s", contentName_.data(), contentLocation_.data());
	app.loadSessionOptions();
}

void EmuSystem::createWithMedia(IG::ApplicationContext ctx, GenericIO io, IG::CStringView path, std::string_view displayName,
	EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	if(io)
		loadGameFromFile(ctx, std::move(io), path, displayName, params, onLoadProgress);
	else
		loadGameFromPath(ctx, path, displayName, params, onLoadProgress);
}

void EmuSystem::loadGameFromPath(IG::ApplicationContext ctx, IG::CStringView pathStr, std::string_view displayName, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	auto path = willLoadGameFromPath(ctx, pathStr, displayName);
	if(!handlesGenericIO)
	{
		closeAndSetupNew(ctx, path, displayName);
		loadGame(ctx, FileIO{}, params, onLoadProgress);
		return;
	}
	logMsg("load from %s:%s", IG::isUri(path) ? "uri" : "path", path.data());
	loadGameFromFile(ctx, ctx.openFileUri(path, IO::AccessHint::SEQUENTIAL), path, displayName, params, onLoadProgress);
}

void EmuSystem::loadGameFromFile(IG::ApplicationContext ctx, GenericIO file, IG::CStringView path, std::string_view displayName, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
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
		closeAndSetupNew(ctx, path, displayName);
		contentFileName_ = originalName;
		EmuSystem::loadGame(ctx, io, params, onLoadProgress);
	}
	else
	{
		closeAndSetupNew(ctx, path, displayName);
		EmuSystem::loadGame(ctx, file, params, onLoadProgress);
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

FS::PathString EmuSystem::contentDirectory(IG::ApplicationContext ctx, std::string_view name)
{
	return FS::uriString(contentDirectory(), name);
}

std::string EmuSystem::contentDisplayName()
{
	if(contentDisplayName_.size())
		return contentDisplayName_;
	return std::string{contentName_};
}

FS::FileString EmuSystem::contentFileName()
{
	assert(contentFileName_.size());
	return contentFileName_;
}

void EmuSystem::setContentDisplayName(std::string_view name)
{
	logMsg("set content display name:%s", name.data());
	contentDisplayName_ = name;
}

FS::FileString EmuSystem::contentDisplayNameForPathDefaultImpl(IG::ApplicationContext ctx, IG::CStringView path)
{
	return IG::stringWithoutDotExtension<FS::FileString>(ctx.fileUriDisplayName(path));
}

void EmuSystem::setInitialLoadPath(IG::CStringView path)
{
	assert(contentName_.empty());
	contentLocation_ = path;
}

char EmuSystem::saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return '0' + slot;
		default: bug_unreachable("slot == %d", slot); return 0;
	}
}

char EmuSystem::saveSlotCharUpper(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_unreachable("slot == %d", slot); return 0;
	}
}

void EmuSystem::sessionOptionSet()
{
	if(!gameIsRunning())
		return;
	sessionOptionsSet = true;
}

bool EmuSystem::inputHasTriggers()
{
	return inputLTriggerIndex != -1 && inputRTriggerIndex != -1;
}

[[gnu::weak]] void EmuSystem::onInit(IG::ApplicationContext) {}

[[gnu::weak]] void EmuSystem::initOptions(EmuApp &) {}

[[gnu::weak]] void EmuSystem::onOptionsLoaded(IG::ApplicationContext) {}

[[gnu::weak]] void EmuSystem::saveBackupMem(IG::ApplicationContext) {}

[[gnu::weak]] void EmuSystem::savePathChanged() {}

[[gnu::weak]] unsigned EmuSystem::multiresVideoBaseX() { return 0; }

[[gnu::weak]] unsigned EmuSystem::multiresVideoBaseY() { return 0; }

[[gnu::weak]] bool EmuSystem::vidSysIsPAL() { return false; }

[[gnu::weak]] bool EmuSystem::onPointerInputStart(Input::Event, Input::DragTrackerState, IG::WindowRect) { return false; }

[[gnu::weak]] bool EmuSystem::onPointerInputUpdate(Input::Event, Input::DragTrackerState, Input::DragTrackerState, IG::WindowRect) { return false; }

[[gnu::weak]] bool EmuSystem::onPointerInputEnd(Input::Event, Input::DragTrackerState, IG::WindowRect) { return false; }

[[gnu::weak]] void EmuSystem::onPrepareAudio(EmuAudio &) {}

[[gnu::weak]] bool EmuSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat) { return false; }

[[gnu::weak]] FS::FileString EmuSystem::contentDisplayNameForPath(IG::ApplicationContext ctx, IG::CStringView path)
{
	return contentDisplayNameForPathDefaultImpl(ctx, path);
}

[[gnu::weak]] bool EmuSystem::shouldFastForward() { return false; }

[[gnu::weak]] void EmuSystem::writeConfig(IO &io) {}

[[gnu::weak]] bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize) { return false; }

[[gnu::weak]] bool EmuSystem::resetSessionOptions(EmuApp &) { return false; }

[[gnu::weak]] void EmuSystem::onSessionOptionsLoaded(EmuApp &) {}

[[gnu::weak]] void EmuSystem::writeSessionConfig(IO &io) {}

[[gnu::weak]] bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize) { return false; }

[[gnu::weak]] void EmuSystem::loadGame(IG::ApplicationContext, IO &io, EmuSystemCreateParams params,
	EmuSystem::OnLoadProgressDelegate onLoadProgress)
{
	loadGame(io, params, onLoadProgress);
}

[[gnu::weak]] void EmuSystem::reset(EmuApp &, ResetMode mode)
{
	reset(mode);
}

[[gnu::weak]] void EmuSystem::loadState(EmuApp &, IG::CStringView path)
{
	loadState(path);
}

[[gnu::weak]] void EmuSystem::saveState(IG::ApplicationContext, IG::CStringView path)
{
	saveState(path);
}

[[gnu::weak]] void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	video.clear();
}

[[gnu::weak]] void EmuSystem::handleInputAction(EmuApp *app, Input::Action state, unsigned emuKey)
{
	handleInputAction(app, state, emuKey, 0);
}

[[gnu::weak]] void EmuSystem::handleInputAction(EmuApp *app, Input::Action state, unsigned emuKey, uint32_t metaState)
{
	handleInputAction(app, state, emuKey);
}

[[gnu::weak]] void EmuSystem::onVKeyboardShown(VControllerKeyboard &, bool shown) {}

}

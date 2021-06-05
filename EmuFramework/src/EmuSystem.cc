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
#include "EmuOptions.hh"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/FileUtils.hh>
#include <emuframework/FilePicker.hh>
#include "EmuTiming.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/ScopeGuard.hh>
#include <algorithm>
#include <cstring>

EmuSystem::State EmuSystem::state = EmuSystem::State::OFF;
FS::PathString EmuSystem::gamePath_{};
FS::PathString EmuSystem::fullGamePath_{};
FS::PathString EmuSystem::savePath_{};
FS::PathString EmuSystem::defaultSavePath_{};
FS::PathString EmuSystem::gameSavePath_{};
FS::FileString EmuSystem::gameName_{};
FS::FileString EmuSystem::fullGameName_{};
FS::FileString EmuSystem::originalGameName_{};
int EmuSystem::saveStateSlot = 0;
[[gnu::weak]] bool EmuSystem::inputHasKeyboard = false;
[[gnu::weak]] bool EmuSystem::inputHasShortBtnTexture = false;
[[gnu::weak]] bool EmuSystem::hasBundledGames = false;
[[gnu::weak]] bool EmuSystem::hasPALVideoSystem = false;
IG::FloatSeconds EmuSystem::frameTimeNative{1./60.};
IG::FloatSeconds EmuSystem::frameTimePAL{1./50.};
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

static IG::Microseconds makeWantedAudioLatencyUSecs(uint8_t buffers)
{
	return buffers * std::chrono::duration_cast<IG::Microseconds>(EmuSystem::frameTime());
}

bool EmuSystem::stateExists(int slot)
{
	auto saveStr = sprintStateFilename(slot);
	return FS::exists(saveStr.data());
}

bool EmuSystem::shouldOverwriteExistingState()
{
	return !optionConfirmOverwriteState || !EmuSystem::stateExists(EmuSystem::saveStateSlot);
}

EmuFrameTimeInfo EmuSystem::advanceFramesWithTime(Base::FrameTime time)
{
	return emuTiming.advanceFramesWithTime(time);
}

void EmuSystem::setSpeedMultiplier(EmuAudio &emuAudio, uint8_t speed)
{
	emuTiming.setSpeedMultiplier(speed);
	emuAudio.setSpeedMultiplier(speed);
}

void EmuSystem::setupGamePaths(Base::ApplicationContext ctx, const char *filePath)
{
	if(FS::exists(filePath))
	{
		// find the realpath of the dirname portion separately in case the file is a symlink
		strcpy(gamePath_.data(), FS::dirname(filePath).data());
		char realPath[PATH_MAX];
		if(!realpath(gamePath_.data(), realPath))
		{
			gamePath_ = {};
			logErr("error in realpath()");
		}
		else
		{
			strcpy(gamePath_.data(), realPath); // destination is always large enough
			logMsg("set game directory: %s", gamePath_.data());
			fixFilePermissions(ctx, gamePath_);
		}
	}

	string_copy(gameName_, FS::basename(filePath).data());

	if(strlen(gamePath_.data()))
	{
		string_printf(fullGamePath_, "%s/%s", gamePath_.data(), gameName_.data());
		logMsg("set full game path: %s", fullGamePath_.data());
	}

	// If gameName has an extension, truncate it
	gameName_ = FS::makeFileStringWithoutDotExtension(gameName_);
	logMsg("set game name: %s", gameName_.data());

	setupGameSavePath(ctx);
}

void EmuSystem::setupGameSavePath(Base::ApplicationContext ctx)
{
	if(!strlen(gameName_.data()))
		return;
	if(strlen(savePath_.data()))
	{
		if(string_equal(savePath_.data(), optionSavePathDefaultToken))
			setGameSavePath(ctx, defaultSavePath(ctx));
		else
			setGameSavePath(ctx, savePath_.data());
	}
	else
	{
		setGameSavePath(ctx, gamePath_.data());
	}
}

FS::PathString EmuSystem::baseSavePath(Base::ApplicationContext ctx)
{
	if(strlen(savePath_.data()) && !string_equal(savePath_.data(), optionSavePathDefaultToken))
	{
		return savePath_;
	}
	return FS::makePathStringPrintf("%s/Game Data/%s", ctx.sharedStoragePath().data(), shortSystemName());
}

static bool hasWriteAccessToDir(Base::ApplicationContext ctx, const char *path)
{
	auto hasAccess = FS::access(path, FS::acc::w);
	#ifdef CONFIG_BASE_ANDROID
	// on Android 4.4 also test file creation since
	// access() can still claim an SD card is writable
	// even though parts are locked-down by the OS
	if(ctx.androidSDK() >= 19)
	{
		auto testFilePath = FS::makePathStringPrintf("%s/.safe-to-delete-me", path);
		FileIO testFile;
		auto ec = testFile.create(testFilePath.data());
		if(ec)
		{
			hasAccess = false;
		}
		else
		{
			FS::remove(testFilePath);
		}
	}
	#endif
	return hasAccess;
}

void EmuSystem::setGameSavePath(Base::ApplicationContext ctx, const char *path)
{
	if(!strlen(gameName_.data()))
		return;
	bool reportNoWriteAccess = false;
	// check if the path is writable
	if(path && strlen(path))
	{
		fixFilePermissions(ctx, path);
		if(optionCheckSavePathWriteAccess && !hasWriteAccessToDir(ctx, path))
		{
			reportNoWriteAccess = true;
		}
		else
		{
			logMsg("set game save path: %s", path);
			string_copy(gameSavePath_, path);
			return;
		}
	}
	// fallback to a default path
	logMsg("set game save path to default: %s", defaultSavePath(ctx));
	string_copy(gameSavePath_, defaultSavePath(ctx));
	fixFilePermissions(ctx, gameSavePath_);
	if(reportNoWriteAccess)
	{
		logWarn("Save path lacks write access, using default:\n%s", gameSavePath_.data());
	}
}

FS::PathString EmuSystem::makeDefaultBaseSavePath(Base::ApplicationContext ctx)
{
	FS::PathString pathTemp = ctx.sharedStoragePath();
	string_cat(pathTemp, "/Game Data");
	FS::create_directory(pathTemp);
	string_cat(pathTemp, "/");
	string_cat(pathTemp, shortSystemName());
	FS::create_directory(pathTemp);
	return pathTemp;
}

void EmuSystem::makeDefaultSavePath(Base::ApplicationContext ctx)
{
	assert(strlen(gameName_.data()));
	auto pathTemp = makeDefaultBaseSavePath(ctx);
	string_cat(pathTemp, "/");
	string_cat(pathTemp, gameName_.data());
	FS::create_directory(pathTemp);
}

void EmuSystem::clearGamePaths()
{
	gameName_ = {};
	fullGameName_ = {};
	originalGameName_ = {};
	gamePath_ = {};
	fullGamePath_ = {};
	defaultSavePath_ = {};
	gameSavePath_ = {};
}

const char *EmuSystem::savePath()
{
	return gameSavePath_.data();
}

const char *EmuSystem::defaultSavePath(Base::ApplicationContext ctx)
{
	assert(strlen(gameName_.data()));
	if(!strlen(defaultSavePath_.data()))
	{
		string_printf(defaultSavePath_, "%s/Game Data/%s/%s", ctx.sharedStoragePath().data(), shortSystemName(), gameName_.data());
		logMsg("game default save path: %s", defaultSavePath_.data());
	}
	if(!FS::exists(defaultSavePath_.data()))
		makeDefaultSavePath(ctx);
	return defaultSavePath_.data();
}

FS::PathString EmuSystem::baseDefaultGameSavePath(Base::ApplicationContext ctx)
{
	return FS::makePathStringPrintf("%s/Game Data/%s", ctx.sharedStoragePath().data(), shortSystemName());
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
		logMsg("closing game %s", gameName_.data());
		closeSystem();
		app.cancelAutoSaveStateTimer();
		state = State::OFF;
	}
	clearGamePaths();
}

bool EmuSystem::gameIsRunning()
{
	return gameName_[0];
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
		runFrame(nullptr, &video, nullptr);
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

[[gnu::weak]] FS::PathString EmuSystem::willLoadGameFromPath(FS::PathString path)
{
	return path;
}

void EmuSystem::prepareAudio(EmuAudio &audio)
{
	onPrepareAudio(audio);
	configAudioPlayback(audio, optionSoundRate);
}

static void closeAndSetupNew(Base::ApplicationContext ctx, const char *path)
{
	auto &app = EmuApp::get(ctx);
	EmuSystem::closeRuntimeSystem(app, true);
	EmuSystem::setupGamePaths(ctx, path);
	app.loadSessionOptions();
}

void EmuSystem::createWithMedia(Base::ApplicationContext ctx, GenericIO io, const char *path, const char *name, Error &err, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	if(io)
		err = loadGameFromFile(ctx, std::move(io), name, params, onLoadProgress);
	else
		err = loadGameFromPath(ctx, path, params, onLoadProgress);
}

EmuSystem::Error EmuSystem::loadGameFromPath(Base::ApplicationContext ctx, const char *pathStr, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	auto path = willLoadGameFromPath(FS::makePathString(pathStr));
	if(!handlesGenericIO)
	{
		closeAndSetupNew(ctx, path.data());
		auto err = loadGame(ctx, GenericIO{}, params, onLoadProgress);
		if(err)
		{
			clearGamePaths();
		}
		return err;
	}
	logMsg("load from path:%s", path.data());
	FileIO io{};
	auto ec = io.open(path, IO::AccessHint::SEQUENTIAL);
	if(ec)
	{
		return makeError("Error opening file: %s", ec.message().c_str());
	}
	return loadGameFromFile(ctx, io.makeGeneric(), path.data(), params, onLoadProgress);
}

EmuSystem::Error EmuSystem::loadGameFromFile(Base::ApplicationContext ctx, GenericIO file, const char *name, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	Error err;
	if(EmuApp::hasArchiveExtension(name))
	{
		ArchiveIO io{};
		std::error_code ec{};
		FS::FileString originalName{};
		for(auto &entry : FS::ArchiveIterator{std::move(file), ec})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			logMsg("archive file entry:%s", name);
			if(EmuSystem::defaultFsFilter(name))
			{
				string_copy(originalName, name);
				io = entry.moveIO();
				break;
			}
		}
		if(ec)
		{
			//EmuApp::printfMessage(3, true, "Error opening archive: %s", ec.message().c_str());
			return makeError("Error opening archive: %s", ec.message().c_str());
		}
		if(!io)
		{
			//EmuApp::postErrorMessage("No recognized file extensions in archive");
			return makeError("No recognized file extensions in archive");
		}
		closeAndSetupNew(ctx, name);
		originalGameName_ = originalName;
		err = EmuSystem::loadGame(ctx, io, params, onLoadProgress);
	}
	else
	{
		closeAndSetupNew(ctx, name);
		err = EmuSystem::loadGame(ctx, file, params, onLoadProgress);
	}
	if(err)
	{
		clearGamePaths();
	}
	return err;
}

EmuSystem::Error EmuSystem::makeError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	std::array<char, 1024> str{};
	vsnprintf(str.data(), str.size(), format, args);
	return std::runtime_error(str.data());
}

EmuSystem::Error EmuSystem::makeError(std::error_code ec)
{
	return std::runtime_error(ec.message().c_str());
}

EmuSystem::Error EmuSystem::makeFileReadError()
{
	return std::runtime_error("Error reading file");
}

EmuSystem::Error EmuSystem::makeFileWriteError()
{
	return std::runtime_error("Error writing file");
}

EmuSystem::Error EmuSystem::makeBlankError()
{
	return std::runtime_error("");
}

FS::FileString EmuSystem::fullGameName()
{
	return strlen(fullGameName_.data()) ? fullGameName_ : gameName_;
}

FS::FileString EmuSystem::gameFileName()
{
	return FS::basename(fullGamePath_);
}

FS::FileString EmuSystem::originalGameFileName()
{
	return strlen(originalGameName_.data()) ? originalGameName_ : gameFileName();
}

void EmuSystem::setFullGameName(const char *name)
{
	string_copy(fullGameName_, name);
}

FS::FileString EmuSystem::fullGameNameForPathDefaultImpl(const char *path)
{
	auto basename = FS::basename(path);
	auto dotpos = strrchr(basename.data(), '.');
	if(dotpos)
		*dotpos = 0;
	//logMsg("full game name:%s", basename.data());
	return basename;
}

void EmuSystem::setInitialLoadPath(const char *path)
{
	assert(!strlen(gameName_.data()));
	string_copy(gamePath_, path);
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

[[gnu::weak]] EmuSystem::Error EmuSystem::onInit(Base::ApplicationContext) { return {}; }

[[gnu::weak]] void EmuSystem::initOptions() {}

[[gnu::weak]] EmuSystem::Error EmuSystem::onOptionsLoaded(Base::ApplicationContext) { return {}; }

[[gnu::weak]] void EmuSystem::saveBackupMem() {}

[[gnu::weak]] void EmuSystem::savePathChanged() {}

[[gnu::weak]] unsigned EmuSystem::multiresVideoBaseX() { return 0; }

[[gnu::weak]] unsigned EmuSystem::multiresVideoBaseY() { return 0; }

[[gnu::weak]] bool EmuSystem::vidSysIsPAL() { return false; }

[[gnu::weak]] bool EmuSystem::touchControlsApplicable() { return true; }

[[gnu::weak]] bool EmuSystem::handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect) { return false; }

[[gnu::weak]] void EmuSystem::onPrepareAudio(EmuAudio &) {}

[[gnu::weak]] void EmuSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat) {}

[[gnu::weak]] FS::FileString EmuSystem::fullGameNameForPath(Base::ApplicationContext, const char *path)
{
	return fullGameNameForPathDefaultImpl(path);
}

[[gnu::weak]] bool EmuSystem::shouldFastForward() { return false; }

[[gnu::weak]] void EmuSystem::writeConfig(IO &io) {}

[[gnu::weak]] bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize) { return false; }

[[gnu::weak]] bool EmuSystem::resetSessionOptions(EmuApp &) { return false; }

[[gnu::weak]] void EmuSystem::onSessionOptionsLoaded(EmuApp &) {}

[[gnu::weak]] void EmuSystem::writeSessionConfig(IO &io) {}

[[gnu::weak]] bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize) { return false; }

[[gnu::weak]] EmuSystem::Error EmuSystem::loadGame(Base::ApplicationContext, IO &io, EmuSystemCreateParams params,
	EmuSystem::OnLoadProgressDelegate onLoadProgress)
{
	return loadGame(io, params, onLoadProgress);
}

[[gnu::weak]] void EmuSystem::reset(EmuApp &, ResetMode mode)
{
	reset(mode);
}

[[gnu::weak]] EmuSystem::Error EmuSystem::loadState(EmuApp &, const char *path)
{
	return loadState(path);
}

[[gnu::weak]] EmuSystem::Error EmuSystem::saveState(EmuApp &, const char *path)
{
	return saveState(path);
}

[[gnu::weak]] void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	video.clear();
}

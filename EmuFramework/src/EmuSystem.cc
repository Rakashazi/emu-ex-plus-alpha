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
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuViewController.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FSUtils.hh>
#include <imagine/io/IO.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/string.h>
#include <imagine/util/zlib.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <algorithm>
#include <cstring>
#include "pathUtils.hh"

namespace EmuEx
{

constexpr SystemLogger log{"EmuSystem"};

[[gnu::weak]] bool EmuSystem::inputHasKeyboard = false;
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
[[gnu::weak]] F2Size EmuSystem::validFrameRateRange{minFrameRate, 80.};
[[gnu::weak]] bool EmuSystem::hasRectangularPixels = false;
[[gnu::weak]] bool EmuSystem::stateSizeChangesAtRuntime = false;

bool EmuSystem::stateExists(int slot) const
{
	return appContext().fileUriExists(statePath(slot));
}

std::string_view EmuSystem::stateSlotName(int slot)
{
	assert(slot >= 0 && slot < 10);
	static constexpr std::string_view str[]{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	return str[slot];
}

bool EmuApp::shouldOverwriteExistingState() const
{
	return !confirmOverwriteState || !system().stateExists(system().stateSlot());
}

void EmuSystem::loadState(EmuApp &app, CStringView uri)
{
	auto file = appContext().openFileUri(uri, {.accessHint = IOAccessHint::All});
	readState(app, file.buffer(IOBufferMode::Release));
}

void EmuSystem::saveState(CStringView uri)
{
	auto file = appContext().openFileUri(uri, OpenFlags::newFile());
	file.write(saveState().span());
}

DynArray<uint8_t> EmuSystem::saveState()
{
	auto stateArr = dynArrayForOverwrite<uint8_t>(stateSize());
	stateArr.trim(writeState(stateArr));
	return stateArr;
}

DynArray<uint8_t> EmuSystem::uncompressGzipState(std::span<uint8_t> buff, size_t expectedSize)
{
	assert(hasGzipHeader(buff));
	auto uncompSize = gzipUncompressedSize(buff);
	if(expectedSize && expectedSize != uncompSize)
		throw std::runtime_error("Invalid state size from header");
	auto uncompArr = dynArrayForOverwrite<uint8_t>(uncompSize);
	auto size = uncompressGzip(uncompArr, buff);
	if(!size)
		throw std::runtime_error("Error uncompressing state");
	if(expectedSize && size != expectedSize)
		throw std::runtime_error("Invalid state size");
	return uncompArr;
}

void EmuSystem::setupContentUriPaths(CStringView uri, std::string_view displayName)
{
	contentFileName_ = displayName;
	contentName_ = IG::withoutDotExtension(contentFileName_);
	contentLocation_ = uri;
	contentDirectory_ = FS::dirnameUri(uri);
	updateContentSaveDirectory();
}

void EmuSystem::setupContentFilePaths(CStringView filePath, std::string_view displayName)
{
	contentFileName_ = displayName;
	contentName_ = IG::withoutDotExtension(contentFileName_);
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
		log.error("error in realpath() while setting content directory");
		contentDirectory_ = fileDir;
		contentLocation_ = filePath;
	}
	else
	{
		contentDirectory_ = realPath.data();
		contentLocation_ = FS::pathString(contentDirectory_, contentFileName_);
		log.info("set content directory:{}", contentDirectory_);
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
	log.info("updated content save path:{}", contentSaveDirectory_);
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
		log.error("error making fallback save dir");
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

FS::PathString EmuSystem::contentSavePath(std::string_view name) const
{
	assert(!contentName_.empty());
	return FS::uriString(contentSaveDirectory(), name);
}

FS::PathString EmuSystem::contentSaveFilePath(std::string_view ext) const
{
	assert(!contentName_.empty());
	return FS::uriString(contentSaveDirectory(), contentNameExt(ext));
}

void EmuSystem::setUserSaveDirectory(CStringView path)
{
	log.info("set user save path:{}", path);
	userSaveDirectory_ = path;
	updateContentSaveDirectory();
	savePathChanged();
}

FS::FileString EmuSystem::stateFilename(std::string_view name) const
{
	FS::FileString filename{name};
	filename += stateFilenameExt();
	return filename;
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

FS::PathString EmuSystem::userPath(std::string_view userDir, std::string_view filename) const
{
	return FS::uriString(userPath(userDir), filename);
}

FS::PathString EmuSystem::userPath(std::string_view userDir) const
{
	if(userDir.size())
	{
		if(userDir == optionUserPathContentToken)
			return contentDirectory();
		else
			return FS::PathString{userDir};
	}
	else
	{
		return contentSaveDirectory();
	}
}

FS::PathString EmuSystem::userFilePath(std::string_view userDir, std::string_view ext) const
{
	assert(!contentName_.empty());
	return userPath(userDir, contentNameExt(ext));
}

void EmuSystem::closeRuntimeSystem(EmuApp &app)
{
	if(hasContent())
	{
		app.video.clear();
		app.audio.flush();
		app.autosaveManager.save();
		app.saveSessionOptions();
		log.info("closing game:{}", contentName_);
		flushBackupMemory(app);
		closeSystem();
		app.autosaveManager.cancelTimer();
		app.rewindManager.clear();
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
	timing.reset();
}

void EmuSystem::pause(EmuApp &app)
{
	if(isActive())
		state = State::PAUSED;
	app.audio.stop();
	app.autosaveManager.pauseTimer();
	app.rewindManager.pauseTimer();
	onStop();
}

void EmuSystem::start(EmuApp &app)
{
	state = State::ACTIVE;
	if(inputHasKeyboard)
		app.defaultVController().keyboard().setShiftActive(false);
	clearInputBuffers(app.viewController().inputView);
	resetFrameTime();
	onStart();
	app.startAudio();
	app.autosaveManager.startTimer();
	if(stateSizeChangesAtRuntime && app.rewindManager.maxStates)
	{
		auto newStateSize = stateSize();
		if(newStateSize != app.rewindManager.stateSize)
			app.rewindManager.reset(newStateSize);
	}
	app.rewindManager.startTimer();
}

SteadyClockTime EmuSystem::benchmark(EmuVideo &video)
{
	auto before = SteadyClock::now();
	for([[maybe_unused]] auto i : iotaCount(180))
	{
		runFrame({}, &video, nullptr);
	}
	return SteadyClock::now() - before;
}

void EmuSystem::configFrameTime(int outputRate, FrameTime outputFrameTime)
{
	if(!hasContent())
		return;
	if(frameTimeMultiplier == 1.)
	{
		configAudioRate(outputFrameTime, outputRate);
		audioFramesPerVideoFrameFloat = outputRate * duration_cast<FloatSeconds>(outputFrameTime).count();
		audioFramesPerVideoFrame = std::ceil(audioFramesPerVideoFrameFloat);
		currentAudioFramesPerVideoFrame = audioFramesPerVideoFrameFloat;
	}
	timing.setFrameTime(outputFrameTime);
}

void EmuSystem::onFrameTimeChanged()
{
	log.info("frame rate changed:{}", scaledFrameRate());
	EmuApp::get(appContext()).configFrameTime();
}

double EmuSystem::audioMixRate(int outputRate, double inputFrameRate, FrameTime outputFrameTime)
{
	assumeExpr(outputRate > 0);
	assumeExpr(inputFrameRate > 0);
	assumeExpr(outputFrameTime.count() > 0);
	return inputFrameRate * duration_cast<FloatSeconds>(outputFrameTime).count() * outputRate;
}

int EmuSystem::updateAudioFramesPerVideoFrame()
{
	assumeExpr(currentAudioFramesPerVideoFrame < audioFramesPerVideoFrameFloat + 1.);
	double wholeFrames;
	currentAudioFramesPerVideoFrame = std::modf(currentAudioFramesPerVideoFrame, &wholeFrames) + audioFramesPerVideoFrameFloat;
	return wholeFrames;
}

[[gnu::weak]] FS::PathString EmuSystem::willLoadContentFromPath(std::string_view, std::string_view)
{
	return {};
}

void EmuSystem::closeAndSetupNew(CStringView path, std::string_view displayName)
{
	auto &app = EmuApp::get(appContext());
	closeRuntimeSystem(app);
	if(!IG::isUri(path))
		setupContentFilePaths(path, displayName);
	else
		setupContentUriPaths(path, displayName);
	log.info("set content name:{} location:{}", contentName_, contentLocation_);
	app.loadSessionOptions();
}

void EmuSystem::createWithMedia(IO io, CStringView path, std::string_view displayName,
	EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	if(io)
		loadContentFromFile(std::move(io), path, displayName, params, onLoadProgress);
	else
		loadContentFromPath(path, displayName, params, onLoadProgress);
}

void EmuSystem::loadContentFromPath(CStringView pathStr, std::string_view displayNameStr, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	FS::PathString path;
	FS::FileString displayName;
	if(auto altPath = willLoadContentFromPath(pathStr, displayName);
		altPath.size() && appContext().fileUriExists(altPath))
	{
		log.info("loading {} instead of {}", altPath, pathStr);
		path = altPath;
		displayName = appContext().fileUriDisplayName(altPath);
	}
	else
	{
		path = pathStr;
		displayName = displayNameStr;
	}
	if(!handlesGenericIO)
	{
		closeAndSetupNew(path, displayName);
		IO nullIO{};
		loadContent(nullIO, params, onLoadProgress);
		return;
	}
	log.info("load from {}:{}", IG::isUri(path) ? "uri" : "path", path);
	loadContentFromFile(appContext().openFileUri(path, {.accessHint = IOAccessHint::Sequential}),
		path, displayName, params, onLoadProgress);
}

void EmuSystem::loadContentFromFile(IO file, CStringView path, std::string_view displayName, EmuSystemCreateParams params, OnLoadProgressDelegate onLoadProgress)
{
	if(!EmuSystem::handlesArchiveFiles && EmuApp::hasArchiveExtension(displayName))
	{
		IO io{};
		FS::FileString originalName{};
		for(auto &entry : FS::ArchiveIterator{std::move(file)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			log.info("archive file entry:{}", name);
			if(EmuSystem::defaultFsFilter(name))
			{
				originalName = name;
				io = std::move(entry);
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

FS::PathString EmuSystem::contentFilePath(std::string_view ext) const
{
	assert(!contentName_.empty());
	return contentDirectory(contentNameExt(ext));
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
	log.info("set content display name:{}", name);
	contentDisplayName_ = name;
}

FS::FileString EmuSystem::contentDisplayNameForPathDefaultImpl(CStringView path) const
{
	return FS::FileString{IG::withoutDotExtension(appContext().fileUriDisplayName(path))};
}

void EmuSystem::setInitialLoadPath(CStringView path)
{
	assert(contentName_.empty());
	if(!path)
		return;
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

void EmuSystem::flushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags flags)
{
	onFlushBackupMemory(app, flags);
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
			flushBackupMemory(EmuApp::get(appContext()), backupMemoryDirtyFlags);
			return true;
		}
	}
	return false;
}

FileIO EmuSystem::openStaticBackupMemoryFile(CStringView uri, size_t size, uint8_t initValue) const
{
	if(!size) [[unlikely]]
		return {};
	auto file = appContext().openFileUri(uri, OpenFlags::testCreateFile());
	if(!file) [[unlikely]]
		return {};
	auto fileSize = file.size();
	if(fileSize != size)
		file.truncate(size);
	// size is static so try to use a mapped file for writing
	bool isMapped = file.tryMap(OpenFlags::createFile());
	if(initValue && fileSize < size)
	{
		if(isMapped)
		{
			auto buff = file.map();
			std::fill(&buff[fileSize], &buff[size], initValue);
		}
		else
		{
			size_t fillSize = size - fileSize;
			uint8_t fillBuff[fillSize];
			memset(fillBuff, initValue, fillSize);
			log.info("padding {} bytes at offset {} with value:{:X}", fillSize, fileSize, initValue);
			file.write(fillBuff, fillSize, fileSize);
		}
	}
	return file;
}

EmuSystem &gSystem() { return gApp().system(); }

}

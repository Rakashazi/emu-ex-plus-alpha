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

#include "EmuOptions.hh"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/VController.hh>
#include "privateInput.hh"
#include "WindowData.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

void EmuApp::initOptions(IG::ApplicationContext ctx)
{
	optionSoundRate.initDefault(audioManager().nativeRate());

	#ifdef CONFIG_BASE_IOS
	if(ctx.deviceIsIPad())
		optionFontSize.initDefault(5000);
	#endif

	#ifdef __ANDROID__
	auto androidSdk = ctx.androidSDK();
	if(ctx.hasHardwareNavButtons())
	{
		optionLowProfileOSNav.isConst = 1;
		optionHideOSNav.isConst = 1;
	}
	else
	{
		if(androidSdk >= 19)
			optionHideOSNav.initDefault(1);
	}
	if(androidSdk >= 11)
	{
		optionNotificationIcon.initDefault(false);
		if(androidSdk >= 17)
			optionNotificationIcon.isConst = true;
	}
	if(androidSdk < 12)
	{
		optionNotifyInputDeviceChange.isConst = 1;
	}
	if(androidSdk < 17)
	{
		optionShowOnSecondScreen.isConst = true;
	}
	else if(FS::exists(FS::pathString(ctx.sharedStoragePath(), "emuex_disable_presentation_displays")))
	{
		logMsg("force-disabling presentation display support");
		optionShowOnSecondScreen.initDefault(false);
		optionShowOnSecondScreen.isConst = true;
	}
	else
	{
		#ifdef CONFIG_BLUETOOTH
		optionShowBluetoothScan.initDefault(0);
		#endif
	}
	{
		auto type = ctx.sustainedPerformanceModeType();
		if(type == IG::SustainedPerformanceType::NONE)
		{
			optionSustainedPerformanceMode.initDefault(0);
			optionSustainedPerformanceMode.isConst = true;
		}
	}
	if(androidSdk < 11)
	{
		// never run ctx in onPaused state on Android 2.3
		optionPauseUnfocused.isConst = true;
	}
	if(androidSdk < 27) // use safer value for devices defaulting to OpenSL ES
	{
		optionSoundBuffers.initDefault(4);
	}
	#endif

	if(!ctx.mainScreen().frameRateIsReliable())
	{
		optionFrameRate.initDefault(60);
	}

	if(!EmuApp::autoSaveStateDefault)
	{
		optionAutoSaveState.initDefault(false);
	}

	if(!EmuApp::hasIcon)
	{
		optionNotificationIcon.initDefault(false);
		optionNotificationIcon.isConst = true;
	}

	if(EmuSystem::forcedSoundRate)
	{
		optionSoundRate.initDefault(EmuSystem::forcedSoundRate);
		optionSoundRate.isConst = true;
	}

	if(!EmuSystem::hasSound)
	{
		optionSound.initDefault(0);
	}

	if(EmuSystem::constFrameRate)
	{
		optionFrameRate.isConst = true;
	}
}

void EmuApp::applyFontSize(Window &win)
{
	auto settings = fontSettings(win);
	logMsg("setting up font with pixel height:%d", settings.pixelHeight());
	viewManager.defaultFace().setFontSettings(renderer, settings);
	viewManager.defaultBoldFace().setFontSettings(renderer, settings);
}

IG::FontSettings EmuApp::fontSettings(Window &win) const
{
	float size = optionFontSize / 1000.;
	return {win.heightScaledMMInPixels(size)};
}

void EmuApp::writeRecentContent(FileIO &io)
{
	size_t strSizes = 0;
	for(const auto &e : recentContentList)
	{
		strSizes += 2;
		strSizes += e.path.size();
	}
	logMsg("writing recent list");
	writeOptionValueHeader(io, CFGKEY_RECENT_GAMES, strSizes);
	for(const auto &e : recentContentList)
	{
		auto len = e.path.size();
		io.write((uint16_t)len);
		io.write(e.path.data(), len);
	}
}

void EmuApp::readRecentContent(IG::ApplicationContext ctx, MapIO &io, size_t readSize_)
{
	auto readSize = readSize_;
	while(readSize && !recentContentList.isFull())
	{
		if(readSize < 2)
		{
			logMsg("expected string length but only %zu bytes left", readSize);
			break;
		}

		auto len = io.get<uint16_t>();
		readSize -= 2;

		if(len > readSize)
		{
			logMsg("string length %d longer than %zu bytes left", len, readSize);
			break;
		}

		FS::PathString path{};
		auto bytesRead = io.readSized(path, len);
		if(bytesRead == -1)
		{
			logErr("error reading string option");
			return;
		}
		if(!bytesRead)
			continue; // don't add empty paths
		readSize -= len;
		auto displayName = system().contentDisplayNameForPath(path);
		if(displayName.empty())
		{
			logMsg("skipping missing recent content:%s", path.data());
			continue;
		}
		RecentContentInfo info{path, displayName};
		const auto &added = recentContentList.emplace_back(info);
		logMsg("added game to recent list:%s, name:%s", added.path.data(), added.name.data());
	}

	if(readSize)
	{
		logMsg("skipping excess %zu bytes", readSize);
	}
}

std::pair<IG::FloatSeconds, bool> EmuApp::setFrameTime(VideoSystem vidSys, IG::FloatSeconds time)
{
	auto wantedTime = time;
	if(!time.count())
	{
		wantedTime = bestFrameTimeForScreen(vidSys);
	}
	if(!system().setFrameTime(vidSys, wantedTime))
	{
		return {wantedTime, false};
	}
	system().configFrameTime(soundRate());
	frameTimeOption(vidSys) = time.count();
	return {wantedTime, true};
}

IG::FloatSeconds EmuApp::frameTime(VideoSystem system) const
{
	auto &opt = frameTimeOption(system);
	if(opt.val)
		return IG::FloatSeconds(opt.val);
	return bestFrameTimeForScreen(system);
}

bool EmuApp::frameTimeIsConst(VideoSystem system) const
{
	return frameTimeOption(system).isConst;
}

void EmuApp::setFrameInterval(int val)
{
	optionFrameInterval = val;
};

int EmuApp::frameInterval() const
{
	if constexpr(Config::SCREEN_FRAME_INTERVAL)
		return optionFrameInterval;
	else
		return 1;
}

IG::PixelFormat EmuApp::videoEffectPixelFormat() const
{
	if(optionImageEffectPixelFormat)
		return (IG::PixelFormatID)optionImageEffectPixelFormat.val;
	return windowPixelFormat();
}

bool EmuApp::setVideoZoom(uint8_t val)
{
	if(!optionImageZoom.isValidVal(val))
		return false;
	optionImageZoom = val;
	logMsg("set video zoom: %d", int(optionImageZoom));
	emuVideoLayer.setZoom(val);
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
	return true;
}

bool EmuApp::setViewportZoom(uint8_t val)
{
	if(!optionViewportZoom.isValidVal(val))
		return false;
	optionViewportZoom = val;
	logMsg("set viewport zoom: %d", int(optionViewportZoom));
	auto &win = appContext().mainWindow();
	viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
	viewController().postDrawToEmuWindows();
	return true;
}

bool EmuApp::setOverlayEffectLevel(EmuVideoLayer &videoLayer, uint8_t val)
{
	if(!optionOverlayEffectLevel.isValidVal(val))
		return false;
	optionOverlayEffectLevel = val;
	videoLayer.setOverlayIntensity(val/100.);
	viewController().postDrawToEmuWindows();
	return true;
}

bool EmuApp::setVideoAspectRatio(double ratio)
{
	if(!optionAspectRatio.isValidVal(ratio))
		return false;
	optionAspectRatio = ratio;
	logMsg("set aspect ratio:%.2f", ratio);
	emuVideoLayer.setAspectRatio(ratio);
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
	return true;
}

double EmuApp::videoAspectRatio() const
{
	return optionAspectRatio;
}

void EmuApp::setShowsTitleBar(bool on)
{
	optionTitleBar = on;
	viewController().showNavView(optionTitleBar);
	viewController().placeElements();
}

void EmuApp::setIdleDisplayPowerSave(bool on)
{
	optionIdleDisplayPowerSave = on;
	appContext().setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
}

void EmuApp::setLowProfileOSNavMode(Tristate mode)
{
	optionLowProfileOSNav = (uint8_t)mode;
	applyOSNavStyle(appContext(), false);
}

void EmuApp::setHideOSNavMode(Tristate mode)
{
	optionHideOSNav = (uint8_t)mode;
	applyOSNavStyle(appContext(), false);
}

void EmuApp::setHideStatusBarMode(Tristate mode)
{
	optionHideStatusBar = (uint8_t)mode;
	applyOSNavStyle(appContext(), false);
}

void EmuApp::setEmuOrientation(Orientation o)
{
	optionEmuOrientation = o;
	logMsg("set game orientation: %s", orientationToStr(int(optionEmuOrientation)));
}

void EmuApp::setMenuOrientation(Orientation o)
{
	optionMenuOrientation = o;
	renderer.setWindowValidOrientations(appContext().mainWindow(), optionMenuOrientation);
	logMsg("set menu orientation: %s", IG::orientationToStr(int(optionMenuOrientation)));
}

void EmuApp::setShowsBundledGames(bool on)
{
	optionShowBundledGames = on;
	dispatchOnMainMenuItemOptionChanged();
}

void EmuApp::setShowsBluetoothScanItems(bool on)
{
	optionShowBluetoothScan = on;
	dispatchOnMainMenuItemOptionChanged();
}

void EmuApp::setLayoutBehindSystemUI(bool on)
{
	layoutBehindSystemUI = on;
	auto &win = appContext().mainWindow();
	viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
	viewController().postDrawToEmuWindows();
}

}

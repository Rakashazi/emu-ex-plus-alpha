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
#include "private.hh"
#include "privateInput.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

template<class T>
bool optionFrameTimeIsValid(T val)
{
	return !val || EmuSystem::frameTimeIsValid(EmuSystem::VIDSYS_NATIVE_NTSC, IG::FloatSeconds(val));
}

template<class T>
bool optionFrameTimePALIsValid(T val)
{
	return !val || EmuSystem::frameTimeIsValid(EmuSystem::VIDSYS_PAL, IG::FloatSeconds(val));
}

constexpr bool optionAspectRatioIsValid(double val)
{
	return val == 0. || (val >= 0.1 && val <= 10.);
}

DoubleOption optionAspectRatio{CFGKEY_GAME_ASPECT_RATIO, (double)EmuSystem::aspectRatioInfo[0], 0, optionAspectRatioIsValid};

Byte1Option optionImgFilter(CFGKEY_GAME_IMG_FILTER, 1, 0);
Byte1Option optionImgEffect(CFGKEY_IMAGE_EFFECT, 0, 0, optionIsValidWithMax<lastImageEffectIdValue>);
Byte1Option optionOverlayEffect(CFGKEY_OVERLAY_EFFECT, 0, 0, optionIsValidWithMax<VideoImageOverlay::MAX_EFFECT_VAL>);
Byte1Option optionOverlayEffectLevel(CFGKEY_OVERLAY_EFFECT_LEVEL, 25, 0, optionIsValidWithMax<100>);

constexpr bool imageEffectPixelFormatIsValid(uint8_t val)
{
	switch(val)
	{
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGBA8888:
			return true;
	}
	return false;
}

Byte1Option optionImageEffectPixelFormat(CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT, IG::PIXEL_NONE, 0, imageEffectPixelFormatIsValid);

Byte1Option optionVideoImageBuffers{CFGKEY_VIDEO_IMAGE_BUFFERS, 0, 0,
	optionIsValidWithMax<2>};

bool isValidOption2DO(_2DOrigin val)
{
	return val.isValid() && val != C2DO;
}
bool isValidOption2DOCenterBtn(_2DOrigin val)
{
	return val.isValid() && !val.onYCenter();
}

Byte1Option optionFrameInterval
	{CFGKEY_FRAME_INTERVAL,	1, !Config::envIsIOS, optionIsValidWithMinMax<1, 4>};
Byte1Option optionSkipLateFrames{CFGKEY_SKIP_LATE_FRAMES, 1, 0};
DoubleOption optionFrameRate{CFGKEY_FRAME_RATE, 0, 0, optionFrameTimeIsValid};
DoubleOption optionFrameRatePAL{CFGKEY_FRAME_RATE_PAL, 1./50., !EmuSystem::hasPALVideoSystem, optionFrameTimePALIsValid};

bool optionImageZoomIsValid(uint8_t val)
{
	return val == optionImageZoomIntegerOnly || val == optionImageZoomIntegerOnlyY
		|| (val >= 10 && val <= 100);
}
Byte1Option optionImageZoom
		(CFGKEY_IMAGE_ZOOM, 100, 0, optionImageZoomIsValid);
Byte1Option optionViewportZoom(CFGKEY_VIEWPORT_ZOOM, 100, 0, optionIsValidWithMinMax<50, 100>);
Byte1Option optionShowOnSecondScreen{CFGKEY_SHOW_ON_2ND_SCREEN, 1, 0};

Byte1Option optionTextureBufferMode{CFGKEY_TEXTURE_BUFFER_MODE, 0};

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

	EmuSystem::initOptions(*this);
}

void EmuApp::applyFontSize(Window &win)
{
	float size = optionFontSize / 1000.;
	logMsg("setting up font size %f", (double)size);
	viewManager.defaultFace().setFontSettings(renderer, IG::FontSettings(win.heightScaledMMInPixels(size)));
	viewManager.defaultBoldFace().setFontSettings(renderer, IG::FontSettings(win.heightScaledMMInPixels(size)));
}

void EmuApp::writeRecentContent(IO &io)
{
	unsigned strSizes = 0;
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

void EmuApp::readRecentContent(IG::ApplicationContext ctx, IO &io, unsigned readSize_)
{
	int readSize = readSize_;
	while(readSize && !recentContentList.isFull())
	{
		if(readSize < 2)
		{
			logMsg("expected string length but only %d bytes left", readSize);
			break;
		}

		auto len = io.get<uint16_t>();
		readSize -= 2;

		if(len > readSize)
		{
			logMsg("string length %d longer than %d bytes left", len, readSize);
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
		auto displayName = EmuSystem::contentDisplayNameForPath(ctx, path);
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
		logMsg("skipping excess %d bytes", readSize);
	}
}

uint8_t currentFrameInterval()
{
	if constexpr(Config::SCREEN_FRAME_INTERVAL)
		return optionFrameInterval;
	else
		return 1;
}

IG::PixelFormat EmuApp::imageEffectPixelFormat() const
{
	if(optionImageEffectPixelFormat)
		return (IG::PixelFormatID)optionImageEffectPixelFormat.val;
	return windowPixelFormat();
}

void EmuApp::setVideoZoom(uint8_t val)
{
	optionImageZoom = val;
	logMsg("set video zoom: %d", int(optionImageZoom));
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
}

void EmuApp::setViewportZoom(uint8_t val)
{
	optionViewportZoom = val;
	logMsg("set viewport zoom: %d", int(optionViewportZoom));
	viewController().startMainViewportAnimation();
}

void EmuApp::setOverlayEffectLevel(EmuVideoLayer &videoLayer, uint8_t val)
{
	optionOverlayEffectLevel = val;
	videoLayer.setOverlayIntensity(val/100.);
	viewController().postDrawToEmuWindows();
}

void EmuApp::setVideoAspectRatio(double val)
{
	optionAspectRatio = val;
	logMsg("set aspect ratio: %.2f", val);
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
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

}

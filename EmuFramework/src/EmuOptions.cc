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
#include <emuframework/MainMenuView.hh>
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/VController.hh>
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

constexpr SystemLogger log{"App"};

void EmuApp::initOptions(IG::ApplicationContext ctx)
{
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
	if(androidSdk < 17)
	{
		optionShowOnSecondScreen.isConst = true;
	}
	else
	{
		optionShowBluetoothScan.initDefault(0);
	}
	if(androidSdk < 11)
	{
		// never run ctx in onPaused state on Android 2.3
		optionPauseUnfocused.isConst = true;
	}
	if(androidSdk < 27) // use safer value for devices defaulting to OpenSL ES
	{
		audio().soundBuffers = audio().defaultSoundBuffers = 4;
	}
	#endif

	if(!EmuApp::hasIcon)
	{
		optionNotificationIcon.initDefault(false);
		optionNotificationIcon.isConst = true;
	}
}

void EmuApp::applyFontSize(Window &win)
{
	auto settings = fontSettings(win);
	log.info("setting up font with pixel height:{}", settings.pixelHeight());
	viewManager.defaultFace.setFontSettings(renderer, settings);
	viewManager.defaultBoldFace.setFontSettings(renderer, settings);
}

IG::FontSettings EmuApp::fontSettings(Window &win) const
{
	float size = optionFontSize / 1000.;
	return {win.heightScaledMMInPixels(size)};
}

void EmuApp::setFrameInterval(int val)
{
	log.info("set frame interval:{}", val);
	optionFrameInterval = val;
};

int EmuApp::frameInterval() const
{
	return optionFrameInterval;
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
	log.info("set video zoom:{}", int(optionImageZoom));
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
	log.info("set viewport zoom:{}", int(optionViewportZoom));
	auto &win = appContext().mainWindow();
	viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
	viewController().postDrawToEmuWindows();
	return true;
}

void EmuApp::setContentRotation(IG::Rotation r)
{
	contentRotation_ = r;
	updateContentRotation();
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
}

void EmuApp::updateContentRotation()
{
	if(contentRotation_ == Rotation::ANY)
		emuVideoLayer.setRotation(system().contentRotation());
	else
		emuVideoLayer.setRotation(contentRotation_);
}

bool EmuApp::setOverlayEffectLevel(EmuVideoLayer &videoLayer, uint8_t val)
{
	if(!optionOverlayEffectLevel.isValidVal(val))
		return false;
	optionOverlayEffectLevel = val;
	videoLayer.setOverlayIntensity(val / 100.f);
	viewController().postDrawToEmuWindows();
	return true;
}

bool isValidAspectRatio(float val)
{
	return val == -1.f || (val >= 0.1f && val <= 10.f);
}

bool EmuApp::setVideoAspectRatio(float ratio)
{
	if(ratio == 0.f)
		ratio = viewController().emuWindow().size().ratio<float>();
	if(!isValidAspectRatio(ratio))
		return false;
	log.info("set aspect ratio:{:g}", ratio);
	if(viewController().emuWindow().isLandscape())
		emuVideoLayer.landscapeAspectRatio = ratio;
	else
		emuVideoLayer.portraitAspectRatio = ratio;
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
	return true;
}

float EmuApp::videoAspectRatio() const
{
	return viewController().emuWindow().isLandscape() ? emuVideoLayer.landscapeAspectRatio : emuVideoLayer.portraitAspectRatio;
}

float EmuApp::defaultVideoAspectRatio() const
{
	return EmuSystem::aspectRatioInfos()[0].asFloat();
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

void EmuApp::setEmuOrientation(Orientations o)
{
	optionEmuOrientation = o;
	log.info("set game orientation:{}", asString(o).data());
}

void EmuApp::setMenuOrientation(Orientations o)
{
	optionMenuOrientation = o;
	renderer.setWindowValidOrientations(appContext().mainWindow(), o);
	log.info("set menu orientation:{}", asString(o).data());
}

void EmuApp::setShowsBundledGames(bool on)
{
	optionShowBundledGames = on;
	viewController().mainMenu().reloadItems();
}

void EmuApp::setShowsBluetoothScanItems(bool on)
{
	optionShowBluetoothScan = on;
	viewController().mainMenu().reloadItems();
}

void EmuApp::setLayoutBehindSystemUI(bool on)
{
	layoutBehindSystemUI = on;
	auto &win = appContext().mainWindow();
	viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
	viewController().postDrawToEmuWindows();
}

}

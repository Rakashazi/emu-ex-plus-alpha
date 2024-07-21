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

#include <emuframework/EmuOptions.hh>
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
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"App"};

void EmuApp::initOptions([[maybe_unused]] IG::ApplicationContext ctx)
{
	#ifdef CONFIG_OS_IOS
	if(ctx.deviceIsIPad())
		fontSize.resetDefault(5000);
	#endif

	#ifdef __ANDROID__
	auto androidSdk = ctx.androidSDK();
	if(!ctx.hasHardwareNavButtons() && androidSdk >= 19)
	{
		hidesOSNav.resetDefault(InEmuTristate::InEmu);
	}
	if(androidSdk >= 11)
	{
		showsNotificationIcon.resetDefault(false);
	}
	if(androidSdk >= 17)
	{
		showsBluetoothScan.resetDefault(false);
	}
	if(androidSdk < 27) // use safer value for devices defaulting to OpenSL ES
	{
		audio.soundBuffers = audio.defaultSoundBuffers = 4;
	}
	#endif
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
	float size = fontSize / 1000.f;
	return {win.heightScaledMMInPixels(size)};
}

IG::PixelFormat EmuApp::videoEffectPixelFormat() const
{
	if(imageEffectPixelFormat.value() != PixelFormatId::Unset)
		return imageEffectPixelFormat.value();
	return windowPixelFormat();
}

bool EmuApp::setContentScale(uint8_t val)
{
	if(!videoLayer.scale.set(val))
		return false;
	log.info("set content scale:{}", val);
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
	return true;
}

bool EmuApp::setMenuScale(int8_t val)
{
	if(!menuScale.set(val))
		return false;
	log.info("set menu scale:{}", menuScale.value());
	viewController().placeElements();
	viewController().postDrawToEmuWindows();
	return true;
}

void EmuApp::setContentRotation(IG::Rotation r)
{
	contentRotation = r;
	updateContentRotation();
}

void EmuApp::updateVideoContentRotation()
{
	if(contentRotation == Rotation::ANY)
		videoLayer.setRotation(system().contentRotation());
	else
		videoLayer.setRotation(contentRotation);
}

void EmuApp::updateContentRotation()
{
	updateVideoContentRotation();
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
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
		videoLayer.landscapeAspectRatio = ratio;
	else
		videoLayer.portraitAspectRatio = ratio;
	viewController().placeEmuViews();
	viewController().postDrawToEmuWindows();
	return true;
}

float EmuApp::videoAspectRatio() const
{
	return viewController().emuWindow().isLandscape() ? videoLayer.landscapeAspectRatio : videoLayer.portraitAspectRatio;
}

float EmuApp::defaultVideoAspectRatio() const
{
	return EmuSystem::aspectRatioInfos()[0].asFloat();
}

void EmuApp::setShowsTitleBar(bool on)
{
	showsTitleBar = on;
	viewController().showNavView(on);
	viewController().placeElements();
}

void EmuApp::setIdleDisplayPowerSave(bool on)
{
	idleDisplayPowerSave = on;
	appContext().setIdleDisplayPowerSave(on);
}

void EmuApp::setLowProfileOSNavMode(InEmuTristate mode)
{
	lowProfileOSNav = mode;
	applyOSNavStyle(appContext(), false);
}

void EmuApp::setHideOSNavMode(InEmuTristate mode)
{
	hidesOSNav = mode;
	applyOSNavStyle(appContext(), false);
}

void EmuApp::setHideStatusBarMode(InEmuTristate mode)
{
	hidesStatusBar = mode;
	applyOSNavStyle(appContext(), false);
}

void EmuApp::setEmuOrientation(Orientations o)
{
	emuOrientation = o;
	log.info("set game orientation:{}", asString(o).data());
}

void EmuApp::setMenuOrientation(Orientations o)
{
	menuOrientation = o;
	renderer.setWindowValidOrientations(appContext().mainWindow(), o);
	log.info("set menu orientation:{}", asString(o).data());
}

void EmuApp::setShowsBundledGames(bool on)
{
	showsBundledGames = on;
	viewController().mainMenu().reloadItems();
}

void EmuApp::setShowsBluetoothScanItems(bool on)
{
	showsBluetoothScan = on;
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

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

#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuOptions.hh>
#include "configFile.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/util/ScopeGuard.hh>

namespace EmuEx
{

constexpr SystemLogger log{"ConfigFile"};

constexpr bool colorSpaceIsValid(Gfx::ColorSpace val)
{
	return val == Gfx::ColorSpace::SRGB;
}

void EmuApp::saveConfigFile(FileIO &io)
{
	if(!io)
	{
		log.info("not writing config file");
		return;
	}
	writeConfigHeader(io);
	recentContent.writeConfig(io);
	writeOptionValueIfNotDefault(io, imageEffectPixelFormat);
	writeOptionValueIfNotDefault(io, menuScale);
	writeOptionValueIfNotDefault(io, fontSize);
	writeOptionValueIfNotDefault(io, showsBluetoothScan);
	writeOptionValueIfNotDefault(io, showsNotificationIcon);
	writeOptionValueIfNotDefault(io, lowProfileOSNav);
	writeOptionValueIfNotDefault(io, hidesOSNav);
	writeOptionValueIfNotDefault(io, hidesStatusBar);
	writeOptionValueIfNotDefault(io, showsBundledGames);
	writeOptionValueIfNotDefault(io, frameInterval);
	writeOptionValueIfNotDefault(io, frameTimeSource);
	writeOptionValueIfNotDefault(io, idleDisplayPowerSave);
	writeOptionValueIfNotDefault(io, confirmOverwriteState);
	writeOptionValueIfNotDefault(io, systemActionsIsDefaultMenu);
	writeOptionValueIfNotDefault(io, pauseUnfocused);
	writeOptionValueIfNotDefault(io, emuOrientation);
	writeOptionValueIfNotDefault(io, menuOrientation);
	writeOptionValue(io, CFGKEY_BACK_NAVIGATION, viewManager.needsBackControlOption());
	writeOptionValue(io, CFGKEY_SWAPPED_GAMEPAD_CONFIM, swappedConfirmKeysOption());
	writeOptionValue(io, CFGKEY_AUDIO_SOLO_MIX, audio.manager.soloMixOption());
	writeOptionValueIfNotDefault(io, windowDrawableConfig.pixelFormat);
	writeOptionValueIfNotDefault(io, windowDrawableConfig.colorSpace);
	writeOptionValueIfNotDefault(io, renderPixelFormat);
	writeOptionValueIfNotDefault(io, textureBufferMode);
	writeOptionValueIfNotDefault(io, showOnSecondScreen);
	writeOptionValueIfNotDefault(io, showHiddenFilesInPicker);
	writeOptionValueIfNotDefault(io, showsTitleBar);
	if constexpr(MOGA_INPUT)
	{
		if(mogaManagerPtr)
			writeOptionValue(io, CFGKEY_MOGA_INPUT_SYSTEM, true);
	}
	writeOptionValueIfNotDefault(io, useSustainedPerformanceMode);
	writeOptionValueIfNotDefault(io, keepBluetoothActive);
	writeOptionValueIfNotDefault(io, notifyOnInputDeviceChange);
	if(appContext().hasTranslucentSysUI() && !doesLayoutBehindSystemUI())
		writeOptionValue(io, CFGKEY_LAYOUT_BEHIND_SYSTEM_UI, false);
	writeOptionValueIfNotDefault(io, contentRotation);
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_LANDSCAPE_ASPECT_RATIO, videoLayer.landscapeAspectRatio, defaultVideoAspectRatio());
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_PORTRAIT_ASPECT_RATIO, videoLayer.portraitAspectRatio, defaultVideoAspectRatio());
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_LANDSCAPE_OFFSET, videoLayer.landscapeOffset, 0);
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_PORTRAIT_OFFSET, videoLayer.portraitOffset, 0);
	writeOptionValueIfNotDefault(io, fastModeSpeed);
	writeOptionValueIfNotDefault(io, slowModeSpeed);
	writeOptionValueIfNotDefault(io, CFGKEY_FRAME_RATE, outputTimingManager.frameTimeOption(VideoSystem::NATIVE_NTSC), OutputTimingManager::autoOption);
	writeOptionValueIfNotDefault(io, CFGKEY_FRAME_RATE_PAL, outputTimingManager.frameTimeOption(VideoSystem::PAL), OutputTimingManager::autoOption);
	inputManager.vController.writeConfig(io);
	autosaveManager.writeConfig(io);
	rewindManager.writeConfig(io);
	audio.writeConfig(io);
	videoLayer.writeConfig(io);
	if(overrideScreenFrameRate)
		writeOptionValue(io, CFGKEY_OVERRIDE_SCREEN_FRAME_RATE, overrideScreenFrameRate);
	writeOptionValueIfNotDefault(io, allowBlankFrameInsertion);
	if(Config::Bluetooth::scanCache && !bluetoothAdapter.useScanCache)
		writeOptionValue(io, CFGKEY_BLUETOOTH_SCAN_CACHE, false);
	writeOptionValueIfNotDefault(io, cpuAffinityMask);
	writeOptionValueIfNotDefault(io, cpuAffinityMode);
	writeOptionValueIfNotDefault(io, presentMode);
	if(renderer.supportsPresentationTime())
		writeOptionValueIfNotDefault(io, CFGKEY_RENDERER_PRESENTATION_TIME, presentationTimeMode, PresentationTimeMode::basic);
	writeStringOptionValue(io, CFGKEY_LAST_DIR, contentSearchPath);
	writeStringOptionValue(io, CFGKEY_SAVE_PATH, system().userSaveDirectory());
	writeStringOptionValue(io, CFGKEY_SCREENSHOTS_PATH, userScreenshotPath);
	system().writeConfig(ConfigType::MAIN, io);
	inputManager.writeCustomKeyConfigs(io);
	inputManager.writeSavedInputDevices(appContext(), io);
}

EmuApp::ConfigParams EmuApp::loadConfigFile(IG::ApplicationContext ctx)
{
	auto configFilePath = FS::pathString(ctx.supportPath(), "config");
	// move config files from old locations
	if(Config::envIsLinux)
	{
		auto oldConfigFilePath = FS::pathString(ctx.assetPath(), "config");
		if(FS::exists(oldConfigFilePath))
		{
			log.info("moving config file from app path to support path");
			FS::rename(oldConfigFilePath, configFilePath);
		}
	}
	#ifdef CONFIG_OS_IOS
	if(ctx.isSystemApp())
	{
		const char *oldConfigDir = "/User/Library/Preferences/explusalpha.com";
		auto oldConfigFilePath = FS::pathString(oldConfigDir, EmuSystem::configFilename);
		if(FS::exists(oldConfigFilePath))
		{
			log.info("moving config file from prefs path to support path");
			FS::rename(oldConfigFilePath, configFilePath);
		}
		if(!FS::directoryItems(oldConfigDir))
		{
			log.info("removing old empty config directory");
			FS::remove(oldConfigDir);
		}
	}
	#endif
	ConfigParams appConfig{};
	Gfx::DrawableConfig pendingWindowDrawableConf{};
	readConfigKeys(FileUtils::bufferFromPath(configFilePath, {.test = true}),
		[&](auto key, auto &io) -> bool
		{
			switch(key)
			{
				default:
				{
					if(system().readConfig(ConfigType::MAIN, io, key))
						return true;
					if(inputManager.vController.readConfig(*this, io, key))
						return true;
					if(autosaveManager.readConfig(io, key))
						return true;
					if(rewindManager.readConfig(io, key))
						return true;
					if(audio.readConfig(io, key))
						return true;
					if(recentContent.readConfig(io, key, system()))
						return true;
					if(videoLayer.readConfig(io, key))
						return true;
					log.info("skipping key:{}", key);
					return false;
				}
				case CFGKEY_FRAME_INTERVAL: return readOptionValue(io, frameInterval);
				case CFGKEY_FRAME_RATE: return readOptionValue<FrameTime>(io, [&](auto &&val){outputTimingManager.setFrameTimeOption(VideoSystem::NATIVE_NTSC, val);});
				case CFGKEY_FRAME_RATE_PAL: return readOptionValue<FrameTime>(io, [&](auto &&val){outputTimingManager.setFrameTimeOption(VideoSystem::PAL, val);});
				case CFGKEY_LAST_DIR:
					return readStringOptionValue<FS::PathString>(io, [&](auto &&path){contentSearchPath = path;});
				case CFGKEY_FONT_Y_SIZE: return readOptionValue(io, fontSize);
				case CFGKEY_GAME_ORIENTATION: return readOptionValue(io, emuOrientation);
				case CFGKEY_MENU_ORIENTATION: return readOptionValue(io, menuOrientation);
				case CFGKEY_MENU_SCALE: return readOptionValue(io, menuScale);
				case CFGKEY_SHOW_ON_2ND_SCREEN: return readOptionValue(io, showOnSecondScreen);
				case CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT: return readOptionValue(io, imageEffectPixelFormat);
				case CFGKEY_RENDER_PIXEL_FORMAT: return EmuSystem::canRenderMultipleFormats() ? readOptionValue(io, renderPixelFormat) : false;
				case CFGKEY_SWAPPED_GAMEPAD_CONFIM:
					setSwappedConfirmKeys(readOptionValue<bool>(io));
					return true;
				case CFGKEY_PAUSE_UNFOCUSED: return readOptionValue(io, pauseUnfocused);
				case CFGKEY_NOTIFICATION_ICON: return canShowNotificationIcon(ctx) ? readOptionValue(io, showsNotificationIcon) : false;
				case CFGKEY_TITLE_BAR: return readOptionValue(io, showsTitleBar);
				case CFGKEY_BACK_NAVIGATION: return readOptionValue(io, viewManager.needsBackControl);
				case CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU: return readOptionValue(io, systemActionsIsDefaultMenu);
				case CFGKEY_IDLE_DISPLAY_POWER_SAVE: return readOptionValue(io, idleDisplayPowerSave);
				case CFGKEY_HIDE_STATUS_BAR: return readOptionValue(io, hidesStatusBar);
				case CFGKEY_LAYOUT_BEHIND_SYSTEM_UI:
					return ctx.hasTranslucentSysUI() ? readOptionValue(io, layoutBehindSystemUI) : false;
				case CFGKEY_CONFIRM_OVERWRITE_STATE: return readOptionValue(io, confirmOverwriteState);
				case CFGKEY_FAST_MODE_SPEED: return readOptionValue(io, fastModeSpeed);
				case CFGKEY_SLOW_MODE_SPEED: return readOptionValue(io, slowModeSpeed);
				case CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE: return readOptionValue(io, notifyOnInputDeviceChange);
				case CFGKEY_MOGA_INPUT_SYSTEM:
					return MOGA_INPUT ? readOptionValue<bool>(io, [&](auto on){setMogaManagerActive(on, false);}) : false;
				case CFGKEY_TEXTURE_BUFFER_MODE: return readOptionValue(io, textureBufferMode);
				case CFGKEY_LOW_PROFILE_OS_NAV: return readOptionValue(io, lowProfileOSNav);
				case CFGKEY_HIDE_OS_NAV: return readOptionValue(io, hidesOSNav);
				case CFGKEY_SUSTAINED_PERFORMANCE_MODE: return appContext().hasSustainedPerformanceMode()
					&& readOptionValue(io, useSustainedPerformanceMode);
				case CFGKEY_KEEP_BLUETOOTH_ACTIVE: return Config::Input::BLUETOOTH && readOptionValue(io, keepBluetoothActive);
				case CFGKEY_SHOW_BLUETOOTH_SCAN: return Config::Input::BLUETOOTH && readOptionValue(io, showsBluetoothScan);
				case CFGKEY_BLUETOOTH_SCAN_CACHE: return Config::Bluetooth::scanCache
					&& readOptionValue<bool>(io, [this](auto on){ bluetoothAdapter.useScanCache = on; });
				case CFGKEY_CPU_AFFINITY_MASK: return readOptionValue(io, cpuAffinityMask);
				case CFGKEY_CPU_AFFINITY_MODE: return readOptionValue(io, cpuAffinityMode);
				case CFGKEY_RENDERER_PRESENT_MODE: return readOptionValue(io, presentMode);
				case CFGKEY_RENDERER_PRESENTATION_TIME:
					return used(presentationTimeMode) ? readOptionValue(io, presentationTimeMode, [](auto m){return m <= lastEnum<PresentationTimeMode>;}) : false;
				case CFGKEY_FRAME_CLOCK: return readOptionValue(io, frameTimeSource);
				case CFGKEY_AUDIO_SOLO_MIX:
					audio.manager.setSoloMix(readOptionValue<bool>(io));
					return true;
				case CFGKEY_SAVE_PATH:
					return readStringOptionValue<FS::PathString>(io, [&](auto &&path){system().setUserSaveDirectory(path);});
				case CFGKEY_SCREENSHOTS_PATH: return readStringOptionValue(io, userScreenshotPath);
				case CFGKEY_SHOW_BUNDLED_GAMES: return EmuSystem::hasBundledGames ? readOptionValue(io, showsBundledGames) : false;
				case CFGKEY_WINDOW_PIXEL_FORMAT: return readOptionValue(io, pendingWindowDrawableConf.pixelFormat, windowPixelFormatIsValid);
				case CFGKEY_VIDEO_COLOR_SPACE: return readOptionValue(io, pendingWindowDrawableConf.colorSpace, colorSpaceIsValid);
				case CFGKEY_SHOW_HIDDEN_FILES: return readOptionValue(io, showHiddenFilesInPicker);
				case CFGKEY_OVERRIDE_SCREEN_FRAME_RATE: return readOptionValue(io, overrideScreenFrameRate);
				case CFGKEY_BLANK_FRAME_INSERTION: return readOptionValue(io, allowBlankFrameInsertion);
				case CFGKEY_CONTENT_ROTATION: return readOptionValue(io, contentRotation);
				case CFGKEY_VIDEO_LANDSCAPE_ASPECT_RATIO: return readOptionValue(io, videoLayer.landscapeAspectRatio, isValidAspectRatio);
				case CFGKEY_VIDEO_PORTRAIT_ASPECT_RATIO: return readOptionValue(io, videoLayer.portraitAspectRatio, isValidAspectRatio);
				case CFGKEY_VIDEO_LANDSCAPE_OFFSET: return readOptionValue(io, videoLayer.landscapeOffset, [](auto v){return v >= -4096 && v <= 4096;});
				case CFGKEY_VIDEO_PORTRAIT_OFFSET: return readOptionValue(io, videoLayer.portraitOffset, [](auto v){return v >= -4096 && v <= 4096;});
				case CFGKEY_INPUT_KEY_CONFIGS_V2: return inputManager.readCustomKeyConfig(io);
				case CFGKEY_INPUT_DEVICE_CONFIGS: return inputManager.readSavedInputDevices(io);
			}
			return false;
		});

	// apply any pending read options
	if(pendingWindowDrawableConf)
	{
		if(pendingWindowDrawableConf.colorSpace != Gfx::ColorSpace{} && pendingWindowDrawableConf.pixelFormat != IG::PixelFmtRGBA8888)
			pendingWindowDrawableConf.colorSpace = {};
		windowDrawableConfig = pendingWindowDrawableConf;
	}

	return appConfig;
}

void EmuApp::saveConfigFile(IG::ApplicationContext ctx)
{
	auto configFilePath = FS::pathString(ctx.supportPath(), "config");
	try
	{
		FileIO file{configFilePath, OpenFlags::newFile()};
		saveConfigFile(file);
	}
	catch(...)
	{
		log.error("error writing config file");
	}
}

}

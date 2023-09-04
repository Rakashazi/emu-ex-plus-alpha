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
#include "EmuOptions.hh"
#include "configFile.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/input/config.hh>
#include <imagine/bluetooth/sys.hh>
#include <imagine/util/ScopeGuard.hh>

namespace EmuEx
{

bool isValidAspectRatio(float val);
bool isValidFastSpeed(int16_t);
bool isValidSlowSpeed(int16_t);

constexpr bool windowPixelFormatIsValid(uint8_t val)
{
	switch(val)
	{
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGBA8888:
			return true;
		default: return false;
	}
}

constexpr bool renderPixelFormatIsValid(IG::PixelFormat val)
{
	return windowPixelFormatIsValid(val);
}

constexpr bool colorSpaceIsValid(Gfx::ColorSpace val)
{
	return val == Gfx::ColorSpace::SRGB;
}

void EmuApp::saveConfigFile(FileIO &io)
{
	if(!io)
	{
		logMsg("not writing config file");
		return;
	}
	writeConfigHeader(io);

	const auto cfgFileOptions = std::tie
	(
		optionImageZoom,
		optionViewportZoom,
		#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
		optionShowOnSecondScreen,
		#endif
		optionImgFilter,
		optionImgEffect,
		optionImageEffectPixelFormat,
		optionVideoImageBuffers,
		optionOverlayEffect,
		optionOverlayEffectLevel,
		optionFontSize,
		optionPauseUnfocused,
		optionConfirmOverwriteState,
		optionFrameInterval,
		optionNotificationIcon,
		optionTitleBar,
		optionIdleDisplayPowerSave,
		optionHideStatusBar,
		optionSystemActionsIsDefaultMenu,
		optionTextureBufferMode,
		#if defined __ANDROID__
		optionLowProfileOSNav,
		optionHideOSNav,
		#endif
		#ifdef CONFIG_INPUT_BLUETOOTH
		optionShowBluetoothScan,
		#endif
		optionShowBundledGames
	);

	std::apply([&](auto &...opt){ (writeOptionValue(io, opt), ...); }, cfgFileOptions);

	recentContent.writeConfig(io);
	writeOptionValueIfNotDefault(io, CFGKEY_GAME_ORIENTATION, optionEmuOrientation, Orientations{});
	writeOptionValueIfNotDefault(io, CFGKEY_MENU_ORIENTATION, optionMenuOrientation, Orientations{});
	writeOptionValue(io, CFGKEY_BACK_NAVIGATION, viewManager.needsBackControlOption());
	writeOptionValue(io, CFGKEY_SWAPPED_GAMEPAD_CONFIM, swappedConfirmKeysOption());
	writeOptionValue(io, CFGKEY_AUDIO_SOLO_MIX, audioManager().soloMixOption());
	writeOptionValue(io, CFGKEY_WINDOW_PIXEL_FORMAT, windowDrawablePixelFormatOption());
	writeOptionValue(io, CFGKEY_VIDEO_COLOR_SPACE, windowDrawableColorSpaceOption());
	writeOptionValue(io, CFGKEY_RENDER_PIXEL_FORMAT, renderPixelFormatOption());
	writeOptionValueIfNotDefault(io, CFGKEY_SHOW_HIDDEN_FILES, showHiddenFilesInPicker, false);
	if constexpr(MOGA_INPUT)
	{
		if(mogaManagerPtr)
			writeOptionValue(io, CFGKEY_MOGA_INPUT_SYSTEM, true);
	}
	if(used(useSustainedPerformanceMode) && appContext().hasSustainedPerformanceMode())
		writeOptionValueIfNotDefault(io, CFGKEY_SUSTAINED_PERFORMANCE_MODE, useSustainedPerformanceMode, false);
	if(used(keepBluetoothActive))
		writeOptionValueIfNotDefault(io, CFGKEY_KEEP_BLUETOOTH_ACTIVE, keepBluetoothActive, false);
	if(used(notifyOnInputDeviceChange))
		writeOptionValueIfNotDefault(io, CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE, notifyOnInputDeviceChange, true);
	if(appContext().hasTranslucentSysUI() && !doesLayoutBehindSystemUI())
		writeOptionValue(io, CFGKEY_LAYOUT_BEHIND_SYSTEM_UI, false);
	if(contentRotation_ != Rotation::ANY)
		writeOptionValue(io, CFGKEY_CONTENT_ROTATION, contentRotation_);
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_LANDSCAPE_ASPECT_RATIO, videoLayer().landscapeAspectRatio, defaultVideoAspectRatio());
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_PORTRAIT_ASPECT_RATIO, videoLayer().portraitAspectRatio, defaultVideoAspectRatio());
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_LANDSCAPE_OFFSET, videoLayer().landscapeOffset, 0);
	writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_PORTRAIT_OFFSET, videoLayer().portraitOffset, 0);
	writeOptionValueIfNotDefault(io, CFGKEY_FAST_MODE_SPEED, fastModeSpeed, defaultFastModeSpeed);
	writeOptionValueIfNotDefault(io, CFGKEY_SLOW_MODE_SPEED, slowModeSpeed, defaultSlowModeSpeed);
	writeOptionValueIfNotDefault(io, CFGKEY_FRAME_RATE, outputTimingManager.frameTimeOption(VideoSystem::NATIVE_NTSC), OutputTimingManager::autoOption);
	writeOptionValueIfNotDefault(io, CFGKEY_FRAME_RATE_PAL, outputTimingManager.frameTimeOption(VideoSystem::PAL), OutputTimingManager::autoOption);
	inputManager.vController.writeConfig(io);
	autosaveManager_.writeConfig(io);
	emuAudio.writeConfig(io);
	doIfUsed(overrideScreenFrameRate, [&](auto &rate)
	{
		writeOptionValueIfNotDefault(io, CFGKEY_OVERRIDE_SCREEN_FRAME_RATE, rate, FrameRate{0});
	});
	writeOptionValueIfNotDefault(io, CFGKEY_BLANK_FRAME_INSERTION, allowBlankFrameInsertion, false);
	if(videoBrightnessRGB != Gfx::Vec3{1.f, 1.f, 1.f})
		writeOptionValue(io, CFGKEY_VIDEO_BRIGHTNESS, videoBrightnessRGB);
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	if(!BluetoothAdapter::scanCacheUsage())
		writeOptionValue(io, CFGKEY_BLUETOOTH_SCAN_CACHE, false);
	#endif
	if(used(cpuAffinityMask) && cpuAffinityMask)
		writeOptionValue(io, CFGKEY_CPU_AFFINITY_MASK, cpuAffinityMask);
	if(used(cpuAffinityMode))
		writeOptionValueIfNotDefault(io, CFGKEY_CPU_AFFINITY_MODE, cpuAffinityMode, CPUAffinityMode::Auto);
	if(used(presentMode) && supportsPresentModes())
		writeOptionValueIfNotDefault(io, CFGKEY_RENDERER_PRESENT_MODE, presentMode, Gfx::PresentMode::Auto);
	if(used(usePresentationTime) && renderer.supportsPresentationTime())
		writeOptionValueIfNotDefault(io, CFGKEY_RENDERER_PRESENTATION_TIME, usePresentationTime, true);
	writeStringOptionValue(io, CFGKEY_LAST_DIR, contentSearchPath());
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
			logMsg("moving config file from app path to support path");
			FS::rename(oldConfigFilePath, configFilePath);
		}
	}
	#ifdef CONFIG_BASE_IOS
	if(ctx.isSystemApp())
	{
		const char *oldConfigDir = "/User/Library/Preferences/explusalpha.com";
		auto oldConfigFilePath = FS::pathString(oldConfigDir, EmuSystem::configFilename);
		if(FS::exists(oldConfigFilePath))
		{
			logMsg("moving config file from prefs path to support path");
			FS::rename(oldConfigFilePath, configFilePath);
		}
		if(!FS::directoryItems(oldConfigDir))
		{
			logMsg("removing old empty config directory");
			FS::remove(oldConfigDir);
		}
	}
	#endif
	ConfigParams appConfig{};
	Gfx::DrawableConfig pendingWindowDrawableConf{};
	readConfigKeys(FileUtils::bufferFromPath(configFilePath, {.test = true}),
		[&](auto key, auto size, auto &io) -> bool
		{
			switch(key)
			{
				default:
				{
					if(system().readConfig(ConfigType::MAIN, io, key, size))
						return true;
					if(inputManager.vController.readConfig(*this, io, key, size))
						return true;
					if(autosaveManager_.readConfig(io, key, size))
						return true;
					if(emuAudio.readConfig(io, key, size))
						return true;
					if(recentContent.readConfig(io, key, size, system()))
						return true;
					logMsg("skipping key %u", (unsigned)key);
					return false;
				}
				case CFGKEY_FRAME_INTERVAL: return optionFrameInterval.readFromIO(io, size);;
				case CFGKEY_FRAME_RATE: return readOptionValue<FrameTime>(io, size, [&](auto &&val){outputTimingManager.setFrameTimeOption(VideoSystem::NATIVE_NTSC, val);});
				case CFGKEY_FRAME_RATE_PAL: return readOptionValue<FrameTime>(io, size, [&](auto &&val){outputTimingManager.setFrameTimeOption(VideoSystem::PAL, val);});
				case CFGKEY_LAST_DIR:
					return readStringOptionValue<FS::PathString>(io, size, [&](auto &&path){setContentSearchPath(path);});
				case CFGKEY_FONT_Y_SIZE: return optionFontSize.readFromIO(io, size);
				case CFGKEY_GAME_ORIENTATION: return readOptionValue(io, size, optionEmuOrientation);
				case CFGKEY_MENU_ORIENTATION: return readOptionValue(io, size, optionMenuOrientation);
				case CFGKEY_GAME_IMG_FILTER: return optionImgFilter.readFromIO(io, size);
				case CFGKEY_IMAGE_ZOOM: return optionImageZoom.readFromIO(io, size);
				case CFGKEY_VIEWPORT_ZOOM: return optionViewportZoom.readFromIO(io, size);
				#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
				case CFGKEY_SHOW_ON_2ND_SCREEN: return optionShowOnSecondScreen.readFromIO(io, size);
				#endif
				case CFGKEY_IMAGE_EFFECT: return optionImgEffect.readFromIO(io, size);
				case CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT: return optionImageEffectPixelFormat.readFromIO(io, size);
				case CFGKEY_RENDER_PIXEL_FORMAT:
					setRenderPixelFormat(readOptionValue<IG::PixelFormat>(io, size, renderPixelFormatIsValid));
					return true;
				case CFGKEY_VIDEO_IMAGE_BUFFERS: return optionVideoImageBuffers.readFromIO(io, size);
				case CFGKEY_OVERLAY_EFFECT: return optionOverlayEffect.readFromIO(io, size);
				case CFGKEY_OVERLAY_EFFECT_LEVEL: return optionOverlayEffectLevel.readFromIO(io, size);
				case CFGKEY_RECENT_CONTENT: return recentContent.readLegacyConfig(io, system());
				case CFGKEY_SWAPPED_GAMEPAD_CONFIM:
					setSwappedConfirmKeys(readOptionValue<bool>(io, size));
					return true;
				case CFGKEY_PAUSE_UNFOCUSED: return optionPauseUnfocused.readFromIO(io, size);
				case CFGKEY_NOTIFICATION_ICON: return optionNotificationIcon.readFromIO(io, size);
				case CFGKEY_TITLE_BAR: return optionTitleBar.readFromIO(io, size);
				case CFGKEY_BACK_NAVIGATION:
					return readOptionValue(io, size, viewManager.needsBackControl);
				case CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU: return optionSystemActionsIsDefaultMenu.readFromIO(io, size);
				case CFGKEY_IDLE_DISPLAY_POWER_SAVE: return optionIdleDisplayPowerSave.readFromIO(io, size);
				case CFGKEY_HIDE_STATUS_BAR: return doIfUsed(optionHideStatusBar, [&](auto &opt){ return opt.readFromIO(io, size); });
				case CFGKEY_LAYOUT_BEHIND_SYSTEM_UI:
					return ctx.hasTranslucentSysUI() ? readOptionValue(io, size, layoutBehindSystemUI) : false;
				case CFGKEY_CONFIRM_OVERWRITE_STATE: return optionConfirmOverwriteState.readFromIO(io, size);
				case CFGKEY_FAST_MODE_SPEED: return readOptionValue(io, size, fastModeSpeed, isValidFastSpeed);
				case CFGKEY_SLOW_MODE_SPEED: return readOptionValue(io, size, slowModeSpeed, isValidSlowSpeed);
				#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
				case CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE: return used(notifyOnInputDeviceChange) ? readOptionValue(io, size, notifyOnInputDeviceChange) : false;
				#endif
				case CFGKEY_MOGA_INPUT_SYSTEM:
					return MOGA_INPUT ? readOptionValue<bool>(io, size, [&](auto on){setMogaManagerActive(on, false);}) : false;
				case CFGKEY_TEXTURE_BUFFER_MODE: return optionTextureBufferMode.readFromIO(io, size);
				#if defined __ANDROID__
				case CFGKEY_LOW_PROFILE_OS_NAV: return optionLowProfileOSNav.readFromIO(io, size);
				case CFGKEY_HIDE_OS_NAV: return optionHideOSNav.readFromIO(io, size);
				case CFGKEY_SUSTAINED_PERFORMANCE_MODE:
					return used(useSustainedPerformanceMode) ? readOptionValue(io, size, useSustainedPerformanceMode) : false;
				#endif
				#ifdef CONFIG_INPUT_BLUETOOTH
				case CFGKEY_KEEP_BLUETOOTH_ACTIVE: return used(keepBluetoothActive) ? readOptionValue(io, size, keepBluetoothActive) : false;
				case CFGKEY_SHOW_BLUETOOTH_SCAN: return optionShowBluetoothScan.readFromIO(io, size);
					#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
					case CFGKEY_BLUETOOTH_SCAN_CACHE: return readOptionValue<bool>(io, size, [](auto on){BluetoothAdapter::setScanCacheUsage(on);});
					#endif
				#endif
				case CFGKEY_CPU_AFFINITY_MASK:
					return used(cpuAffinityMask) ? readOptionValue(io, size, cpuAffinityMask) : false;
				case CFGKEY_CPU_AFFINITY_MODE:
					return used(cpuAffinityMode) ? readOptionValue(io, size, cpuAffinityMode, [](auto m){return m <= lastEnum<CPUAffinityMode>;}) : false;
				case CFGKEY_RENDERER_PRESENT_MODE:
					return used(presentMode) && supportsPresentModes() ? readOptionValue(io, size, presentMode, [](auto m){return m <= lastEnum<Gfx::PresentMode>;}) : false;
				case CFGKEY_RENDERER_PRESENTATION_TIME:
					return used(usePresentationTime) ? readOptionValue(io, size, usePresentationTime) : false;
				case CFGKEY_AUDIO_SOLO_MIX:
					audioManager().setSoloMix(readOptionValue<bool>(io, size));
					return true;
				case CFGKEY_SAVE_PATH:
					return readStringOptionValue<FS::PathString>(io, size, [&](auto &&path){system().setUserSaveDirectory(path);});
				case CFGKEY_SCREENSHOTS_PATH: return readStringOptionValue(io, size, userScreenshotPath);
				case CFGKEY_SHOW_BUNDLED_GAMES: return EmuSystem::hasBundledGames ? optionShowBundledGames.readFromIO(io, size) : false;
				case CFGKEY_WINDOW_PIXEL_FORMAT: return readOptionValue(io, size, pendingWindowDrawableConf.pixelFormat, windowPixelFormatIsValid);
				case CFGKEY_VIDEO_COLOR_SPACE: return readOptionValue(io, size, pendingWindowDrawableConf.colorSpace, colorSpaceIsValid);
				case CFGKEY_SHOW_HIDDEN_FILES: return readOptionValue(io, size, showHiddenFilesInPicker);
				case CFGKEY_OVERRIDE_SCREEN_FRAME_RATE: return readOptionValue(io, size, overrideScreenFrameRate);
				case CFGKEY_BLANK_FRAME_INSERTION: return readOptionValue(io, size, allowBlankFrameInsertion);
				case CFGKEY_CONTENT_ROTATION: return readOptionValue(io, size, contentRotation_, [](auto r){return r <= lastEnum<Rotation>;});
				case CFGKEY_VIDEO_LANDSCAPE_ASPECT_RATIO: return readOptionValue(io, size, videoLayer().landscapeAspectRatio, isValidAspectRatio);
				case CFGKEY_VIDEO_PORTRAIT_ASPECT_RATIO: return readOptionValue(io, size, videoLayer().portraitAspectRatio, isValidAspectRatio);
				case CFGKEY_VIDEO_LANDSCAPE_OFFSET: return readOptionValue(io, size, videoLayer().landscapeOffset, [](auto v){return v >= -4096 && v <= 4096;});
				case CFGKEY_VIDEO_PORTRAIT_OFFSET: return readOptionValue(io, size, videoLayer().portraitOffset, [](auto v){return v >= -4096 && v <= 4096;});
				case CFGKEY_VIDEO_BRIGHTNESS: return readOptionValue(io, size, videoBrightnessRGB);
				case CFGKEY_INPUT_KEY_CONFIGS_V2: return inputManager.readCustomKeyConfig(io);
				case CFGKEY_INPUT_DEVICE_CONFIGS: return inputManager.readSavedInputDevices(io);
			}
			return false;
		});

	// apply any pending read options
	if(pendingWindowDrawableConf)
	{
		if(pendingWindowDrawableConf.colorSpace != Gfx::ColorSpace{} && pendingWindowDrawableConf.pixelFormat != IG::PIXEL_RGBA8888)
			pendingWindowDrawableConf.colorSpace = {};
		windowDrawableConf = pendingWindowDrawableConf;
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
		logErr("error writing config file");
	}
}

}

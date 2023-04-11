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
#include "privateInput.hh"
#include "configFile.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/input/config.hh>
#include <imagine/util/ScopeGuard.hh>

namespace EmuEx
{

static constexpr int KEY_CONFIGS_HARD_LIMIT = 256;
static constexpr int INPUT_DEVICE_CONFIGS_HARD_LIMIT = 256;

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

static bool readKeyConfig(KeyConfigContainer &customKeyConfigs,
	MapIO &io, size_t &size, std::span<const KeyCategory> categorySpan)
{
	auto confs = io.get<uint8_t>(); // TODO: unused currently, use to pre-allocate memory for configs
	size--;
	if(!size)
		return false;

	while(size)
	{
		KeyConfig keyConf{};

		keyConf.map = (Input::Map)io.get<uint8_t>();
		size--;
		if(!size)
			return false;

		auto nameLen = io.get<uint8_t>();
		size--;
		if(size < nameLen)
			return false;

		if(io.readSized(keyConf.name, nameLen) != nameLen)
			return false;
		size -= nameLen;
		if(!size)
			return false;

		auto categories = io.get<uint8_t>();
		size--;
		if(categories > categorySpan.size())
		{
			return false;
		}

		for(auto i : iotaCount(categories))
		{
			if(!size)
				return false;

			auto categoryIdx = io.get<uint8_t>();
			size--;
			if(categoryIdx >= categorySpan.size())
			{
				return false;
			}
			if(size < 2)
			{
				return false;
			}
			auto &cat = categorySpan[categoryIdx];
			auto catSize = io.get<uint16_t>();
			size -= 2;
			if(size < catSize)
				return false;

			if(catSize > cat.keys() * sizeof(KeyConfig::Key))
				return false;
			auto key = keyConf.key(cat);
			if(io.read(static_cast<void*>(key), catSize) != catSize)
				return false;
			size -= catSize;

			// verify keys
			{
				const auto keyMax = Input::KeyEvent::mapNumKeys(keyConf.map);
				for(auto i : iotaCount(cat.keys()))
				{
					if(key[i] >= keyMax)
					{
						logWarn("key code 0x%X out of range for map type %d", key[i], (int)keyConf.map);
						key[i] = 0;
					}
				}
			}

			logMsg("read category %d", categoryIdx);
		}

		logMsg("read key config %s", keyConf.name.data());
		customKeyConfigs.emplace_back(std::make_unique<KeyConfig>(keyConf));

		if(customKeyConfigs.size() == KEY_CONFIGS_HARD_LIMIT)
		{
			logWarn("reached custom key config hard limit:%d", KEY_CONFIGS_HARD_LIMIT);
			break;
		}
	}
	return true;
}

static bool readInputDeviceConfig(InputDeviceSavedConfigContainer &savedInputDevs,
	MapIO &io, size_t &size, const KeyConfigContainer &customKeyConfigs)
{
	auto confs = io.get<uint8_t>(); // TODO: unused currently, use to pre-allocate memory for configs
	size--;
	if(!size)
		return false;

	while(size)
	{
		InputDeviceSavedConfig devConf;

		auto enumIdWithFlags = io.get<uint8_t>();
		size--;
		if(!size)
			return false;
		devConf.handleUnboundEvents = enumIdWithFlags & devConf.HANDLE_UNBOUND_EVENTS_FLAG;
		devConf.enumId = enumIdWithFlags & devConf.ENUM_ID_MASK;

		devConf.enabled = io.get<uint8_t>();
		size--;
		if(!size)
			return false;

		devConf.player = io.get<uint8_t>();
		if(devConf.player != InputDeviceConfig::PLAYER_MULTI && devConf.player > EmuSystem::maxPlayers)
		{
			logWarn("player %d out of range", devConf.player);
			devConf.player = 0;
		}
		size--;
		if(!size)
			return false;

		devConf.joystickAxisAsDpadBits = io.get<uint8_t>();
		size--;
		if(!size)
			return false;

		#ifdef CONFIG_INPUT_ICADE
		devConf.iCadeMode = io.get<uint8_t>();
		size--;
		if(!size)
			return false;
		#endif

		auto nameLen = io.get<uint8_t>();
		size--;
		if(size < nameLen)
			return false;

		io.readSized(devConf.name, nameLen);
		size -= nameLen;
		if(!size)
			return false;

		auto keyConfMap = Input::validateMap(io.get<uint8_t>());
		size--;

		if(keyConfMap != Input::Map::UNKNOWN)
		{
			if(!size)
				return false;

			auto keyConfNameLen = io.get<uint8_t>();
			size--;
			if(size < keyConfNameLen)
				return false;

			char keyConfName[keyConfNameLen + 1];
			if(io.read(keyConfName, keyConfNameLen) != keyConfNameLen)
				return false;
			keyConfName[keyConfNameLen] = '\0';
			size -= keyConfNameLen;

			for(auto &ePtr : customKeyConfigs)
			{
				auto &e = *ePtr;
				if(e.map == keyConfMap && e.name == keyConfName)
				{
					logMsg("found referenced custom key config %s while reading input device config", keyConfName);
					devConf.keyConf = &e;
					break;
				}
			}

			if(!devConf.keyConf) // check built-in configs after user-defined ones
			{
				for(const auto &conf : KeyConfig::defaultConfigsForInputMap(keyConfMap))
				{
					if(conf.name == keyConfName)
					{
						logMsg("found referenced built-in key config %s while reading input device config", keyConfName);
						devConf.keyConf = &conf;
						break;
					}
				}
			}
		}

		if(!IG::containsIf(savedInputDevs, [&](const auto &confPtr){ return *confPtr == devConf;}))
		{
			logMsg("read input device config:%s, id:%d", devConf.name.data(), devConf.enumId);
			savedInputDevs.emplace_back(std::make_unique<InputDeviceSavedConfig>(devConf));
		}
		else
		{
			logMsg("ignoring duplicate input device config:%s, id:%d", devConf.name.data(), devConf.enumId);
		}

		if(savedInputDevs.size() == INPUT_DEVICE_CONFIGS_HARD_LIMIT)
		{
			logWarn("reached input device config hard limit:%d", INPUT_DEVICE_CONFIGS_HARD_LIMIT);
			return false;
		}
	}
	return true;
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
		optionSound,
		optionSoundVolume,
		optionSoundRate,
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
		optionEmuOrientation,
		optionMenuOrientation,
		optionConfirmOverwriteState,
		#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
		optionNotifyInputDeviceChange,
		#endif
		optionFrameInterval,
		optionSkipLateFrames,
		optionNotificationIcon,
		optionTitleBar,
		optionIdleDisplayPowerSave,
		optionHideStatusBar,
		optionSystemActionsIsDefaultMenu,
		optionTextureBufferMode,
		#if defined __ANDROID__
		optionLowProfileOSNav,
		optionHideOSNav,
		optionSustainedPerformanceMode,
		#endif
		#ifdef CONFIG_INPUT_BLUETOOTH
		optionKeepBluetoothActive,
		optionShowBluetoothScan,
		#endif
		optionSoundBuffers,
		optionAddSoundBuffersOnUnderrun,
		#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
		optionAudioAPI,
		#endif
		optionShowBundledGames
	);

	std::apply([&](auto &...opt){ (writeOptionValue(io, opt), ...); }, cfgFileOptions);

	writeRecentContent(io);
	writeOptionValue(io, CFGKEY_BACK_NAVIGATION, viewManager.needsBackControlOption());
	writeOptionValue(io, CFGKEY_SWAPPED_GAMEPAD_CONFIM, swappedConfirmKeysOption());
	writeOptionValue(io, CFGKEY_AUDIO_SOLO_MIX, audioManager().soloMixOption());
	writeOptionValue(io, CFGKEY_WINDOW_PIXEL_FORMAT, windowDrawablePixelFormatOption());
	writeOptionValue(io, CFGKEY_VIDEO_COLOR_SPACE, windowDrawableColorSpaceOption());
	writeOptionValue(io, CFGKEY_RENDER_PIXEL_FORMAT, renderPixelFormatOption());
	if(showHiddenFilesInPicker()) writeOptionValue(io, CFGKEY_SHOW_HIDDEN_FILES, true);
	if constexpr(MOGA_INPUT)
	{
		if(mogaManagerPtr)
			writeOptionValue(io, CFGKEY_MOGA_INPUT_SYSTEM, true);
	}
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
	vController.writeConfig(io);
	autosaveManager_.writeConfig(io);
	if(IG::used(usePresentationTime_) && !usePresentationTime_)
		writeOptionValue(io, CFGKEY_RENDERER_PRESENTATION_TIME, false);
	if(IG::used(forceMaxScreenFrameRate) && forceMaxScreenFrameRate)
		writeOptionValue(io, CFGKEY_FORCE_MAX_SCREEN_FRAME_RATE, true);
	if(videoBrightnessRGB != Gfx::Vec3{1.f, 1.f, 1.f})
		writeOptionValue(io, CFGKEY_VIDEO_BRIGHTNESS, videoBrightnessRGB);
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	if(!BluetoothAdapter::scanCacheUsage())
		writeOptionValue(io, CFGKEY_BLUETOOTH_SCAN_CACHE, false);
	#endif
	if(used(cpuAffinityMask) && cpuAffinityMask)
		writeOptionValue(io, CFGKEY_CPU_AFFINITY_MASK, cpuAffinityMask);

	if(customKeyConfigs.size())
	{
		auto categories = inputControlCategories();
		bool writeCategory[customKeyConfigs.size()][categories.size()];
		uint8_t writeCategories[customKeyConfigs.size()];
		std::fill_n(writeCategories, customKeyConfigs.size(), 0);
		// compute total size
		static_assert(sizeof(KeyConfig::name) <= 255, "key config name array is too large");
		size_t bytes = 2; // config key size
		bytes += 1; // number of configs
		for(uint8_t configs = 0; auto &ePtr : customKeyConfigs)
		{
			auto &e = *ePtr;
			bytes += 1; // input map type
			bytes += 1; // name string length
			bytes += e.name.size(); // name string
			bytes += 1; // number of categories present
			for(auto &cat : inputControlCategories())
			{
				bool write{};
				const auto key = e.key(cat);
				for(auto k : iotaCount(cat.keys()))
				{
					if(key[k]) // check if category has any keys defined
					{
						write = true;
						break;
					}
				}
				writeCategory[configs][std::distance(inputControlCategories().data(), &cat)] = write;
				if(!write)
				{
					logMsg("category:%s of key conf:%s skipped", cat.name.data(), e.name.data());
					continue;
				}
				writeCategories[configs]++;
				bytes += 1; // category index
				bytes += 2; // category key array size
				bytes += cat.keys() * sizeof(KeyConfig::Key); // keys array
			}
			configs++;
		}
		if(bytes > 0xFFFF)
		{
			bug_unreachable("excessive key config size, should not happen");
		}
		// write to config file
		logMsg("saving %d key configs, %zu bytes", (int)customKeyConfigs.size(), bytes);
		io.put(uint16_t(bytes));
		io.put(uint16_t(CFGKEY_INPUT_KEY_CONFIGS));
		io.put(uint8_t(customKeyConfigs.size()));
		for(uint8_t configs = 0; auto &ePtr : customKeyConfigs)
		{
			auto &e = *ePtr;
			logMsg("writing config %s", e.name.data());
			io.put(uint8_t(e.map));
			uint8_t nameLen = e.name.size();
			io.put(nameLen);
			io.write(e.name.data(), nameLen);
			io.put(writeCategories[configs]);
			for(auto &cat : inputControlCategories())
			{
				uint8_t catIdx = std::distance(inputControlCategories().data(), &cat);
				if(!writeCategory[configs][catIdx])
					continue;
				io.put(uint8_t(catIdx));
				uint16_t catSize = cat.keys() * sizeof(KeyConfig::Key);
				io.put(catSize);
				io.write(static_cast<void*>(e.key(cat)), catSize);
			}
			configs++;
		}
	}

	if(savedInputDevs.size())
	{
		// input device configs must be saved after key configs since
		// they reference the key configs when read back from the config file

		// compute total size
		unsigned bytes = 2; // config key size
		bytes += 1; // number of configs
		for(auto &ePtr : savedInputDevs)
		{
			auto &e = *ePtr;
			bytes += 1; // device id
			bytes += 1; // enabled
			bytes += 1; // player
			bytes += 1; // mapJoystickAxis1ToDpad
			#ifdef CONFIG_INPUT_ICADE
			bytes += 1; // iCade mode
			#endif
			bytes += 1; // name string length
			bytes += std::min((size_t)256, e.name.size()); // name string
			bytes += 1; // key config map
			if(e.keyConf)
			{
				bytes += 1; // name of key config string length
				bytes += e.keyConf->name.size(); // name of key config string
			}
		}
		if(bytes > 0xFFFF)
		{
			bug_unreachable("excessive input device config size, should not happen");
		}
		// write to config file
		logMsg("saving %d input device configs, %d bytes", (int)savedInputDevs.size(), bytes);
		io.put(uint16_t(bytes));
		io.put(uint16_t(CFGKEY_INPUT_DEVICE_CONFIGS));
		io.put(uint8_t(savedInputDevs.size()));
		for(auto &ePtr : savedInputDevs)
		{
			auto &e = *ePtr;
			logMsg("writing config %s, id %d", e.name.data(), e.enumId);
			uint8_t enumIdWithFlags = e.enumId;
			if(e.handleUnboundEvents)
				enumIdWithFlags |= e.HANDLE_UNBOUND_EVENTS_FLAG;
			io.put(uint8_t(enumIdWithFlags));
			io.put(uint8_t(e.enabled));
			io.put(uint8_t(e.player));
			io.put(uint8_t(e.joystickAxisAsDpadBits));
			#ifdef CONFIG_INPUT_ICADE
			io.put(uint8_t(e.iCadeMode));
			#endif
			uint8_t nameLen = std::min((size_t)256, e.name.size());
			io.put(nameLen);
			io.write(e.name.data(), nameLen);
			uint8_t keyConfMap = e.keyConf ? (uint8_t)e.keyConf->map : 0;
			io.put(keyConfMap);
			if(keyConfMap)
			{
				logMsg("has key conf %s, map %d", e.keyConf->name.data(), keyConfMap);
				uint8_t keyConfNameLen = e.keyConf->name.size();
				io.put(keyConfNameLen);
				io.write(e.keyConf->name.data(), keyConfNameLen);
			}
		}
	}

	writeStringOptionValue(io, CFGKEY_LAST_DIR, contentSearchPath());
	writeStringOptionValue(io, CFGKEY_SAVE_PATH, system().userSaveDirectory());
	writeStringOptionValue(io, CFGKEY_SCREENSHOTS_PATH, userScreenshotDir);

	system().writeConfig(ConfigType::MAIN, io);
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
	readConfigKeys(FileUtils::bufferFromPath(configFilePath, OpenFlagsMask::Test),
		[&](auto key, auto size, auto &io) -> bool
		{
			switch(key)
			{
				default:
				{
					if(system().readConfig(ConfigType::MAIN, io, key, size))
						return true;
					if(vController.readConfig(*this, io, key, size))
						return true;
					if(autosaveManager_.readConfig(io, key, size))
						return true;
					logMsg("skipping key %u", (unsigned)key);
					return false;
				}
				case CFGKEY_SOUND: return optionSound.readFromIO(io, size);
				case CFGKEY_SOUND_RATE: return optionSoundRate.readFromIO(io, size);
				case CFGKEY_FRAME_INTERVAL:
					return doIfUsed(optionFrameInterval, [&](auto &opt){return opt.readFromIO(io, size);});
				case CFGKEY_SKIP_LATE_FRAMES: return optionSkipLateFrames.readFromIO(io, size);
				case CFGKEY_FRAME_RATE: return readOptionValue<FloatSeconds>(io, size, [&](auto &&val){outputTimingManager.setFrameTimeOption(VideoSystem::NATIVE_NTSC, val);});
				case CFGKEY_FRAME_RATE_PAL: return readOptionValue<FloatSeconds>(io, size, [&](auto &&val){outputTimingManager.setFrameTimeOption(VideoSystem::PAL, val);});
				case CFGKEY_LAST_DIR:
					return readStringOptionValue<FS::PathString>(io, size, [&](auto &&path){setContentSearchPath(path);});
				case CFGKEY_FONT_Y_SIZE: return optionFontSize.readFromIO(io, size);
				case CFGKEY_GAME_ORIENTATION: return optionEmuOrientation.readFromIO(io, size);
				case CFGKEY_MENU_ORIENTATION: return optionMenuOrientation.readFromIO(io, size);
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
				case CFGKEY_RECENT_GAMES: return readRecentContent(ctx, io, size);
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
				case CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE: return optionNotifyInputDeviceChange.readFromIO(io, size);
				#endif
				case CFGKEY_MOGA_INPUT_SYSTEM:
					return MOGA_INPUT ? readOptionValue<bool>(io, size, [&](auto on){setMogaManagerActive(on, false);}) : false;
				case CFGKEY_TEXTURE_BUFFER_MODE: return optionTextureBufferMode.readFromIO(io, size);
				#if defined __ANDROID__
				case CFGKEY_LOW_PROFILE_OS_NAV: return optionLowProfileOSNav.readFromIO(io, size);
				case CFGKEY_HIDE_OS_NAV: return optionHideOSNav.readFromIO(io, size);
				case CFGKEY_SUSTAINED_PERFORMANCE_MODE: return optionSustainedPerformanceMode.readFromIO(io, size);
				#endif
				#ifdef CONFIG_INPUT_BLUETOOTH
				case CFGKEY_KEEP_BLUETOOTH_ACTIVE: return doIfUsed(optionKeepBluetoothActive, [&](auto &opt){ return opt.readFromIO(io, size); });
				case CFGKEY_SHOW_BLUETOOTH_SCAN: return optionShowBluetoothScan.readFromIO(io, size);
					#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
					case CFGKEY_BLUETOOTH_SCAN_CACHE: return readOptionValue<bool>(io, size, [](auto on){BluetoothAdapter::setScanCacheUsage(on);});
					#endif
				#endif
				case CFGKEY_CPU_AFFINITY_MASK:
					if(used(cpuAffinityMask))
						return readOptionValue(io, size, cpuAffinityMask);
					return false;
				case CFGKEY_SOUND_BUFFERS: return optionSoundBuffers.readFromIO(io, size);
				case CFGKEY_SOUND_VOLUME: return optionSoundVolume.readFromIO(io, size);
				case CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN: return optionAddSoundBuffersOnUnderrun.readFromIO(io, size);
				case CFGKEY_AUDIO_SOLO_MIX:
					audioManager().setSoloMix(readOptionValue<bool>(io, size));
					return true;
				#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
				case CFGKEY_AUDIO_API: return optionAudioAPI.readFromIO(io, size);
				#endif
				case CFGKEY_SAVE_PATH:
					return readStringOptionValue<FS::PathString>(io, size, [&](auto &&path){system().setUserSaveDirectory(path);});
				case CFGKEY_SCREENSHOTS_PATH: return readStringOptionValue(io, size, userScreenshotDir);
				case CFGKEY_SHOW_BUNDLED_GAMES: return EmuSystem::hasBundledGames ? optionShowBundledGames.readFromIO(io, size) : false;
				case CFGKEY_WINDOW_PIXEL_FORMAT: return readOptionValue(io, size, pendingWindowDrawableConf.pixelFormat, windowPixelFormatIsValid);
				case CFGKEY_VIDEO_COLOR_SPACE: return readOptionValue(io, size, pendingWindowDrawableConf.colorSpace, colorSpaceIsValid);
				case CFGKEY_SHOW_HIDDEN_FILES: return readOptionValue<bool>(io, size, [&](auto on){setShowHiddenFilesInPicker(on);});
				case CFGKEY_RENDERER_PRESENTATION_TIME: return readOptionValue<bool>(io, size, [&](auto on){setUsePresentationTime(on);});
				case CFGKEY_FORCE_MAX_SCREEN_FRAME_RATE: return readOptionValue<bool>(io, size, [&](auto on){setForceMaxScreenFrameRate(on);});
				case CFGKEY_CONTENT_ROTATION: return readOptionValue(io, size, contentRotation_, [](auto r){return r <= lastEnum<Rotation>;});
				case CFGKEY_VIDEO_LANDSCAPE_ASPECT_RATIO: return readOptionValue(io, size, videoLayer().landscapeAspectRatio, isValidAspectRatio);
				case CFGKEY_VIDEO_PORTRAIT_ASPECT_RATIO: return readOptionValue(io, size, videoLayer().portraitAspectRatio, isValidAspectRatio);
				case CFGKEY_VIDEO_LANDSCAPE_OFFSET: return readOptionValue(io, size, videoLayer().landscapeOffset, [](auto v){return v >= -4096 && v <= 4096;});
				case CFGKEY_VIDEO_PORTRAIT_OFFSET: return readOptionValue(io, size, videoLayer().portraitOffset, [](auto v){return v >= -4096 && v <= 4096;});
				case CFGKEY_VIDEO_BRIGHTNESS: return readOptionValue(io, size, videoBrightnessRGB);
				case CFGKEY_INPUT_KEY_CONFIGS: return readKeyConfig(customKeyConfigs, io, size, inputControlCategories());
				case CFGKEY_INPUT_DEVICE_CONFIGS: return readInputDeviceConfig(savedInputDevs, io, size, customKeyConfigs);
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
		FileIO file{configFilePath, OpenFlagsMask::New};
		saveConfigFile(file);
	}
	catch(...)
	{
		logErr("error writing config file");
	}
}

}

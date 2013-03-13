/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <util/number.h>
#include <ConfigFile.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>

static bool readKeyConfig(Io *io, uint16 &size)
{
	uint8 confs; // TODO: unused currently, use to pre-allocate memory for configs
	io->readVar(confs);
	size--;
	if(!size)
		return 0;

	while(size)
	{
		KeyConfig keyConf {0};

		io->readVarAsType<uint8>(keyConf.map);
		size--;
		if(!size)
			return 0;

		uint8 nameLen;
		io->readVar(nameLen);
		size--;
		if(size < nameLen)
			return 0;

		if(nameLen > sizeof(keyConf.name)-1)
			return 0;
		io->read(keyConf.name, nameLen);
		size -= nameLen;
		if(!size)
			return 0;

		uint8 categories;
		io->readVar(categories);
		size--;
		if(categories > EmuControls::categories)
		{
			return 0;
		}

		iterateTimes(categories, i)
		{
			if(!size)
				return 0;

			uint8 categoryIdx;
			io->readVar(categoryIdx);
			size--;
			if(categoryIdx >= EmuControls::categories)
			{
				return 0;
			}
			if(size < 2)
			{
				return 0;
			}

			uint16 catSize;
			io->readVar(catSize);
			size -= 2;
			if(size < catSize)
				return 0;

			if(catSize > EmuControls::category[categoryIdx].keys * sizeof(KeyConfig::Key))
				return 0;
			auto key = keyConf.key(EmuControls::category[categoryIdx]);
			io->read(key, catSize);
			size -= catSize;

			// verify keys
			{
				const auto keyMax = Input::Event::mapNumKeys(keyConf.map);
				iterateTimes(EmuControls::category[categoryIdx].keys, i)
				{
					if(key[i] >= keyMax)
					{
						logWarn("key code 0x%X out of range for map type %d", key[i], keyConf.map);
						key[i] = 0;
					}
				}
			}

			logMsg("read category %d", categoryIdx);
		}

		logMsg("read key config %s", keyConf.name);
		if(!customKeyConfig.addToEnd(keyConf))
			return 0;
	}
	return 1;
}

static bool readConfig2(Io *io)
{
	if(!io)
	{
		logMsg("no config file");
		return 0;
	}

	int dirChange = 0;

	uint8 blockSize;
	io->readVar(blockSize);
	auto fileBytesLeft = io->size() - 1;

	if(blockSize != 2)
	{
		logErr("can't read config with block size %d", blockSize);
		goto CLEANUP;
	}

	while(!io->eof() && fileBytesLeft >= 2)
	{
		uint16 size;
		io->readVar(size);
		#ifndef NDEBUG
		auto newFilePos = io->ftell() + size;
		#endif

		if(!size)
		{
			logMsg("invalid 0 size block, skipping rest of config");
			goto CLEANUP;
		}

		if(size > fileBytesLeft)
		{
			logErr("size of key exceeds rest of file, skipping rest of config");
			goto CLEANUP;
		}
		fileBytesLeft -= size;

		if(size < 3) // all blocks are at least a 2 byte key + 1 byte or more of data
		{
			logMsg("skipping %d byte block", size);
			if(io->seekRel(size) != OK)
			{
				logErr("unable to seek to next block, skipping rest of config");
				goto CLEANUP;
			}
			continue;
		}

		uint16 key;
		io->readVar(key);
		size -= 2;

		logMsg("got config key %u, size %u", key, size);
		switch(key)
		{
			default:
			{
				if(!EmuSystem::readConfig(io, key, size))
				{
					logMsg("skipping unknown key %u", (uint)key);
					if(io->seekRel(size) != OK)
					{
						logErr("unable to seek to next block, skipping rest of config");
						goto CLEANUP;
					}
				}
			}
			bcase CFGKEY_SOUND: optionSound.readFromIO(io, size);
			bcase CFGKEY_SOUND_RATE: optionSoundRate.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_DISPLAY: optionTouchCtrl.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_ALPHA: optionTouchCtrlAlpha.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_SIZE: optionTouchCtrlSize.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_DPAD_POS: optionTouchCtrlDpadPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_POS: optionTouchCtrlFaceBtnPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_MENU_POS: optionTouchCtrlMenuPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_FF_POS: optionTouchCtrlFFPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE: optionTouchCtrlBtnSpace.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER: optionTouchCtrlBtnStagger.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE: optionTouchDpadDeadzone.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_CENTER_BTN_POS: optionTouchCtrlCenterBtnPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS: optionTouchCtrlTriggerBtnPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY: optionTouchDpadDiagonalSensitivity.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE: optionTouchCtrlExtraXBtnSize.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE: optionTouchCtrlExtraYBtnSize.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW: optionTouchCtrlExtraYBtnSizeMultiRow.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES: optionTouchCtrlBoundingBoxes.readFromIO(io, size);
			bcase CFGKEY_AUTO_SAVE_STATE: optionAutoSaveState.readFromIO(io, size);
			bcase CFGKEY_CONFIRM_AUTO_LOAD_STATE: optionConfirmAutoLoadState.readFromIO(io, size);
			bcase CFGKEY_FRAME_SKIP: optionFrameSkip.readFromIO(io, size);
			#if defined(CONFIG_BASE_ANDROID)
			bcase CFGKEY_DITHER_IMAGE: optionDitherImage.readFromIO(io, size);
			#endif
			#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
			bcase CFGKEY_USE_OS_INPUT_METHOD: optionUseOSInputMethod.readFromIO(io, size);
			#endif
			bcase CFGKEY_LAST_DIR:
			{
				char lastDir[size+1];
				io->read(lastDir, size);
				lastDir[size] = 0;
				logMsg("switching to last dir %s", lastDir);
				if(FsSys::chdir(lastDir) == 0)
					dirChange = 1;
			}
			bcase CFGKEY_FONT_Y_PIXELS: logMsg("reading large font option"); optionLargeFonts.readFromIO(io, size);
			bcase CFGKEY_GAME_ORIENTATION: optionGameOrientation.readFromIO(io, size);
			bcase CFGKEY_MENU_ORIENTATION: optionMenuOrientation.readFromIO(io, size);
			bcase CFGKEY_GAME_IMG_FILTER: optionImgFilter.readFromIO(io, size);
			bcase CFGKEY_GAME_ASPECT_RATIO: optionAspectRatio.readFromIO(io, size);
			bcase CFGKEY_IMAGE_ZOOM: optionImageZoom.readFromIO(io, size);
			bcase CFGKEY_DPI: optionDPI.readFromIO(io, size);
			bcase CFGKEY_OVERLAY_EFFECT: optionOverlayEffect.readFromIO(io, size);
			bcase CFGKEY_OVERLAY_EFFECT_LEVEL: optionOverlayEffectLevel.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_VIRBRATE: optionVibrateOnPush.readFromIO(io, size);
			bcase CFGKEY_RECENT_GAMES: optionRecentGames.readFromIO(io, size);
			bcase CFGKEY_SWAPPED_GAMEPAD_CONFIM: optionSwappedGamepadConfirm.readFromIO(io, size);
			bcase CFGKEY_PAUSE_UNFOCUSED: optionPauseUnfocused.readFromIO(io, size);
			bcase CFGKEY_NOTIFICATION_ICON: optionNotificationIcon.readFromIO(io, size);
			bcase CFGKEY_TITLE_BAR: optionTitleBar.readFromIO(io, size);
			bcase CFGKEY_BACK_NAVIGATION: optionBackNavigation.readFromIO(io, size);
			bcase CFGKEY_REMEMBER_LAST_MENU: optionRememberLastMenu.readFromIO(io, size);
			bcase CFGKEY_IDLE_DISPLAY_POWER_SAVE: optionIdleDisplayPowerSave.readFromIO(io, size);
			bcase CFGKEY_SHOW_MENU_ICON: optionShowMenuIcon.readFromIO(io, size);
			bcase CFGKEY_HIDE_STATUS_BAR: optionHideStatusBar.readFromIO(io, size);
			bcase CFGKEY_CONFIRM_OVERWRITE_STATE: optionConfirmOverwriteState.readFromIO(io, size);
			#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
			bcase CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE: optionNotifyInputDeviceChange.readFromIO(io, size);
			#endif
			#if defined(CONFIG_BASE_ANDROID)
			bcase CFGKEY_LOW_PROFILE_OS_NAV: optionLowProfileOSNav.readFromIO(io, size);
			bcase CFGKEY_HIDE_OS_NAV: optionHideOSNav.readFromIO(io, size);
			bcase CFGKEY_REL_POINTER_DECEL: optionRelPointerDecel.readFromIO(io, size);
			#if defined(SUPPORT_ANDROID_DIRECT_TEXTURE)
			bcase CFGKEY_DIRECT_TEXTURE: optionDirectTexture.readFromIO(io, size);
			#endif
			#if CONFIG_ENV_ANDROID_MINSDK >= 9
			bcase CFGKEY_SURFACE_TEXTURE: optionSurfaceTexture.readFromIO(io, size);
			bcase CFGKEY_PROCESS_PRIORITY: optionProcessPriority.readFromIO(io, size);
			#endif
			bcase CFGKEY_GL_SYNC_HACK: optionGLSyncHack.readFromIO(io, size);
			#endif
			#ifdef CONFIG_BLUETOOTH
			bcase CFGKEY_KEEP_BLUETOOTH_ACTIVE: optionKeepBluetoothActive.readFromIO(io, size);
			bcase CFGKEY_BLUETOOTH_SCAN_CACHE: optionBlueToothScanCache.readFromIO(io, size);
			#endif
			#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
			bcase CFGKEY_SOUND_BUFFERS: optionSoundBuffers.readFromIO(io, size);
			#endif
			#ifdef CONFIG_AUDIO_OPENSL_ES
			bcase CFGKEY_SOUND_UNDERRUN_CHECK: optionSoundUnderrunCheck.readFromIO(io, size);
			#endif
			bcase CFGKEY_SAVE_PATH: logMsg("reading save path"); optionSavePath.readFromIO(io, size);
			#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
			bcase CFGKEY_BEST_COLOR_MODE_HINT: optionBestColorModeHint.readFromIO(io, size);
			#endif
			bcase CFGKEY_INPUT_KEY_CONFIGS:
			{
				if(!readKeyConfig(io, size))
				{
					logErr("error reading key configs from file");
				}
				if(size)
				{
					// skip leftover bytes
					logWarn("%d bytes leftover reading key configs due to invalid data", size);
					io->seekRel(size);
				}
			}
			bcase CFGKEY_INPUT_DEVICE_CONFIGS:
			{
				uint8 confs; // TODO: unused currently, use to pre-allocate memory for configs
				io->readVar(confs);
				size--;
				if(!size)
					break;

				while(size)
				{
					InputDeviceSavedConfig devConf;

					io->readVarAsType<uint8>(devConf.devId);
					size--;
					if(!size)
						break;
					if(devConf.devId > 32)
					{
						logWarn("unusually large device id %d, skipping rest of configs", devConf.devId);
						break;
					}

					io->readVarAsType<uint8>(devConf.enabled);
					size--;
					if(!size)
						break;

					io->readVarAsType<uint8>(devConf.player);
					if(devConf.player != InputDeviceConfig::PLAYER_MULTI && devConf.player > EmuSystem::maxPlayers)
					{
						logWarn("player %d out of range", devConf.player);
						devConf.player = 0;
					}
					size--;
					if(!size)
						break;

					io->readVarAsType<uint8>(devConf.mapJoystickAxis1ToDpad);
					size--;
					if(!size)
						break;

					#ifdef CONFIG_INPUT_ICADE
					io->readVarAsType<uint8>(devConf.iCadeMode);
					size--;
					if(!size)
						break;
					#endif

					uint8 nameLen;
					io->readVar(nameLen);
					size--;
					if(size < nameLen)
						break;

					if(nameLen > sizeof(devConf.name)-1)
						break;
					io->read(devConf.name, nameLen);
					size -= nameLen;
					if(!size)
						break;

					uint8 keyConfMap;
					io->readVar(keyConfMap);
					size--;

					if(keyConfMap)
					{
						if(!size)
							break;

						uint8 keyConfNameLen;
						io->readVar(keyConfNameLen);
						size--;
						if(size < keyConfNameLen)
							break;

						if(keyConfNameLen > sizeof(devConf.name)-1)
							break;
						char keyConfName[sizeof(devConf.name)] {0};
						io->read(keyConfName, keyConfNameLen);
						size -= keyConfNameLen;

						forEachInDLList(&customKeyConfig, e)
						{
							if(e.map == keyConfMap && string_equal(e.name, keyConfName))
							{
								logMsg("found referenced custom key config %s while reading input device config", keyConfName);
								devConf.keyConf = &e;
								break;
							}
						}

						if(!devConf.keyConf) // check built-in configs after user-defined ones
						{
							uint defaultConfs = 0;
							auto defaultConf = KeyConfig::defaultConfigsForInputMap(keyConfMap, defaultConfs);
							iterateTimes(defaultConfs, c)
							{
								if(string_equal(defaultConf[c].name, keyConfName))
								{
									logMsg("found referenced built-in key config %s while reading input device config", keyConfName);
									devConf.keyConf = &defaultConf[c];
									break;
								}
							}
						}
					}

					logMsg("read input device config %s, id %d", devConf.name, devConf.devId);
					if(!savedInputDevList.addToEnd(devConf))
						break;
				}
				if(size)
				{
					// skip leftover bytes
					logWarn("%d bytes leftover reading input device configs due to invalid data", size);
					io->seekRel(size);
				}
			}
		}

		#ifndef NDEBUG
		if(newFilePos != io->ftell())
		{
			bug_exit("expected to be at file pos %d instead of %d", (int)newFilePos, (int)io->ftell());
		}
		#endif
	}

	CLEANUP:
	delete io;
	return dirChange;
}

static OptionBase *cfgFileOption[] =
{
	&optionAutoSaveState,
	&optionConfirmAutoLoadState,
	&optionSound,
	&optionSoundRate,
	&optionAspectRatio,
	&optionImageZoom,
	&optionImgFilter,
	&optionOverlayEffect,
	&optionOverlayEffectLevel,
	&optionRelPointerDecel,
	&optionLargeFonts,
	&optionPauseUnfocused,
	&optionGameOrientation,
	&optionMenuOrientation,
	&optionTouchCtrl,
	&optionTouchCtrlAlpha,
	&optionTouchCtrlSize,
	&optionTouchDpadDeadzone,
	&optionTouchCtrlBtnSpace,
	&optionTouchCtrlBtnStagger,
	&optionTouchCtrlDpadPos,
	&optionTouchCtrlFaceBtnPos,
	&optionTouchCtrlCenterBtnPos,
	&optionTouchCtrlTriggerBtnPos,
	&optionTouchCtrlMenuPos,
	&optionTouchCtrlFFPos,
	&optionTouchCtrlExtraXBtnSize,
	&optionTouchCtrlExtraYBtnSize,
	&optionTouchCtrlExtraYBtnSizeMultiRow,
	&optionTouchDpadDiagonalSensitivity,
	&optionTouchCtrlBoundingBoxes,
	&optionShowMenuIcon,
	&optionSwappedGamepadConfirm,
	&optionConfirmOverwriteState,
	#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
	&optionNotifyInputDeviceChange,
	#endif
	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	&optionUseOSInputMethod,
	#endif
	&optionFrameSkip,
	&optionDPI,
	&optionVibrateOnPush,
	&optionRecentGames,
	&optionNotificationIcon,
	&optionTitleBar,
	&optionIdleDisplayPowerSave,
	&optionHideStatusBar,
#if defined(CONFIG_INPUT_ANDROID)
	&optionBackNavigation,
#endif
	&optionRememberLastMenu,
#ifdef CONFIG_BASE_ANDROID
	&optionLowProfileOSNav,
	&optionHideOSNav,
	&optionDitherImage,
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		&optionDirectTexture,
	#endif
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		&optionSurfaceTexture,
		&optionProcessPriority,
	#endif
	&optionGLSyncHack,
#endif
	#ifdef CONFIG_BLUETOOTH
	&optionKeepBluetoothActive,
	&optionBlueToothScanCache,
	#endif
	#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
	&optionSoundBuffers,
	#endif
	#ifdef CONFIG_AUDIO_OPENSL_ES
	&optionSoundUnderrunCheck,
	#endif
	#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
	&optionBestColorModeHint,
	#endif
};

static void writeConfig2(Io *io)
{
	if(!io)
	{
		logMsg("not writing config file");
		return;
	}

	uint8 blockHeaderSize = 2;
	io->writeVar(blockHeaderSize);

	forEachDInArray(cfgFileOption, e)
	{
		if(!e->isDefault())
		{
			io->writeVar((uint16)e->ioSize());
			e->writeToIO(io);
		}
	}

	if(customKeyConfig.size)
	{
		bool writeCategory[MAX_CUSTOM_KEY_CONFIGS][EmuControls::categories];
		uint8 writeCategories[MAX_CUSTOM_KEY_CONFIGS] {0};
		// compute total size
		static_assert(sizeof(KeyConfig::name) <= 255, "key config name array is too large");
		uint bytes = 2; // config key size
		uint8 configs = 0;
		bytes += 1; // number of configs
		forEachInDLList(&customKeyConfig, e)
		{
			bytes += 1; // input map type
			bytes += 1; // name string length
			bytes += strlen(e.name); // name string
			bytes += 1; // number of categories present
			iterateTimes(EmuControls::categories, cat)
			{
				bool write = 0;
				const auto key = e.key(EmuControls::category[cat]);
				iterateTimes(EmuControls::category[cat].keys, k)
				{
					if(key[k]) // check if category has any keys defined
					{
						write = 1;
						break;
					}
				}
				writeCategory[configs][cat] = write;
				if(!write)
				{
					logMsg("category %d of key conf %s skipped", cat, e.name);
					continue;
				}
				writeCategories[configs]++;
				bytes += 1; // category index
				bytes += 2; // category key array size
				bytes += EmuControls::category[cat].keys * sizeof(KeyConfig::Key); // keys array
			}
			configs++;
		}
		if(bytes > 0xFFFF)
		{
			bug_exit("excessive key config size, should not happen");
		}
		// write to config file
		logMsg("saving %d key configs, %d bytes", customKeyConfig.size, bytes);
		io->writeVar(uint16(bytes));
		io->writeVar((uint16)CFGKEY_INPUT_KEY_CONFIGS);
		io->writeVar((uint8)customKeyConfig.size);
		configs = 0;
		forEachInDLList(&customKeyConfig, e)
		{
			logMsg("writing config %s", e.name);
			io->writeVar(uint8(e.map));
			uint8 nameLen = strlen(e.name);
			io->writeVar(nameLen);
			io->fwrite(e.name, nameLen, 1);
			io->writeVar(writeCategories[configs]);
			iterateTimes(EmuControls::categories, cat)
			{
				if(!writeCategory[configs][cat])
					continue;
				io->writeVar((uint8)cat);
				uint16 catSize = EmuControls::category[cat].keys * sizeof(KeyConfig::Key);
				io->writeVar(catSize);
				io->fwrite(e.key(EmuControls::category[cat]), catSize, 1);
			}
			configs++;
		}
	}

	if(savedInputDevList.size)
	{
		// input device configs must be saved after key configs since
		// they reference the key configs when read back from the config file

		// compute total size
		static_assert(sizeof(InputDeviceSavedConfig::name) <= 255, "input device config name array is too large");
		uint bytes = 2; // config key size
		bytes += 1; // number of configs
		forEachInDLList(&savedInputDevList, e)
		{
			bytes += 1; // device id
			bytes += 1; // enabled
			bytes += 1; // player
			bytes += 1; // mapJoystickAxis1ToDpad
			#ifdef CONFIG_INPUT_ICADE
			bytes += 1; // iCade mode
			#endif
			bytes += 1; // name string length
			bytes += strlen(e.name); // name string
			bytes += 1; // key config map
			if(e.keyConf)
			{
				bytes += 1; // name of key config string length
				bytes += strlen(e.keyConf->name); // name of key config string
			}
		}
		if(bytes > 0xFFFF)
		{
			bug_exit("excessive input device config size, should not happen");
		}
		// write to config file
		logMsg("saving %d input device configs, %d bytes", savedInputDevList.size, bytes);
		io->writeVar((uint16)bytes);
		io->writeVar((uint16)CFGKEY_INPUT_DEVICE_CONFIGS);
		io->writeVar((uint8)savedInputDevList.size);
		forEachInDLList(&savedInputDevList, e)
		{
			logMsg("writing config %s, id %d", e.name, e.devId);
			io->writeVar((uint8)e.devId);
			io->writeVar((uint8)e.enabled);
			io->writeVar((uint8)e.player);
			io->writeVar((uint8)e.mapJoystickAxis1ToDpad);
			#ifdef CONFIG_INPUT_ICADE
				io->writeVar((uint8)e.iCadeMode);
			#endif
			uint8 nameLen = strlen(e.name);
			io->writeVar(nameLen);
			io->fwrite(e.name, nameLen, 1);
			uint8 keyConfMap = e.keyConf ? e.keyConf->map : 0;
			io->writeVar(keyConfMap);
			if(keyConfMap)
			{
				logMsg("has key conf %s, map %d", e.keyConf->name, keyConfMap);
				uint8 keyConfNameLen = strlen(e.keyConf->name);
				io->writeVar(keyConfNameLen);
				io->fwrite(e.keyConf->name, keyConfNameLen, 1);
			}
		}
	}

	uint len = strlen(FsSys::workDir());
	if(len > 32000)
	{
		logErr("option string too long to write");
	}
	else if(!string_equal(FsSys::workDir(), Base::storagePath()))
	{
		logMsg("saving current directory: %s", FsSys::workDir());
		io->writeVar((uint16)(2 + len));
		io->writeVar((uint16)CFGKEY_LAST_DIR);
		io->fwrite(FsSys::workDir(), len, 1);
	}

	optionSavePath.writeToIO(io);

	EmuSystem::writeConfig(io);

	delete io;
}

void loadConfigFile()
{
	FsSys::cPath configFilePath;
	#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	snprintf(configFilePath, sizeof(configFilePath), "%s/explusalpha.com/" CONFIG_FILE_NAME, Base::documentsPath());
	#else
	snprintf(configFilePath, sizeof(configFilePath), "%s/" CONFIG_FILE_NAME, Base::documentsPath());
	#endif
	if(!readConfig2(IoSys::open(configFilePath)))
	{
		FsSys::chdir(Base::storagePath());
	}
}

void saveConfigFile()
{
	FsSys::cPath configFilePath;
	#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	snprintf(configFilePath, sizeof(configFilePath), "%s/explusalpha.com", Base::documentsPath());
	FsSys::mkdir(configFilePath);
	/*#ifdef CONFIG_BASE_IOS_SETUID
	fixFilePermissions(configFilePath);
	#endif*/
	snprintf(configFilePath, sizeof(configFilePath), "%s/explusalpha.com/" CONFIG_FILE_NAME, Base::documentsPath());
	#else
	snprintf(configFilePath, sizeof(configFilePath), "%s/" CONFIG_FILE_NAME, Base::documentsPath());
	#endif
	writeConfig2(IoSys::create(configFilePath));
}

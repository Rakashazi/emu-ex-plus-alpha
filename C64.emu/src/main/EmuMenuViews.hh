#pragma once
#include <imagine/gui/TextEntry.hh>
#include <emuframework/OptionView.hh>
#include "VicePlugin.hh"

class SystemOptionView : public OptionView
{
	BoolMenuItem trueDriveEmu
	{
		"True Drive Emulation (TDE)",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionDriveTrueEmulation = item.on;
			setDriveTrueEmulation(item.on);
		}
	};

	BoolMenuItem autostartWarp
	{
		"Autostart Fast-forward",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionAutostartWarp = item.on;
			setAutostartWarp(item.on);
		}
	};

	BoolMenuItem autostartTDE
	{
		"Autostart Handles TDE",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionAutostartTDE = item.on;
			setAutostartTDE(item.on);
		}
	};

	BoolMenuItem cropNormalBorders
	{
		"Crop Normal Borders",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionCropNormalBorders = item.on;
			c64VidActiveX = 0; // force pixmap to update on next frame
		}
	};

	template <size_t S>
	void defaultModelInit(MultiChoiceSelectMenuItem &item, Byte1Option option, const char *(&modelStr)[S], int baseVal = 0)
	{
		auto model = option;
		if(model < baseVal || model >= (int)S + baseVal)
		{
			model = baseVal;
		}
		item.init(modelStr, model, S, baseVal);
	}

	MultiChoiceSelectMenuItem defaultC64Model
	{
		"Default C64 Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionC64Model = val;
			if(!EmuSystem::gameIsRunning() &&
				(currSystem == VICE_SYSTEM_C64 || currSystem == VICE_SYSTEM_C64SC))
			{
				setSysModel(optionC64Model.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultDTVModel
	{
		"Default DTV Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionDTVModel = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_C64DTV)
			{
				setSysModel(optionDTVModel.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultC128Model
	{
		"Default C128 Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionC128Model = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_C128)
			{
				setSysModel(optionC128Model.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultSuperCPUModel
	{
		"Default C64 SuperCPU Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionSuperCPUModel = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_SUPER_CPU)
			{
				setSysModel(optionSuperCPUModel.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultCBM2Model
	{
		"Default CBM-II 6x0 Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionCBM2Model = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_CBM2)
			{
				setSysModel(optionCBM2Model.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultCBM5x0Model
	{
		"Default CBM-II 5x0 Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionCBM5x0Model = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_CBM5X0)
			{
				setSysModel(optionCBM5x0Model.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultPetModel
	{
		"Default PET Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionPETModel = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_PET)
			{
				setSysModel(optionPETModel.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultPlus4Model
	{
		"Default Plus/4 Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionPlus4Model = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_PLUS4)
			{
				setSysModel(optionPlus4Model.val);
			}
		}
	};

	MultiChoiceSelectMenuItem defaultVIC20Model
	{
		"Default VIC-20 Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionVIC20Model = val;
			if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_VIC20)
			{
				setSysModel(optionVIC20Model.val);
			}
		}
	};

	MultiChoiceSelectMenuItem borderMode
	{
		"Borders",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionBorderMode = val;
			setBorderMode(val);
		}
	};

	void borderModeInit()
	{
		static const char *str[] =
		{
			"Normal",
			"Full",
			"Debug",
			"None"
		};
		auto mode = optionBorderMode.val;
		if(mode >= (int)IG::size(str))
		{
			mode = VICII_NORMAL_BORDERS;
		}
		borderMode.init(str, mode, IG::size(str));
	}

	MultiChoiceSelectMenuItem sidEngine
	{
		"SID Engine",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			assert(val <= (int)IG::size(sidEngineChoiceMap));
			logMsg("setting SID engine: %d", sidEngineChoiceMap[val]);
			optionSidEngine = sidEngineChoiceMap[val];
			setSidEngine(sidEngineChoiceMap[val]);
		}
	};

	template <size_t S>
	static void printSysPathMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "System File Path: %s", strlen(firmwareBasePath.data()) ? FS::basename(firmwareBasePath).data() : "Default");
	}

	FirmwarePathSelector systemFileSelector;
	char systemFilePathStr[256]{};
	TextMenuItem systemFilePath
	{
		"",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			systemFileSelector.init("System File Path", e);
			systemFileSelector.onPathChange =
				[this](const char *newPath)
				{
					printSysPathMenuEntryStr(systemFilePathStr);
					systemFilePath.compile(projP);
					sysFilePath[0] = firmwareBasePath;
					if(!strlen(newPath))
					{
						if(Config::envIsLinux && !Config::MACHINE_IS_PANDORA)
							popup.printf(5, false, "Using default paths:\n%s\n%s\n%s", Base::assetPath().data(), "~/.local/share/C64.emu", "/usr/share/games/vice");
						else
							popup.printf(4, false, "Using default path:\n%s/C64.emu", Base::storagePath().data());
					}
				};
			postDraw();
		}
	};

public:
	static constexpr int sidEngineChoiceMap[]
	{
		SID_ENGINE_FASTSID,
		#if defined HAVE_RESID
		SID_ENGINE_RESID,
		#endif
	};

private:
	void sidEngineInit()
	{
		static const char *str[] =
		{
			"FastSID",
			#if defined HAVE_RESID
			"ReSID",
			#endif
		};
		auto engine = intResource("SidEngine");
		logMsg("current SID engine: %d", engine);
		uint idx = 0;
		for(auto &e : sidEngineChoiceMap)
		{
			if(e == engine)
			{
				idx = &e - sidEngineChoiceMap;
				break;
			}
		}
		sidEngine.init(str, idx, IG::size(str));
	}

public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		cropNormalBorders.init(optionCropNormalBorders); item[items++] = &cropNormalBorders;
		borderModeInit(); item[items++] = &borderMode;
	}

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		OptionView::loadAudioItems(item, items);
		#ifdef HAVE_RESID
		sidEngineInit(); item[items++] = &sidEngine;
		#endif
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		defaultModelInit(defaultC64Model, optionC64Model, c64ModelStr); item[items++] = &defaultC64Model;
		defaultModelInit(defaultDTVModel, optionDTVModel, dtvModelStr); item[items++] = &defaultDTVModel;
		defaultModelInit(defaultC128Model, optionC128Model, c128ModelStr); item[items++] = &defaultC128Model;
		defaultModelInit(defaultSuperCPUModel, optionSuperCPUModel, superCPUModelStr); item[items++] = &defaultSuperCPUModel;
		defaultModelInit(defaultCBM2Model, optionCBM2Model, cbm2ModelStr, 2); item[items++] = &defaultCBM2Model;
		defaultModelInit(defaultCBM5x0Model, optionCBM5x0Model, cbm5x0ModelStr); item[items++] = &defaultCBM5x0Model;
		defaultModelInit(defaultPetModel, optionPETModel, petModelStr); item[items++] = &defaultPetModel;
		defaultModelInit(defaultPlus4Model, optionPlus4Model, plus4ModelStr); item[items++] = &defaultPlus4Model;
		defaultModelInit(defaultVIC20Model, optionVIC20Model, vic20ModelStr); item[items++] = &defaultVIC20Model;
		trueDriveEmu.init(optionDriveTrueEmulation); item[items++] = &trueDriveEmu;
		autostartTDE.init(optionAutostartTDE); item[items++] = &autostartTDE;
		autostartWarp.init(optionAutostartWarp); item[items++] = &autostartWarp;
		printSysPathMenuEntryStr(systemFilePathStr);
		systemFilePath.init(systemFilePathStr, true); item[items++] = &systemFilePath;
	}
};

constexpr int SystemOptionView::sidEngineChoiceMap[];

static const char *insertEjectMenuStr[] { "Insert File", "Eject" };
static bool hasC64DiskExtension(const char *name);
static bool hasC64TapeExtension(const char *name);
static bool hasC64CartExtension(const char *name);

class C64IOControlView : public TableView
{
private:
	char tapeSlotStr[1024]{};

	void updateTapeText()
	{
		auto name = plugin.tape_get_file_name();
		string_printf(tapeSlotStr, "Tape: %s", name ? FS::basename(name).data() : "");
	}

public:
	void onTapeMediaChange(const char *name)
	{
		updateTapeText();
		tapeSlot.compile(projP);
	}

	void addTapeFilePickerView(Input::Event e)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(false, hasC64TapeExtension, true);
		fPicker.setOnSelectFile(
			[this](FSPicker &picker, const char* name, Input::Event e)
			{
				if(plugin.tape_image_attach(1, name) == 0)
				{
					onTapeMediaChange(name);
				}
				picker.dismiss();
			});
		modalViewController.pushAndShow(fPicker, e);
	}

private:
	TextMenuItem tapeSlot
	{
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active)
				return;
			auto name = plugin.tape_get_file_name();
			if(name && strlen(name))
			{
				auto &multiChoiceView = *new MultiChoiceView{"Tape Drive", window()};
				multiChoiceView.init(insertEjectMenuStr, IG::size(insertEjectMenuStr));
				multiChoiceView.setItem(0,
					[this](TextMenuItem &, View &, Input::Event e)
					{
						addTapeFilePickerView(e);
						postDraw();
						popAndShow();
					});
				multiChoiceView.setItem(1,
					[this](TextMenuItem &, View &, Input::Event e)
					{
						plugin.tape_image_detach(1);
						onTapeMediaChange("");
						popAndShow();
					});
				viewStack.pushAndShow(multiChoiceView, e);
			}
			else
			{
				addTapeFilePickerView(e);
			}
			window().postDraw();
		}
	};

	char romSlotStr[1024]{};

	void updateROMText()
	{
		auto name = plugin.cartridge_get_file_name(plugin.cart_getid_slotmain());
		string_printf(romSlotStr, "ROM: %s", name ? FS::basename(name).data() : "");
	}

public:
	void onROMMediaChange(const char *name)
	{
		updateROMText();
		romSlot.compile(projP);
	}

	static int systemCartType(ViceSystem system)
	{
		switch(system)
		{
			case VICE_SYSTEM_CBM2:
			case VICE_SYSTEM_CBM5X0:
				return CARTRIDGE_CBM2_8KB_1000;
			case VICE_SYSTEM_PLUS4:
				return CARTRIDGE_PLUS4_DETECT;
			default:
				return CARTRIDGE_CRT;
		}
	}

	void addCartFilePickerView(Input::Event e)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(false, hasC64CartExtension, true);
		fPicker.setOnSelectFile(
			[this](FSPicker &picker, const char* name, Input::Event e)
			{
				if(plugin.cartridge_attach_image(systemCartType(currSystem), name) == 0)
				{
					onROMMediaChange(name);
				}
				picker.dismiss();
			});
		modalViewController.pushAndShow(fPicker, e);
	}

private:
	TextMenuItem romSlot
	{
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto cartFilename = plugin.cartridge_get_file_name(plugin.cart_getid_slotmain());
			if(cartFilename && strlen(cartFilename))
			{
				auto &multiChoiceView = *new MultiChoiceView{"Cartridge Slot", window()};
				multiChoiceView.init(insertEjectMenuStr, IG::size(insertEjectMenuStr));
				multiChoiceView.setItem(0,
					[this](TextMenuItem &, View &, Input::Event e)
					{
						addCartFilePickerView(e);
						postDraw();
						popAndShow();
					});
				multiChoiceView.setItem(1,
					[this](TextMenuItem &, View &, Input::Event e)
					{
						plugin.cartridge_detach_image(-1);
						onROMMediaChange("");
						popAndShow();
					});
				viewStack.pushAndShow(multiChoiceView, e);
			}
			else
			{
				addCartFilePickerView(e);
			}
			window().postDraw();
		}
	};

	static constexpr const char *diskSlotPrefix[2]
	{
		"Disk #8:",
		"Disk #9:"
	};
	char diskSlotStr[2][1024]{};

	void updateDiskText(int slot)
	{
		auto name = plugin.file_system_get_disk_name(slot+8);
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot], name ? FS::basename(name).data() : "");
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		updateDiskText(slot);
		diskSlot[slot].compile(projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(false, hasC64DiskExtension, true);
		fPicker.setOnSelectFile(
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				logMsg("inserting disk in unit %d", slot+8);
				if(plugin.file_system_attach_disk(slot+8, name) == 0)
				{
					onDiskMediaChange(name, slot);
				}
				picker.dismiss();
			});
		modalViewController.pushAndShow(fPicker, e);
	}

public:
	void onSelectDisk(Input::Event e, uint8 slot)
	{
		auto name = plugin.file_system_get_disk_name(slot+8);
		if(name && strlen(name))
		{
			auto &multiChoiceView = *new MultiChoiceView{"Disk Drive", window()};
			multiChoiceView.init(insertEjectMenuStr, IG::size(insertEjectMenuStr));
			multiChoiceView.setItem(0,
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addDiskFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.setItem(1,
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					plugin.file_system_detach_disk(slot+8);
					onDiskMediaChange("", slot);
					popAndShow();
				});
			viewStack.pushAndShow(multiChoiceView, e);
		}
		else
		{
			addDiskFilePickerView(e, slot);
		}
		window().postDraw();
	}

private:
	TextMenuItem diskSlot[2]
	{
		{[this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 0); }},
		{[this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 1); }},
	};

	MultiChoiceSelectMenuItem model
	{
		"Model",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			setSysModel(val);
		}
	};

	void modelInit()
	{
		auto modelVal = sysModel();
		auto baseVal = currSystem == VICE_SYSTEM_CBM2 ? 2 : 0;
		if(modelVal < baseVal || modelVal >= plugin.models + baseVal)
		{
			modelVal = baseVal;
		}
		model.init(plugin.modelStr, modelVal, plugin.models, baseVal);
	}

	MenuItem *item[10]{};
public:
	C64IOControlView(Base::Window &win): TableView{"System & IO", win} { }

	void init()
	{
		uint i = 0;
		if(plugin.cartridge_attach_image_)
		{
			updateROMText();
			romSlot.init(romSlotStr); item[i++] = &romSlot;
		}

		iterateTimes(1, slot)
		{
			updateDiskText(slot);
			diskSlot[slot].init(diskSlotStr[slot]); item[i++] = &diskSlot[slot];
		}

		updateTapeText();
		tapeSlot.init(tapeSlotStr); item[i++] = &tapeSlot;

		modelInit(); item[i++] = &model;
		assert(i <= IG::size(item));
		TableView::init(item, i);
	}
};

constexpr const char *C64IOControlView::diskSlotPrefix[2];

class SystemMenuView : public MenuView
{
	BoolMenuItem swapJoystickPorts
	{
		"Swap Joystick Ports",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionSwapJoystickPorts = item.on;
		}
	};

	TextMenuItem c64IOControl
	{
		"System & IO Control",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active)
			{
				FS::current_path(EmuSystem::gamePath());// Stay in active media's directory
				auto &c64IoMenu = *new C64IOControlView{window()};
				c64IoMenu.init();
				viewStack.pushAndShow(c64IoMenu, e);
			}
		}
	};

	TextMenuItem quickSettings
	{
		"Apply Quick Settings",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			static auto reloadGame =
				[]()
				{
					if(EmuSystem::gameIsRunning())
					{
						FS::PathString gamePath;
						string_copy(gamePath, EmuSystem::fullGamePath());
						EmuSystem::loadGame(gamePath.data());
						startGameFromMenu();
					}
				};

			auto &multiChoiceView = *new MultiChoiceView{item.t.str, window()};
			multiChoiceView.init(4, LC2DO);
			multiChoiceView.setItem(0, "1. NTSC & True Drive Emu",
				[this](TextMenuItem &, View &, Input::Event e)
				{
					popAndShow();
					optionDriveTrueEmulation = true;
					setDriveTrueEmulation(true);
					setDefaultNTSCModel();
					reloadGame();
				});
			multiChoiceView.setItem(1, "2. NTSC",
				[this](TextMenuItem &, View &, Input::Event e)
				{
					popAndShow();
					optionDriveTrueEmulation = false;
					setDriveTrueEmulation(false);
					setDefaultNTSCModel();
					reloadGame();
				});
			multiChoiceView.setItem(2, "3. PAL & True Drive Emu",
				[this](TextMenuItem &, View &, Input::Event e)
				{
					popAndShow();
					optionDriveTrueEmulation = true;
					setDriveTrueEmulation(true);
					setDefaultPALModel();
					reloadGame();
				});
			multiChoiceView.setItem(3, "4. PAL",
				[this](TextMenuItem &, View &, Input::Event e)
				{
					popAndShow();
					optionDriveTrueEmulation = false;
					setDriveTrueEmulation(false);
					setDefaultPALModel();
					reloadGame();
				});
			viewStack.pushAndShow(multiChoiceView, e);
		}
	};

	std::array<char, 34> systemStr{};

	TextMenuItem system
	{
		systemStr.data(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			bool systemPresent[VicePlugin::SYSTEMS]{};
			uint systems = 0;
			iterateTimes(IG::size(systemPresent), i)
			{
				bool hasSystem = VicePlugin::hasSystemLib((ViceSystem)i);
				systemPresent[i] = hasSystem;
				if(hasSystem)
					systems++;
			}
			auto &multiChoiceView = *new MultiChoiceView{item.t.str, window()};
			multiChoiceView.init(systems, LC2DO);
			uint idx = 0;
			iterateTimes(IG::size(systemPresent), i)
			{
				if(!systemPresent[i])
				{
					continue;
				}
				multiChoiceView.setItem(idx, VicePlugin::systemName((ViceSystem)i),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						optionViceSystem = i;
						popAndShow();
						auto &ynAlertView = *new YesNoAlertView{window(), "Changing systems needs app restart, exit now?"};
						ynAlertView.onYes() =
							[](Input::Event e)
							{
								Base::exit();
							};
						modalViewController.pushAndShow(ynAlertView, e);
					});
				idx++;
			}
			viewStack.pushAndShow(multiChoiceView, e);
		}
	};

	FS::FileString newDiskName;

	TextMenuItem startWithBlankDisk
	{
		"Start System With Blank Disk",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input Disk Name", getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!strlen(str))
						{
							popup.postError("Name can't be blank");
							return 1;
						}
						string_copy(newDiskName, str);
						workDirStack.push();
						FS::current_path(optionSavePath);
						auto &fPicker = *new EmuFilePicker{window()};
						fPicker.init(true, {});
						fPicker.setOnClose(
							[this](FSPicker &picker, Input::Event e)
							{
								picker.dismiss();
								auto path = FS::makePathStringPrintf("%s/%s.d64", FS::current_path().data(), newDiskName.data());
								workDirStack.pop();
								if(e.isDefaultCancelButton())
								{
									// picker was cancelled
									popup.clear();
									return;
								}
								if(FS::exists(path))
								{
									popup.postError("File already exists");
									return;
								}
								if(plugin.vdrive_internal_create_format_disk_image(path.data(),
									FS::makeFileStringPrintf("%s,dsk", newDiskName.data()).data(),
									DISK_IMAGE_TYPE_D64) == -1)
								{
									popup.postError("Error creating disk image");
									return;
								}
								auto res = ::loadGame(path.data(), false);
								if(res == 1)
								{
									loadGameComplete(false, true);
								}
								else if(res == 0)
								{
									EmuSystem::clearGamePaths();
								}
							});
						view.dismiss();
						modalViewController.pushAndShow(fPicker, Input::defaultEvent());
						popup.post("Set directory to save disk");
					}
					else
					{
						view.dismiss();
					}
					return 0;
				};
			modalViewController.pushAndShow(textInputView, {});
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView{win} {}

	void onShow()
	{
		MenuView::onShow();
		c64IOControl.active = EmuSystem::gameIsRunning();
		swapJoystickPorts.on = optionSwapJoystickPorts;
	}

	void init()
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		startWithBlankDisk.init(); item[items++] = &startWithBlankDisk;
		c64IOControl.init(); item[items++] = &c64IOControl;
		quickSettings.init(); item[items++] = &quickSettings;
		swapJoystickPorts.init(optionSwapJoystickPorts); item[items++] = &swapJoystickPorts;
		string_printf(systemStr, "System: %s", VicePlugin::systemName(currSystem));
		system.init(); item[items++] = &system;
		loadStandardItems(item, items);
		assert(items <= IG::size(item));
		TableView::init(item, items);
	}
};

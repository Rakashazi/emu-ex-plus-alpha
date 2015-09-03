#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>

static const char *installFirmwareFilesMessage =
	#if defined CONFIG_BASE_ANDROID
	"Install the C-BIOS BlueMSX machine files to your storage device?";
	#elif defined CONFIG_ENV_WEBOS
	"Install the C-BIOS BlueMSX machine files to internal storage? If using WebOS 1.4.5, make sure you have a version without the write permission bug.";
	#elif defined CONFIG_BASE_IOS
	"Install the C-BIOS BlueMSX machine files to /User/Media/MSX.emu?";
	#else
	"Install the C-BIOS BlueMSX machine files to Machines directory?";
	#endif
static char installFirmwareFilesStr[512] = "";

template <size_t S>
static void printInstallFirmwareFilesStr(char (&str)[S])
{
	FS::PathString basenameTemp;
	string_printf(str, "Install the C-BIOS BlueMSX machine files to: %s", machineBasePath.data());
}

static void installFirmwareFiles()
{
	CallResult ret;
	FS::create_directory(machineBasePath, ret);
	if(ret != OK && ret != ALREADY_EXISTS)
	{
		popup.printf(4, 1, "Can't create directory:\n%s", machineBasePath.data());
		return;
	}

	const char *dirsToCreate[] =
	{
		"Machines", "Machines/MSX - C-BIOS",
		"Machines/MSX2 - C-BIOS", "Machines/MSX2+ - C-BIOS"
	};

	forEachDInArray(dirsToCreate, e)
	{
		auto pathTemp = FS::makePathStringPrintf("%s/%s", machineBasePath.data(), e);
		CallResult ret;
		FS::create_directory(pathTemp, ret);
		if(ret != OK && ret != ALREADY_EXISTS)
		{
			popup.printf(4, 1, "Can't create directory:\n%s", pathTemp.data());
			return;
		}
	}

	const char *srcPath[] =
	{
		"cbios.txt", "cbios.txt", "cbios.txt",
		"cbios_logo_msx1.rom", "cbios_main_msx1.rom", "config1.ini",
		"cbios_logo_msx2.rom", "cbios_main_msx2.rom", "cbios_sub.rom", "config2.ini",
		"cbios_logo_msx2+.rom", "cbios_main_msx2+.rom", "cbios_sub.rom", "cbios_music.rom", "config3.ini"
	};
	const char *destDir[] =
	{
			"MSX - C-BIOS", "MSX2 - C-BIOS", "MSX2+ - C-BIOS",
			"MSX - C-BIOS", "MSX - C-BIOS", "MSX - C-BIOS",
			"MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS",
			"MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS"
	};

	forEachDInArray(srcPath, e)
	{
		auto src = openAppAssetIO(e);
		if(!src)
		{
			popup.printf(4, 1, "Can't open source file:\n %s", e);
			return;
		}
		auto pathTemp = FS::makePathStringPrintf("%s/Machines/%s/%s",
				machineBasePath.data(), destDir[e_i], strstr(e, "config") ? "config.ini" : e);
		CallResult ret = writeIOToNewFile(src, pathTemp.data());
		if(ret != OK)
		{
			popup.printf(4, 1, "Can't write file:\n%s", e);
			return;
		}
	}

	string_copy(optionMachineNameStr, "MSX2 - C-BIOS");
	popup.post("Installation OK");
}

class SystemOptionView : public OptionView
{
private:

	struct MsxMachineItem : public MultiChoiceSelectMenuItem
	{
		std::vector<FS::FileString> machineName{};
		std::vector<const char *> machineNamePtr{};

		MsxMachineItem() {}

		void init()
		{
			if(machineName.size())
				deinit();
			static const char *title = "Machine Type";
			static const char *noneStr[]{"None"};
			{
				auto machinePath = FS::makePathStringPrintf("%s/Machines", machineBasePath.data());
				for(auto &entry : FS::directory_iterator{machinePath})
				{
					auto configPath = FS::makePathStringPrintf("%s/%s/config.ini", machinePath.data(), entry.name());
					if(!FS::exists(configPath))
					{
						logMsg("%s doesn't exist", configPath.data());
						continue;
					}
					machineName.emplace_back(FS::makeFileString(entry.name()));
					logMsg("added machine %s", entry.name());
				}
			}
			std::sort(machineName.begin(), machineName.end(), FS::fileStringNoCaseLexCompare());
			int currentMachineIdx =
				std::find(machineName.begin(), machineName.end(), FS::makeFileString(optionMachineName)) - machineName.begin();
			if(currentMachineIdx != (int)machineName.size())
			{
				logMsg("current machine is idx %d", currentMachineIdx);
			}
			else
				currentMachineIdx = -1;
			if(!machineName.size())
			{
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}
			for(const auto &name : machineName)
			{
				machineNamePtr.emplace_back(name.data());
			}
			MultiChoiceSelectMenuItem::init(title, machineNamePtr.data(),
				currentMachineIdx, machineNamePtr.size(), 0, true, currentMachineIdx == -1 ? "None" : nullptr);
		}

		void select(View &view, Input::Event e) override
		{
			if(!machineName.size())
			{
				popup.printf(4, 1, "Place machine directory in:\n%s", machineBasePath.data());
				return;
			}
			auto &multiChoiceView = *new MultiChoiceView{"Machine Type", view.window()};
			multiChoiceView.init(*this);
			iterateTimes(machineName.size(), i)
			{
				multiChoiceView.setItem(i,
					[this, i](TextMenuItem &, View &view, Input::Event e)
					{
						setVal(i, view);
						view.popAndShow();
					});
			}
			viewStack.pushAndShow(multiChoiceView, e);
		}

		void deinit()
		{
			logMsg("deinit MsxMachineItem");
			DualTextMenuItem::deinit();
			machineName.clear();
			machineNamePtr.clear();
		}

		void doSet(int val, View &view) override
		{
			assert((uint)val < machineName.size());
			string_copy(optionMachineNameStr, machineName[val].data());
			logMsg("set machine type: %s", (char*)optionMachineName);
		}
	} msxMachine;

	TextMenuItem installCBIOS
	{
		"Install MSX C-BIOS",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			printInstallFirmwareFilesStr(installFirmwareFilesStr);
			auto &ynAlertView = *new YesNoAlertView{window(), installFirmwareFilesStr};
			ynAlertView.onYes() =
				[](Input::Event e)
				{
					installFirmwareFiles();
				};
			modalViewController.pushAndShow(ynAlertView, e);
		}
	};

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionSkipFdcAccess = item.on;
		}
	};

	template <size_t S>
	static void printMachinePathMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "System/BIOS Path: %s", strlen(machineCustomPath.data()) ? FS::basename(machineCustomPath).data() : "Default");
	}

	FirmwarePathSelector machineFileSelector;
	char machineFilePathStr[256]{};
	TextMenuItem machineFilePath
	{
		"",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			machineFileSelector.init("System/BIOS Path", e);
			machineFileSelector.onPathChange =
				[this](const char *newPath)
				{
					printMachinePathMenuEntryStr(machineFilePathStr);
					machineFilePath.compile(projP);
					machineBasePath = makeMachineBasePath(machineCustomPath);
					msxMachine.init();
					msxMachine.compile(projP);
					if(!strlen(newPath))
					{
						popup.printf(4, false, "Using default path:\n%s/MSX.emu", (Config::envIsLinux && !Config::MACHINE_IS_PANDORA) ? Base::assetPath().data() : Base::storagePath().data());
					}
				};
			postDraw();
		}
	};

public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		msxMachine.init(); item[items++] = &msxMachine;
		skipFdcAccess.init(optionSkipFdcAccess); item[items++] = &skipFdcAccess;
		printMachinePathMenuEntryStr(machineFilePathStr);
		machineFilePath.init(machineFilePathStr, true); item[items++] = &machineFilePath;
		if(canInstallCBIOS)
		{
			installCBIOS.init(); item[items++] = &installCBIOS;
		}
	}
};

class MsxMediaFilePicker
{
public:
	enum { ROM, DISK, TAPE };

	static EmuNameFilterFunc fsFilter(uint type)
	{
		EmuNameFilterFunc filter = isMSXROMExtension;
		if(type == DISK)
			filter = isMSXDiskExtension;
		else if(type == TAPE)
			filter = isMSXTapeExtension;
		return filter;
	}
};

static const char *insertEjectDiskMenuStr[] {"Insert File", "Eject"};

class MsxIOControlView : public TableView
{
public:

	static const char *hdSlotPrefix[4];
	char hdSlotStr[4][1024]{};

	void updateHDText(int slot)
	{
		string_printf(hdSlotStr[slot], "%s %s", hdSlotPrefix[slot],
			strlen(hdName[slot]) ? FS::basename(hdName[slot]).data() : "");
	}

	void updateHDStatusFromCartSlot(int cartSlot)
	{
		int hdSlotStart = cartSlot == 0 ? 0 : 2;
		hdSlot[hdSlotStart].active = hdSlot[hdSlotStart+1].active = boardGetHdType(cartSlot) == HD_SUNRISEIDE;
		updateHDText(hdSlotStart);
		updateHDText(hdSlotStart+1);
	}

	void onHDMediaChange(const char *name, int slot)
	{
		strcpy(hdName[slot], name);
		updateHDText(slot);
		hdSlot[slot].compile(projP);
	}

	void addHDFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(false, MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK), 1);
		fPicker.setOnSelectFile(
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				auto id = diskGetHdDriveId(slot / 2, slot % 2);
				logMsg("inserting hard drive id %d", id);
				if(insertDisk(name, id))
				{
					onHDMediaChange(name, slot);
				}
				picker.dismiss();
			});
		modalViewController.pushAndShow(fPicker, e);
	}

	void onSelectHD(TextMenuItem &item, Input::Event e, uint8 slot)
	{
		if(!item.active) return;
		if(strlen(hdName[slot]))
		{
			auto &multiChoiceView = *new MultiChoiceView{"Hard Drive", window()};
			multiChoiceView.init(insertEjectDiskMenuStr, sizeofArray(insertEjectDiskMenuStr));
			multiChoiceView.setItem(0,
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addHDFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.setItem(1,
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
					onHDMediaChange("", slot);
					popAndShow();
				});
			viewStack.pushAndShow(multiChoiceView, e);
		}
		else
		{
			addHDFilePickerView(e, slot);
			window().postDraw();
		}
	}

	TextMenuItem hdSlot[4]
	{
		{[this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 0); }},
		{[this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 1); }},
		{[this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 2); }},
		{[this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];
	char romSlotStr[2][1024]{};

	void updateROMText(int slot)
	{
		string_printf(romSlotStr[slot], "%s %s", romSlotPrefix[slot],
			strlen(cartName[slot]) ? FS::basename(cartName[slot]).data() : "");
	}

	void onROMMediaChange(const char *name, int slot)
	{
		strcpy(cartName[slot], name);
		updateROMText(slot);
		romSlot[slot].compile(projP);
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(false, MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM), 1);
		fPicker.setOnSelectFile(
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				if(insertROM(name, slot))
				{
					onROMMediaChange(name, slot);
				}
				picker.dismiss();
			});
		modalViewController.pushAndShow(fPicker, e);
	}

	void onSelectROM(Input::Event e, uint8 slot)
	{
		auto &multiChoiceView = *new MultiChoiceView{"ROM Cartridge Slot", window()};
		multiChoiceView.init(5);
		multiChoiceView.setItem(0, "Insert File",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				addROMFilePickerView(e, slot);
				postDraw();
				popAndShow();
			});
		multiChoiceView.setItem(1, "Eject",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
				onROMMediaChange("", slot);
				popAndShow();
			});
		multiChoiceView.setItem(2, "Insert SCC",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCC, "", 0);
				onROMMediaChange("SCC", slot);
				popAndShow();
			});
		multiChoiceView.setItem(3, "Insert SCC+",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
				onROMMediaChange("SCC+", slot);
				popAndShow();
			});
		multiChoiceView.setItem(4, "Insert Sunrise IDE",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					popup.postError("Error loading Sunrise IDE device");
				}
				else
					onROMMediaChange("Sunrise IDE", slot);
				popAndShow();
			});
		viewStack.pushAndShow(multiChoiceView, e);
	}

	TextMenuItem romSlot[2]
	{
		{[this](TextMenuItem &, View &, Input::Event e) { onSelectROM(e, 0); }},
		{[this](TextMenuItem &, View &, Input::Event e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];
	char diskSlotStr[2][1024]{};

	void updateDiskText(int slot)
	{
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot],
			strlen(diskName[slot]) ? FS::basename(diskName[slot]).data() : "");
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		strcpy(diskName[slot], name);
		updateDiskText(slot);
		diskSlot[slot].compile(projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(false, MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK), 1);
		fPicker.setOnSelectFile(
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(name, slot))
				{
					onDiskMediaChange(name, slot);
				}
				picker.dismiss();
			});
		modalViewController.pushAndShow(fPicker, e);
	}

	void onSelectDisk(Input::Event e, uint8 slot)
	{
		if(strlen(diskName[slot]))
		{
			auto &multiChoiceView = *new MultiChoiceView{"Disk Drive", window()};
			multiChoiceView.init(insertEjectDiskMenuStr, sizeofArray(insertEjectDiskMenuStr));
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
					diskChange(slot, 0, 0);
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

	TextMenuItem diskSlot[2]
	{
		{[this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 0); }},
		{[this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 1); }}
	};

	MenuItem *item[9]{};
public:
	MsxIOControlView(Base::Window &win): TableView{"IO Control", win} {}

	void init()
	{
		uint i = 0;
		iterateTimes(2, slot)
		{
			updateROMText(slot);
			romSlot[slot].init(romSlotStr[slot], (int)slot < boardInfo.cartridgeCount); item[i++] = &romSlot[slot];
		}
		iterateTimes(2, slot)
		{
			updateDiskText(slot);
			diskSlot[slot].init(diskSlotStr[slot], (int)slot < boardInfo.diskdriveCount); item[i++] = &diskSlot[slot];
		}
		iterateTimes(4, slot)
		{
			updateHDText(slot);
			hdSlot[slot].init(hdSlotStr[slot], boardGetHdType(slot/2) == HD_SUNRISEIDE); item[i++] = &hdSlot[slot];
		}
		assert(i <= sizeofArray(item));
		TableView::init(item, i);
	}
};

const char *MsxIOControlView::romSlotPrefix[2] {"ROM1:", "ROM2:"};
const char *MsxIOControlView::diskSlotPrefix[2] {"Disk1:", "Disk2:"};
const char *MsxIOControlView::hdSlotPrefix[4] {"IDE1-M:", "IDE1-S:", "IDE2-M:", "IDE2-S:"};

class SystemMenuView : public MenuView
{
private:
	TextMenuItem msxIOControl
	{
		"ROM/Disk Control",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active)
			{
				FS::current_path(EmuSystem::gamePath());// Stay in active media's directory
				auto &msxIoMenu = *new MsxIOControlView{window()};
				msxIoMenu.init();
				viewStack.pushAndShow(msxIoMenu, e);
			}
			else if(EmuSystem::gameIsRunning() && activeBoardType != BOARD_MSX)
			{
				popup.post("Only used in MSX mode", 2);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView(win) {}

	void gameStopped()
	{
		msxIOControl.active = 0;
		reset.active = 0;
		loadState.active = 0;
		saveState.active = 0;
	}

	void onShow()
	{
		MenuView::onShow();
		msxIOControl.active = EmuSystem::gameIsRunning() && activeBoardType == BOARD_MSX;
	}

	void init()
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		msxIOControl.init(); item[items++] = &msxIOControl;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items);
	}
};

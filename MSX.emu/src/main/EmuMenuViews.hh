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
	FsSys::PathString basenameTemp;
	string_printf(str, "Install the C-BIOS BlueMSX machine files to: %s", machineBasePath.data());
}

static void installFirmwareFiles()
{
	CallResult ret = FsSys::mkdir(machineBasePath.data());
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
		auto pathTemp = makeFSPathStringPrintf("%s/%s", machineBasePath.data(), e);
		CallResult ret = FsSys::mkdir(pathTemp.data());
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
		auto pathTemp = makeFSPathStringPrintf("%s/Machines/%s/%s",
				machineBasePath.data(), destDir[e_i], strstr(e, "config") ? "config.ini" : e);
		CallResult ret = writeIOToNewFile(src, pathTemp.data());
		if(ret != OK)
		{
			popup.printf(4, 1, "Can't write file:\n%s", e);
			return;
		}
	}

	string_copy(optionMachineNameStr, "MSX2 - C-BIOS", sizeof(optionMachineNameStr));
	popup.post("Installation OK");
}

class SystemOptionView : public OptionView
{
private:

	struct MsxMachineItem : public MultiChoiceSelectMenuItem
	{
		constexpr MsxMachineItem() {}

		static int dirFsFilter(const char *name, int type)
		{
			return type == Fs::TYPE_DIR;
		}

		uint machines = 0;
		char *machineName[256]{};

		void init()
		{
			if(machines)
				deinit();
			FsSys f;
			static const char *title = "Machine Type";
			static const char *noneStr[] = { "None" };
			auto machinePath = makeFSPathStringPrintf("%s/Machines", machineBasePath.data());
			if(f.openDir(machinePath.data(), 0, dirFsFilter) != OK)
			{
				logMsg("couldn't open %s", machinePath.data());
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}

			int currentMachineIdx = -1;
			machines = 0;
			iterateTimes(std::min(f.numEntries(), 256U), i)
			{
				auto configPath = makeFSPathStringPrintf("%s/%s/config.ini", machinePath.data(), f.entryFilename(i));
				if(!FsSys::fileExists(configPath.data()))
				{
					logMsg("%s doesn't exist", configPath.data());
					continue;
				}
				machineName[machines] = string_dup(f.entryFilename(i));
				logMsg("added machine %s", f.entryFilename(i));
				if(string_equal(machineName[machines], optionMachineName))
				{
					logMsg("current machine is idx %d", i);
					currentMachineIdx = machines;
				}
				machines++;
			}

			if(!machines)
			{
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}

			MultiChoiceSelectMenuItem::init(title, (const char **)machineName,
				currentMachineIdx, machines, 0, true, currentMachineIdx == -1 ? "None" : nullptr);
		}

		void select(View &view, const Input::Event &e) override
		{
			if(!machines)
			{
				popup.printf(4, 1, "Place machine directory in:\n%s", machineBasePath.data());
				return;
			}
			auto &multiChoiceView = *new MultiChoiceView{"Machine Type", view.window()};
			multiChoiceView.init(*this, !e.isPointer());
			iterateTimes(machines, i)
			{
				multiChoiceView.setItem(i,
					[this, i](TextMenuItem &, View &view, const Input::Event &e)
					{
						setVal(i, view);
						view.popAndShow();
					});
			}
			viewStack.pushAndShow(multiChoiceView);
		}

		void deinit()
		{
			logMsg("deinit MsxMachineItem");
			DualTextMenuItem::deinit();
			iterateTimes(machines, i)
			{
				mem_free(machineName[i]);
			}
			machines = 0;
		}

		void doSet(int val, View &view) override
		{
			assert((uint)val < machines);
			string_copy(optionMachineName, machineName[val], sizeof(optionMachineName));
			logMsg("set machine type: %s", (char*)optionMachineName);
		}
	} msxMachine;

	TextMenuItem installCBIOS
	{
		"Install MSX C-BIOS",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			printInstallFirmwareFilesStr(installFirmwareFilesStr);
			auto &ynAlertView = *new YesNoAlertView{window()};
			ynAlertView.init(installFirmwareFilesStr, !e.isPointer());
			ynAlertView.onYes() =
				[](const Input::Event &e)
				{
					installFirmwareFiles();
				};
			modalViewController.pushAndShow(ynAlertView);
		}
	};

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionSkipFdcAccess = item.on;
		}
	};

	template <size_t S>
	static void printMachinePathMenuEntryStr(char (&str)[S])
	{
		FsSys::PathString basenameTemp;
		string_printf(str, "System/BIOS Path: %s", strlen(machineCustomPath.data()) ? string_basename(machineCustomPath, basenameTemp) : "Default");
	}

	FirmwarePathSelector machineFileSelector;
	char machineFilePathStr[256]{};
	TextMenuItem machineFilePath
	{
		"",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			machineFileSelector.init("System/BIOS Path", !e.isPointer());
			machineFileSelector.onPathChange =
				[this](const char *newPath)
				{
					printMachinePathMenuEntryStr(machineFilePathStr);
					machineFilePath.compile(projP);
					setMachineBasePath(machineBasePath, machineCustomPath);
					msxMachine.init();
					msxMachine.compile(projP);
					if(!strlen(newPath))
					{
						popup.printf(4, false, "Using default path:\n%s/MSX.emu", (Config::envIsLinux && !Config::MACHINE_IS_PANDORA) ? Base::assetPath() : Base::storagePath());
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

	static FsDirFilterFunc fsFilter(uint type)
	{
		FsDirFilterFunc filter = isMSXROMExtension;
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
		FsSys::PathString basenameTemp;
		string_printf(hdSlotStr[slot], "%s %s", hdSlotPrefix[slot],
			strlen(hdName[slot]) ? string_basename(hdName[slot], basenameTemp) : "");
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

	void addHDFilePickerView(const Input::Event &e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(!e.isPointer(), false, MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK), 1);
		fPicker.onSelectFile() =
			[this, slot](FSPicker &picker, const char* name, const Input::Event &e)
			{
				auto id = diskGetHdDriveId(slot / 2, slot % 2);
				logMsg("inserting hard drive id %d", id);
				if(insertDisk(name, id))
				{
					onHDMediaChange(name, slot);
				}
				picker.dismiss();
			};
		modalViewController.pushAndShow(fPicker);
	}

	void onSelectHD(TextMenuItem &item, const Input::Event &e, uint8 slot)
	{
		if(!item.active) return;
		if(strlen(hdName[slot]))
		{
			auto &multiChoiceView = *new MultiChoiceView{"Hard Drive", window()};
			multiChoiceView.init(insertEjectDiskMenuStr, sizeofArray(insertEjectDiskMenuStr), !e.isPointer());
			multiChoiceView.setItem(0,
				[this, slot](TextMenuItem &, View &, const Input::Event &e)
				{
					addHDFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.setItem(1,
				[this, slot](TextMenuItem &, View &, const Input::Event &e)
				{
					diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
					onHDMediaChange("", slot);
					popAndShow();
				});
			viewStack.pushAndShow(multiChoiceView);
		}
		else
		{
			addHDFilePickerView(e, slot);
			window().postDraw();
		}
	}

	TextMenuItem hdSlot[4]
	{
		{[this](TextMenuItem &item, View &, const Input::Event &e) { onSelectHD(item, e, 0); }},
		{[this](TextMenuItem &item, View &, const Input::Event &e) { onSelectHD(item, e, 1); }},
		{[this](TextMenuItem &item, View &, const Input::Event &e) { onSelectHD(item, e, 2); }},
		{[this](TextMenuItem &item, View &, const Input::Event &e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];
	char romSlotStr[2][1024]{};

	void updateROMText(int slot)
	{
		FsSys::PathString basenameTemp;
		string_printf(romSlotStr[slot], "%s %s", romSlotPrefix[slot],
			strlen(cartName[slot]) ? string_basename(cartName[slot], basenameTemp) : "");
	}

	void onROMMediaChange(const char *name, int slot)
	{
		strcpy(cartName[slot], name);
		updateROMText(slot);
		romSlot[slot].compile(projP);
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(const Input::Event &e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(!e.isPointer(), false, MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM), 1);
		fPicker.onSelectFile() =
			[this, slot](FSPicker &picker, const char* name, const Input::Event &e)
			{
				if(insertROM(name, slot))
				{
					onROMMediaChange(name, slot);
				}
				picker.dismiss();
			};
		modalViewController.pushAndShow(fPicker);
	}

	void onSelectROM(const Input::Event &e, uint8 slot)
	{
		auto &multiChoiceView = *new MultiChoiceView{"ROM Cartridge Slot", window()};
		multiChoiceView.init(5, !e.isPointer());
		multiChoiceView.setItem(0, "Insert File",
			[this, slot](TextMenuItem &, View &, const Input::Event &e)
			{
				addROMFilePickerView(e, slot);
				postDraw();
				popAndShow();
			});
		multiChoiceView.setItem(1, "Eject",
			[this, slot](TextMenuItem &, View &, const Input::Event &e)
			{
				boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
				onROMMediaChange("", slot);
				popAndShow();
			});
		multiChoiceView.setItem(2, "Insert SCC",
			[this, slot](TextMenuItem &, View &, const Input::Event &e)
			{
				boardChangeCartridge(slot, ROM_SCC, "", 0);
				onROMMediaChange("SCC", slot);
				popAndShow();
			});
		multiChoiceView.setItem(3, "Insert SCC+",
			[this, slot](TextMenuItem &, View &, const Input::Event &e)
			{
				boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
				onROMMediaChange("SCC+", slot);
				popAndShow();
			});
		multiChoiceView.setItem(4, "Insert Sunrise IDE",
			[this, slot](TextMenuItem &, View &, const Input::Event &e)
			{
				if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					popup.postError("Error loading Sunrise IDE device");
				}
				else
					onROMMediaChange("Sunrise IDE", slot);
				popAndShow();
			});
		viewStack.pushAndShow(multiChoiceView);
	}

	TextMenuItem romSlot[2]
	{
		{[this](TextMenuItem &, View &, const Input::Event &e) { onSelectROM(e, 0); }},
		{[this](TextMenuItem &, View &, const Input::Event &e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];
	char diskSlotStr[2][1024]{};

	void updateDiskText(int slot)
	{
		FsSys::PathString basenameTemp;
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot],
				strlen(diskName[slot]) ? string_basename(diskName[slot], basenameTemp) : "");
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		strcpy(diskName[slot], name);
		updateDiskText(slot);
		diskSlot[slot].compile(projP);
	}

	void addDiskFilePickerView(const Input::Event &e, uint8 slot)
	{
		auto &fPicker = *new EmuFilePicker{window()};
		fPicker.init(!e.isPointer(), false, MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK), 1);
		fPicker.onSelectFile() =
			[this, slot](FSPicker &picker, const char* name, const Input::Event &e)
			{
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(name, slot))
				{
					onDiskMediaChange(name, slot);
				}
				picker.dismiss();
			};
		modalViewController.pushAndShow(fPicker);
	}

	void onSelectDisk(const Input::Event &e, uint8 slot)
	{
		if(strlen(diskName[slot]))
		{
			auto &multiChoiceView = *new MultiChoiceView{"Disk Drive", window()};
			multiChoiceView.init(insertEjectDiskMenuStr, sizeofArray(insertEjectDiskMenuStr), !e.isPointer());
			multiChoiceView.setItem(0,
				[this, slot](TextMenuItem &, View &, const Input::Event &e)
				{
					addDiskFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.setItem(1,
				[this, slot](TextMenuItem &, View &, const Input::Event &e)
				{
					diskChange(slot, 0, 0);
					onDiskMediaChange("", slot);
					popAndShow();
				});
			viewStack.pushAndShow(multiChoiceView);
		}
		else
		{
			addDiskFilePickerView(e, slot);
		}
		window().postDraw();
	}

	TextMenuItem diskSlot[2]
	{
		{[this](TextMenuItem &, View &, const Input::Event &e) { onSelectDisk(e, 0); }},
		{[this](TextMenuItem &, View &, const Input::Event &e) { onSelectDisk(e, 1); }}
	};

	MenuItem *item[9]{};
public:
	MsxIOControlView(Base::Window &win): TableView{"IO Control", win} {}

	void init(bool highlightFirst)
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
		TableView::init(item, i, highlightFirst);
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
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(item.active)
			{
				FsSys::chdir(EmuSystem::gamePath());// Stay in active media's directory
				auto &msxIoMenu = *new MsxIOControlView{window()};
				msxIoMenu.init(!e.isPointer());
				viewStack.pushAndShow(msxIoMenu);
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

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		msxIOControl.init(); item[items++] = &msxIOControl;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};

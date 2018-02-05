#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/FilePicker.hh>
#include "internal.hh"

extern "C"
{
	#include <blueMSX/IoDevice/Disk.h>
}

template <size_t S>
static void printInstallFirmwareFilesStr(char (&str)[S])
{
	FS::PathString basenameTemp;
	string_printf(str, "Install the C-BIOS BlueMSX machine files to: %s", machineBasePath.data());
}

void installFirmwareFiles()
{
	std::error_code ec{};
	FS::create_directory(machineBasePath, ec);
	if(ec && ec.value() != (int)std::errc::file_exists)
	{
		EmuApp::printfMessage(4, 1, "Can't create directory:\n%s", machineBasePath.data());
		return;
	}

	const char *dirsToCreate[] =
	{
		"Machines", "Machines/MSX - C-BIOS",
		"Machines/MSX2 - C-BIOS", "Machines/MSX2+ - C-BIOS"
	};

	for(auto e : dirsToCreate)
	{
		auto pathTemp = FS::makePathStringPrintf("%s/%s", machineBasePath.data(), e);
		std::error_code ec{};
		FS::create_directory(pathTemp, ec);
		if(ec && ec.value() != (int)std::errc::file_exists)
		{
			EmuApp::printfMessage(4, 1, "Can't create directory:\n%s", pathTemp.data());
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

	for(auto &e : srcPath)
	{
		auto src = EmuApp::openAppAssetIO(e, IO::AccessHint::ALL);
		if(!src)
		{
			EmuApp::printfMessage(4, 1, "Can't open source file:\n %s", e);
			return;
		}
		auto e_i = &e - srcPath;
		auto pathTemp = FS::makePathStringPrintf("%s/Machines/%s/%s",
				machineBasePath.data(), destDir[e_i], strstr(e, "config") ? "config.ini" : e);
		auto ec = writeIOToNewFile(src, pathTemp.data());
		if(ec)
		{
			EmuApp::printfMessage(4, 1, "Can't write file:\n%s", e);
			return;
		}
	}

	string_copy(optionMachineNameStr, "MSX2 - C-BIOS");
	EmuApp::postMessage("Installation OK");
}

class CustomSystemOptionView : public SystemOptionView
{
private:

	std::vector<FS::FileString> msxMachineName{};
	std::vector<TextMenuItem> msxMachineItem{};

	MultiChoiceMenuItem msxMachine
	{
		"Machine Type",
		[](int idx) -> const char*
		{
			if(idx == -1)
				return "None";
			else
				return nullptr;
		},
		0,
		msxMachineItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			if(!msxMachineItem.size())
			{
				EmuApp::printfMessage(4, 1, "Place machine directory in:\n%s", machineBasePath.data());
				return;
			}
			item.defaultOnSelect(view, e);
		}
	};

	void reloadMachineItem()
	{
		msxMachineName.clear();
		msxMachineItem.clear();
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
				msxMachineName.emplace_back(FS::makeFileString(entry.name()));
				logMsg("added machine %s", entry.name());
			}
		}
		std::sort(msxMachineName.begin(), msxMachineName.end(), FS::fileStringNoCaseLexCompare());
		for(const auto &name : msxMachineName)
		{
			msxMachineItem.emplace_back(name.data(),
			[](TextMenuItem &item, View &, Input::Event)
			{
				string_copy(optionMachineNameStr, item.t.str);
				logMsg("set machine type: %s", (char*)optionMachineName);
			});
		}
		int currentMachineIdx =
			std::find(msxMachineName.begin(), msxMachineName.end(), FS::makeFileString(optionMachineName)) - msxMachineName.begin();
		if(currentMachineIdx != (int)msxMachineName.size())
		{
			logMsg("current machine is idx %d", currentMachineIdx);
			msxMachine.setSelected(currentMachineIdx);
		}
		else
		{
			msxMachine.setSelected(-1);
		}
	}

	char installFirmwareFilesStr[512]{};

	TextMenuItem installCBIOS
	{
		"Install MSX C-BIOS",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			printInstallFirmwareFilesStr(installFirmwareFilesStr);
			auto &ynAlertView = *new YesNoAlertView{attachParams(), installFirmwareFilesStr};
			ynAlertView.setOnYes(
				[](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					installFirmwareFiles();
				});
			EmuApp::pushAndShowModalView(ynAlertView, e);
		}
	};

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO",
		(bool)optionSkipFdcAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSkipFdcAccess = item.flipBoolValue(*this);
		}
	};

	template <size_t S>
	static void printMachinePathMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "System/BIOS Path: %s", strlen(machineCustomPath.data()) ? FS::basename(machineCustomPath).data() : "Default");
	}

	char machineFilePathStr[256]{};

	TextMenuItem machineFilePath
	{
		machineFilePathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pushAndShowFirmwarePathMenu("System/BIOS Path", e);
			postDraw();
		}
	};

	void onFirmwarePathChange(const char *path, Input::Event e) final
	{
		printMachinePathMenuEntryStr(machineFilePathStr);
		machineFilePath.compile(renderer(), projP);
		machineBasePath = makeMachineBasePath(machineCustomPath);
		reloadMachineItem();
		msxMachine.compile(renderer(), projP);
		if(!strlen(path))
		{
			EmuApp::printfMessage(4, false, "Using default path:\n%s/MSX.emu", (Config::envIsLinux && !Config::MACHINE_IS_PANDORA) ? EmuApp::assetPath().data() : Base::sharedStoragePath().data());
		}
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		reloadMachineItem();
		item.emplace_back(&skipFdcAccess);
		item.emplace_back(&msxMachine);
		printMachinePathMenuEntryStr(machineFilePathStr);
		item.emplace_back(&machineFilePath);
		if(canInstallCBIOS)
		{
			item.emplace_back(&installCBIOS);
		}
	}
};

class MsxMediaFilePicker
{
public:
	enum { ROM, DISK, TAPE };

	static EmuSystem::NameFilterFunc fsFilter(uint type)
	{
		EmuSystem::NameFilterFunc filter = hasMSXROMExtension;
		if(type == DISK)
			filter = hasMSXDiskExtension;
		else if(type == TAPE)
			filter = hasMSXTapeExtension;
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
		string_printf(hdSlotStr[slot], "%s %s", hdSlotPrefix[slot], hdName[slot].data());
	}

	void updateHDStatusFromCartSlot(int cartSlot)
	{
		int hdSlotStart = cartSlot == 0 ? 0 : 2;
		bool isActive = boardGetHdType(cartSlot) == HD_SUNRISEIDE;
		hdSlot[hdSlotStart].setActive(isActive);
		hdSlot[hdSlotStart+1].setActive(isActive);
		updateHDText(hdSlotStart);
		updateHDText(hdSlotStart+1);
	}

	void onHDMediaChange(const char *name, int slot)
	{
		string_copy(hdName[slot], name);
		updateHDText(slot);
		hdSlot[slot].compile(renderer(), projP);
	}

	void addHDFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
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
		EmuApp::pushAndShowModalView(fPicker, e);
	}

	void onSelectHD(TextMenuItem &item, Input::Event e, uint8 slot)
	{
		if(!item.active())
			return;
		if(strlen(hdName[slot].data()))
		{
			auto &multiChoiceView = *new TextTableView{"Hard Drive", attachParams(), IG::size(insertEjectDiskMenuStr)};
			multiChoiceView.appendItem(insertEjectDiskMenuStr[0],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addHDFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.appendItem(insertEjectDiskMenuStr[1],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
					onHDMediaChange("", slot);
					popAndShow();
				});
			pushAndShow(multiChoiceView, e);
		}
		else
		{
			addHDFilePickerView(e, slot);
			window().postDraw();
		}
	}

	TextMenuItem hdSlot[4]
	{
		{hdSlotStr[0], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 0); }},
		{hdSlotStr[1], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 1); }},
		{hdSlotStr[2], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 2); }},
		{hdSlotStr[3], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];
	char romSlotStr[2][1024]{};

	void updateROMText(int slot)
	{
		string_printf(romSlotStr[slot], "%s %s", romSlotPrefix[slot], cartName[slot].data());
	}

	void onROMMediaChange(const char *name, int slot)
	{
		string_copy(cartName[slot], name);
		updateROMText(slot);
		romSlot[slot].compile(renderer(), projP);
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM),
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				if(insertROM(name, slot))
				{
					onROMMediaChange(name, slot);
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(fPicker, e);
	}

	void onSelectROM(Input::Event e, uint8 slot)
	{
		auto &multiChoiceView = *new TextTableView{"ROM Cartridge Slot", attachParams(), 5};
		multiChoiceView.appendItem("Insert File",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				addROMFilePickerView(e, slot);
				postDraw();
				popAndShow();
			});
		multiChoiceView.appendItem("Eject",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
				onROMMediaChange("", slot);
				popAndShow();
			});
		multiChoiceView.appendItem("Insert SCC",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCC, "", 0);
				onROMMediaChange("SCC", slot);
				popAndShow();
			});
		multiChoiceView.appendItem("Insert SCC+",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
				onROMMediaChange("SCC+", slot);
				popAndShow();
			});
		multiChoiceView.appendItem("Insert Sunrise IDE",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					EmuApp::postMessage(true, "Error loading Sunrise IDE device");
				}
				else
					onROMMediaChange("Sunrise IDE", slot);
				popAndShow();
			});
		pushAndShow(multiChoiceView, e);
	}

	TextMenuItem romSlot[2]
	{
		{romSlotStr[0], [this](TextMenuItem &, View &, Input::Event e) { onSelectROM(e, 0); }},
		{romSlotStr[1], [this](TextMenuItem &, View &, Input::Event e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];
	char diskSlotStr[2][1024]{};

	void updateDiskText(int slot)
	{
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot], diskName[slot].data());
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		string_copy(diskName[slot], name);
		updateDiskText(slot);
		diskSlot[slot].compile(renderer(), projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(name, slot))
				{
					onDiskMediaChange(name, slot);
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(fPicker, e);
	}

	void onSelectDisk(Input::Event e, uint8 slot)
	{
		if(strlen(diskName[slot].data()))
		{
			auto &multiChoiceView = *new TextTableView{"Disk Drive", attachParams(), IG::size(insertEjectDiskMenuStr)};
			multiChoiceView.appendItem(insertEjectDiskMenuStr[0],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addDiskFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.appendItem(insertEjectDiskMenuStr[1],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					diskChange(slot, 0, 0);
					onDiskMediaChange("", slot);
					popAndShow();
				});
			pushAndShow(multiChoiceView, e);
		}
		else
		{
			addDiskFilePickerView(e, slot);
		}
		window().postDraw();
	}

	TextMenuItem diskSlot[2]
	{
		{diskSlotStr[0], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 0); }},
		{diskSlotStr[1], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 1); }}
	};

	StaticArrayList<MenuItem*, 9> item{};

public:
	MsxIOControlView(ViewAttachParams attach):
		TableView
		{
			"IO Control",
			attach,
			[this](const TableView &)
			{
				return item.size();
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				return *item[idx];
			}
		}
	{
		iterateTimes(2, slot)
		{
			updateROMText(slot);
			romSlot[slot].setActive((int)slot < boardInfo.cartridgeCount);
			item.emplace_back(&romSlot[slot]);
		}
		iterateTimes(2, slot)
		{
			updateDiskText(slot);
			diskSlot[slot].setActive((int)slot < boardInfo.diskdriveCount);
			item.emplace_back(&diskSlot[slot]);
		}
		iterateTimes(4, slot)
		{
			updateHDText(slot);
			hdSlot[slot].setActive(boardGetHdType(slot/2) == HD_SUNRISEIDE);
			item.emplace_back(&hdSlot[slot]);
		}
	}
};

const char *MsxIOControlView::romSlotPrefix[2] {"ROM1:", "ROM2:"};
const char *MsxIOControlView::diskSlotPrefix[2] {"Disk1:", "Disk2:"};
const char *MsxIOControlView::hdSlotPrefix[4] {"IDE1-M:", "IDE1-S:", "IDE2-M:", "IDE2-S:"};

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
	TextMenuItem msxIOControl
	{
		"ROM/Disk Control",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active())
			{
				auto &msxIoMenu = *new MsxIOControlView{attachParams()};
				pushAndShow(msxIoMenu, e);
			}
			else if(EmuSystem::gameIsRunning() && activeBoardType != BOARD_MSX)
			{
				EmuApp::postMessage(2, false, "Only used in MSX mode");
			}
		}
	};

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&msxIOControl);
		loadStandardItems();
	}

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		reloadItems();
	}

	void onShow()
	{
		EmuSystemActionsView::onShow();
		msxIOControl.setActive(EmuSystem::gameIsRunning() && activeBoardType == BOARD_MSX);
	}
};

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return new CustomSystemActionsView(attach);
		case ViewID::SYSTEM_OPTIONS: return new CustomSystemOptionView(attach);
		default: return nullptr;
	}
}

/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/FilePicker.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>
#include "internal.hh"

extern "C"
{
	#include <blueMSX/IoDevice/Disk.h>
}

static std::vector<FS::FileString> machinesNames(const char *basePath)
{
	std::vector<FS::FileString> machineName{};
	auto machinePath = FS::makePathStringPrintf("%s/Machines", basePath);
	for(auto &entry : FS::directory_iterator{machinePath})
	{
		auto configPath = FS::makePathStringPrintf("%s/%s/config.ini", machinePath.data(), entry.name());
		if(!FS::exists(configPath))
		{
			//logMsg("%s doesn't exist", configPath.data());
			continue;
		}
		machineName.emplace_back(FS::makeFileString(entry.name()));
		logMsg("found machine:%s", entry.name());
	}
	std::sort(machineName.begin(), machineName.end(),
		[](FS::FileString n1, FS::FileString n2)
		{
			return FS::fileStringNoCaseLexCompare(n1, n2);
		});
	return machineName;
}

static int machineIndex(std::vector<FS::FileString> &name, FS::FileString searchName)
{
	int currentMachineIdx =
		std::find(name.begin(), name.end(), searchName) - name.begin();
	if(currentMachineIdx != (int)name.size())
	{
		//logMsg("current machine is idx %d", currentMachineIdx);
		return(currentMachineIdx);
	}
	else
	{
		return -1;
	}
}

void installFirmwareFiles(Base::ApplicationContext ctx)
{
	auto &app = EmuApp::get(ctx);
	std::error_code ec{};
	FS::create_directory(machineBasePath, ec);
	if(ec && ec.value() != (int)std::errc::file_exists)
	{
		app.printfMessage(4, 1, "Can't create directory:\n%s", machineBasePath.data());
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
			app.printfMessage(4, 1, "Can't create directory:\n%s", pathTemp.data());
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
		auto src = EmuApp::openAppAssetIO(ctx, e, IO::AccessHint::ALL);
		if(!src)
		{
			app.printfMessage(4, 1, "Can't open source file:\n %s", e);
			return;
		}
		auto e_i = &e - srcPath;
		auto pathTemp = FS::makePathStringPrintf("%s/Machines/%s/%s",
				machineBasePath.data(), destDir[e_i], strstr(e, "config") ? "config.ini" : e);
		if(FileUtils::writeToPath(pathTemp.data(), src) == -1)
		{
			app.printfMessage(4, 1, "Can't write file:\n%s", e);
			return;
		}
	}

	setDefaultMachineName("MSX2 - C-BIOS");
	app.postMessage("Installation OK");
}

class CustomSystemOptionView : public SystemOptionView
{
private:
	std::vector<FS::FileString> msxMachineName{};
	std::vector<TextMenuItem> msxMachineItem{};

	MultiChoiceMenuItem msxMachine
	{
		"Default Machine Type", &defaultFace(),
		[](int idx, Gfx::Text &t)
		{
			if(idx == -1)
			{
				t.setString("None");
				return true;
			}
			return false;
		},
		0,
		msxMachineItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			if(!msxMachineItem.size())
			{
				app().printfMessage(4, 1, "Place machine directory in:\n%s", machineBasePath.data());
				return;
			}
			item.defaultOnSelect(view, e);
		}
	};

	void reloadMachineItem()
	{
		msxMachineItem.clear();
		msxMachineName = machinesNames(machineBasePath.data());
		for(const auto &name : msxMachineName)
		{
			msxMachineItem.emplace_back(name.data(), &defaultFace(),
			[name = name.data()](Input::Event)
			{
				setDefaultMachineName(name);
				logMsg("set machine type: %s", name);
			});
		}
		msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(optionDefaultMachineName)));
	}

	TextMenuItem installCBIOS
	{
		"Install MSX C-BIOS", &defaultFace(),
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>(
				string_makePrintf<512>("Install the C-BIOS BlueMSX machine files to: %s", machineBasePath.data()).data());
			ynAlertView->setOnYes(
				[this]()
				{
					installFirmwareFiles(appContext());
				});
			app().pushAndShowModalView(std::move(ynAlertView), e);
		}
	};

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO", &defaultFace(),
		(bool)optionSkipFdcAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSkipFdcAccess = item.flipBoolValue(*this);
		}
	};

	static std::array<char, 256> makeMachinePathMenuEntryStr()
	{
		return string_makePrintf<256>("System/BIOS Path: %s", strlen(machineCustomPath.data()) ? FS::basename(machineCustomPath).data() : "Default");
	}

	TextMenuItem machineFilePath
	{
		nullptr, &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShowFirmwarePathMenu("System/BIOS Path", e);
			postDraw();
		}
	};

	void onFirmwarePathChange(const char *path, Input::Event e) final
	{
		machineFilePath.compile(makeMachinePathMenuEntryStr().data(), renderer(), projP);
		machineBasePath = makeMachineBasePath(appContext(), machineCustomPath);
		reloadMachineItem();
		msxMachine.compile(renderer(), projP);
		if(!strlen(path))
		{
			app().printfMessage(4, false, "Using default path:\n%s/MSX.emu", (Config::envIsLinux && !Config::MACHINE_IS_PANDORA) ? EmuApp::assetPath(appContext()).data() : appContext().sharedStoragePath().data());
		}
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		reloadMachineItem();
		item.emplace_back(&skipFdcAccess);
		item.emplace_back(&msxMachine);
		machineFilePath.setName(makeMachinePathMenuEntryStr().data());
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

	static EmuSystem::NameFilterFunc fsFilter(unsigned type)
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

class MsxIOControlView : public TableView, public EmuAppHelper<MsxIOControlView>
{
public:
	static const char *hdSlotPrefix[4];

	void updateHDText(int slot)
	{
		hdSlot[slot].setName(string_makePrintf<1024>("%s %s", hdSlotPrefix[slot], hdName[slot].data()).data());
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

	void addHDFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		auto fPicker = EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot, dismissPreviousView](FSPicker &picker, const char* name, Input::Event e)
			{
				auto id = diskGetHdDriveId(slot / 2, slot % 2);
				logMsg("inserting hard drive id %d", id);
				if(insertDisk(app(), name, id))
				{
					onHDMediaChange(name, slot);
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			});
		app().pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectHD(TextMenuItem &item, Input::Event e, uint8_t slot)
	{
		if(!item.active())
			return;
		if(strlen(hdName[slot].data()))
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Hard Drive", std::size(insertEjectDiskMenuStr));
			multiChoiceView->appendItem(insertEjectDiskMenuStr[0],
				[this, slot](View &view, Input::Event e)
				{
					addHDFilePickerView(e, slot, true);
				});
			multiChoiceView->appendItem(insertEjectDiskMenuStr[1],
				[this, slot](View &view, Input::Event e)
				{
					diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
					onHDMediaChange("", slot);
					view.dismiss();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
		else
		{
			addHDFilePickerView(e, slot, false);
		}
	}

	TextMenuItem hdSlot[4]
	{
		{nullptr, &defaultFace(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 0); }},
		{nullptr, &defaultFace(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 1); }},
		{nullptr, &defaultFace(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 2); }},
		{nullptr, &defaultFace(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];

	void updateROMText(int slot)
	{
		romSlot[slot].setName(string_makePrintf<1024>("%s %s", romSlotPrefix[slot], cartName[slot].data()).data());
	}

	void onROMMediaChange(const char *name, int slot)
	{
		string_copy(cartName[slot], name);
		updateROMText(slot);
		romSlot[slot].compile(renderer(), projP);
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		auto fPicker = EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM),
			[this, slot, dismissPreviousView](FSPicker &picker, const char* name, Input::Event e)
			{
				if(insertROM(app(), name, slot))
				{
					onROMMediaChange(name, slot);
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			});
		app().pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectROM(Input::Event e, uint8_t slot)
	{
		auto multiChoiceView = makeViewWithName<TextTableView>("ROM Cartridge Slot", 5);
		multiChoiceView->appendItem("Insert File",
			[this, slot](View &view, Input::Event e)
			{
				addROMFilePickerView(e, slot, true);
				postDraw();
			});
		multiChoiceView->appendItem("Eject",
			[this, slot](View &view, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
				onROMMediaChange("", slot);
				view.dismiss();
			});
		multiChoiceView->appendItem("Insert SCC",
			[this, slot](View &view, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCC, "", 0);
				onROMMediaChange("SCC", slot);
				view.dismiss();
			});
		multiChoiceView->appendItem("Insert SCC+",
			[this, slot](View &view, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
				onROMMediaChange("SCC+", slot);
				view.dismiss();
			});
		multiChoiceView->appendItem("Insert Sunrise IDE",
			[this, slot](View &view, Input::Event e)
			{
				if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					app().postMessage(true, "Error loading Sunrise IDE device");
				}
				else
					onROMMediaChange("Sunrise IDE", slot);
				view.dismiss();
			});
		pushAndShow(std::move(multiChoiceView), e);
	}

	TextMenuItem romSlot[2]
	{
		{nullptr, &defaultFace(), [this](Input::Event e) { onSelectROM(e, 0); }},
		{nullptr, &defaultFace(), [this](Input::Event e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];

	void updateDiskText(int slot)
	{
		diskSlot[slot].setName(string_makePrintf<1024>("%s %s", diskSlotPrefix[slot], diskName[slot].data()).data());
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		string_copy(diskName[slot], name);
		updateDiskText(slot);
		diskSlot[slot].compile(renderer(), projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		auto fPicker = EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot, dismissPreviousView](FSPicker &picker, const char* name, Input::Event e)
			{
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(app(), name, slot))
				{
					onDiskMediaChange(name, slot);
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			});
		app().pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectDisk(Input::Event e, uint8_t slot)
	{
		if(strlen(diskName[slot].data()))
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Disk Drive", std::size(insertEjectDiskMenuStr));
			multiChoiceView->appendItem(insertEjectDiskMenuStr[0],
				[this, slot](Input::Event e)
				{
					addDiskFilePickerView(e, slot, true);
					postDraw();
				});
			multiChoiceView->appendItem(insertEjectDiskMenuStr[1],
				[this, slot](View &view, Input::Event e)
				{
					diskChange(slot, 0, 0);
					onDiskMediaChange("", slot);
					view.dismiss();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
		else
		{
			addDiskFilePickerView(e, slot, false);
		}
	}

	TextMenuItem diskSlot[2]
	{
		{nullptr, &defaultFace(), [this](Input::Event e) { onSelectDisk(e, 0); }},
		{nullptr, &defaultFace(), [this](Input::Event e) { onSelectDisk(e, 1); }}
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
			[this](const TableView &, unsigned idx) -> MenuItem&
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
		"ROM/Disk Control", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active())
			{
				pushAndShow(makeView<MsxIOControlView>(), e);
			}
			else if(EmuSystem::gameIsRunning() && activeBoardType != BOARD_MSX)
			{
				app().postMessage(2, false, "Only used in MSX mode");
			}
		}
	};

	std::vector<FS::FileString> msxMachineName{};
	std::vector<TextMenuItem> msxMachineItem{};

	MultiChoiceMenuItem msxMachine
	{
		"Machine Type", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == -1)
			{
				t.setString("None");
				return true;
			}
			return false;
		},
		0,
		msxMachineItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			if(!msxMachineItem.size())
			{
				return;
			}
			item.defaultOnSelect(view, e);
		}
	};

	void reloadMachineItem()
	{
		msxMachineItem.clear();
		msxMachineName = machinesNames(machineBasePath.data());
		for(const auto &name : msxMachineName)
		{
			msxMachineItem.emplace_back(name.data(), &defaultFace(),
			[this, name = name.data()](Input::Event e)
			{
				auto ynAlertView = makeView<YesNoAlertView>("Change machine type and reset emulation?");
				ynAlertView->setOnYes(
					[this, name]()
					{
						if(auto err = setCurrentMachineName(app(), name);
							err)
						{
							app().printfMessage(3, true, "%s", err->what());
							return;
						}
						auto machineName = currentMachineName();
						strcpy(optionMachineName.val, machineName);
						msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(machineName)));
						EmuSystem::sessionOptionSet();
						dismissPrevious();
					});
				app().pushAndShowModalView(std::move(ynAlertView), e);
				return false;
			});
		}
		msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(currentMachineName())));
	}

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&msxIOControl);
		reloadMachineItem();
		item.emplace_back(&msxMachine);
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
		msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(currentMachineName())));
	}
};

static const MixerAudioType channelType[]
{
	MIXER_CHANNEL_PSG,
	MIXER_CHANNEL_SCC,
	MIXER_CHANNEL_MSXMUSIC,
	MIXER_CHANNEL_MSXAUDIO,
	MIXER_CHANNEL_MOONSOUND,
	MIXER_CHANNEL_YAMAHA_SFG,
	MIXER_CHANNEL_PCM,
};

class SoundMixerView : public TableView, public EmuAppHelper<SoundMixerView>
{
public:
	SoundMixerView(ViewAttachParams attach):
		TableView
		{
			"Sound Mixer",
			attach,
			menuItem
		}
	{}

protected:
	static constexpr unsigned CHANNEL_TYPES = std::size(channelType);

	std::array<TextHeadingMenuItem, CHANNEL_TYPES> heading
	{
		TextHeadingMenuItem{"PSG", &defaultBoldFace()},
		TextHeadingMenuItem{"SCC", &defaultBoldFace()},
		TextHeadingMenuItem{"MSX-MUSIC", &defaultBoldFace()},
		TextHeadingMenuItem{"MSX-AUDIO", &defaultBoldFace()},
		TextHeadingMenuItem{"MoonSound", &defaultBoldFace()},
		TextHeadingMenuItem{"Yamaha SFG", &defaultBoldFace()},
		TextHeadingMenuItem{"PCM", &defaultBoldFace()}
	};

	BoolMenuItem makeEnableChannel(MixerAudioType type)
	{
		return
		{
			"Output", &defaultFace(),
			(bool)mixerEnableOption(type),
			[this, type](BoolMenuItem &item, View &, Input::Event)
			{
				setMixerEnableOption(type, item.flipBoolValue(*this));
			}
		};
	}

	std::array<BoolMenuItem, CHANNEL_TYPES> enableChannel
	{
		makeEnableChannel(MIXER_CHANNEL_PSG),
		makeEnableChannel(MIXER_CHANNEL_SCC),
		makeEnableChannel(MIXER_CHANNEL_MSXMUSIC),
		makeEnableChannel(MIXER_CHANNEL_MSXAUDIO),
		makeEnableChannel(MIXER_CHANNEL_MOONSOUND),
		makeEnableChannel(MIXER_CHANNEL_YAMAHA_SFG),
		makeEnableChannel(MIXER_CHANNEL_PCM),
	};

	using ValueItemArr = std::array<TextMenuItem, 2>;

	ValueItemArr makeVolumeLevelItems(MixerAudioType type, uint8_t idx)
	{
		return
		{
			TextMenuItem{"Default Value", &defaultFace(),
				[this, type]()
				{
					setMixerVolumeOption(type, -1);
				}},
			TextMenuItem{"Custom Value", &defaultFace(),
				[this, type = (uint8_t)type, idx](Input::Event e)
				{
					app().pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 0 to 100", "",
						[this, type, idx](EmuApp &app, auto val)
						{
							if(val >= 0 && val <= 100)
							{
								setMixerVolumeOption((MixerAudioType)type, val);
								volumeLevel[idx].setSelected(std::size(volumeLevelItem[idx]) - 1, *this);
								dismissPrevious();
								return true;
							}
							else
							{
								app.postErrorMessage("Value not in range");
								return false;
							}
						});
					return false;
				}
			}
		};
	}

	std::array<ValueItemArr, CHANNEL_TYPES> volumeLevelItem
	{
		makeVolumeLevelItems(MIXER_CHANNEL_PSG, 0),
		makeVolumeLevelItems(MIXER_CHANNEL_SCC, 1),
		makeVolumeLevelItems(MIXER_CHANNEL_MSXMUSIC, 2),
		makeVolumeLevelItems(MIXER_CHANNEL_MSXAUDIO, 3),
		makeVolumeLevelItems(MIXER_CHANNEL_MOONSOUND, 4),
		makeVolumeLevelItems(MIXER_CHANNEL_YAMAHA_SFG, 5),
		makeVolumeLevelItems(MIXER_CHANNEL_PCM, 6),
	};

	MultiChoiceMenuItem makeVolumeLevel(MixerAudioType type, unsigned idx)
	{
		return
		{
			"Volume", &defaultFace(),
			[this, type](uint32_t idx, Gfx::Text &t)
			{
				t.setString(string_makePrintf<5>("%u%%", mixerVolumeOption(type)).data());
				return true;
			},
			1,
			volumeLevelItem[idx]
		};
	}

	std::array<MultiChoiceMenuItem, CHANNEL_TYPES> volumeLevel
	{
		makeVolumeLevel(MIXER_CHANNEL_PSG, 0),
		makeVolumeLevel(MIXER_CHANNEL_SCC, 1),
		makeVolumeLevel(MIXER_CHANNEL_MSXMUSIC, 2),
		makeVolumeLevel(MIXER_CHANNEL_MSXAUDIO, 3),
		makeVolumeLevel(MIXER_CHANNEL_MOONSOUND, 4),
		makeVolumeLevel(MIXER_CHANNEL_YAMAHA_SFG, 5),
		makeVolumeLevel(MIXER_CHANNEL_PCM, 6),
	};

	ValueItemArr makePanLevelItems(MixerAudioType type, uint8_t idx)
	{
		return
		{
			TextMenuItem{"Default Value", &defaultFace(),
				[this, type]()
				{
					setMixerPanOption(type, -1);
				}},
			TextMenuItem{"Custom Value", &defaultFace(),
				[this, type = (uint8_t)type, idx](Input::Event e)
				{
					app().pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 0 to 100", "",
						[this, type, idx](EmuApp &app, auto val)
						{
							if(val >= 0 && val <= 100)
							{
								setMixerPanOption((MixerAudioType)type, val);
								panLevel[idx].setSelected(std::size(panLevelItem[idx]) - 1, *this);
								dismissPrevious();
								return true;
							}
							else
							{
								app.postErrorMessage("Value not in range");
								return false;
							}
						});
					return false;
				}
			}
		};
	}

	std::array<ValueItemArr, CHANNEL_TYPES> panLevelItem
	{
		makePanLevelItems(MIXER_CHANNEL_PSG, 0),
		makePanLevelItems(MIXER_CHANNEL_SCC, 1),
		makePanLevelItems(MIXER_CHANNEL_MSXMUSIC, 2),
		makePanLevelItems(MIXER_CHANNEL_MSXAUDIO, 3),
		makePanLevelItems(MIXER_CHANNEL_MOONSOUND, 4),
		makePanLevelItems(MIXER_CHANNEL_YAMAHA_SFG, 5),
		makePanLevelItems(MIXER_CHANNEL_PCM, 6),
	};

	MultiChoiceMenuItem makePanLevel(MixerAudioType type, unsigned idx)
	{
		return
		{
			"Pan", &defaultFace(),
			[this, type](uint32_t idx, Gfx::Text &t)
			{
				t.setString(string_makePrintf<5>("%u%%", mixerPanOption(type)).data());
				return true;
			},
			1,
			panLevelItem[idx]
		};
	}

	std::array<MultiChoiceMenuItem, CHANNEL_TYPES> panLevel
	{
		makePanLevel(MIXER_CHANNEL_PSG, 0),
		makePanLevel(MIXER_CHANNEL_SCC, 1),
		makePanLevel(MIXER_CHANNEL_MSXMUSIC, 2),
		makePanLevel(MIXER_CHANNEL_MSXAUDIO, 3),
		makePanLevel(MIXER_CHANNEL_MOONSOUND, 4),
		makePanLevel(MIXER_CHANNEL_YAMAHA_SFG, 5),
		makePanLevel(MIXER_CHANNEL_PCM, 6),
	};

	std::array<MenuItem*, CHANNEL_TYPES * 4> menuItem
	{
		&heading[0],
		&enableChannel[0],
		&volumeLevel[0],
		&panLevel[0],
		&heading[1],
		&enableChannel[1],
		&volumeLevel[1],
		&panLevel[1],
		&heading[2],
		&enableChannel[2],
		&volumeLevel[2],
		&panLevel[2],
		&heading[3],
		&enableChannel[3],
		&volumeLevel[3],
		&panLevel[3],
		&heading[4],
		&enableChannel[4],
		&volumeLevel[4],
		&panLevel[4],
		&heading[5],
		&enableChannel[5],
		&volumeLevel[5],
		&panLevel[5],
		&heading[6],
		&enableChannel[6],
		&volumeLevel[6],
		&panLevel[6],
	};
};

class CustomAudioOptionView : public AudioOptionView
{
public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&mixer);
	}

protected:
	TextMenuItem mixer
	{
		"Sound Mixer", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<SoundMixerView>(), e);
		}
	};
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		default: return nullptr;
	}
}

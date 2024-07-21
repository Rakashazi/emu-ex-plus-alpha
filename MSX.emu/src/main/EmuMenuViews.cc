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

#include <emuframework/SystemOptionView.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include <emuframework/SystemActionsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/viewUtils.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/AssetFS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include "MainApp.hh"
#include <imagine/logger/logger.h>

extern "C"
{
	#include <blueMSX/IoDevice/Disk.h>
}

namespace EmuEx
{

using MainAppHelper = EmuAppHelperBase<MainApp>;

constexpr SystemLogger log{"MSX.emu"};

static std::vector<FS::FileString> readMachinesNames(IG::ApplicationContext ctx, std::string_view firmwarePath)
{
	std::vector<FS::FileString> machineNames;
	try
	{
		if(EmuApp::hasArchiveExtension(firmwarePath))
		{
			for(auto &entry : FS::ArchiveIterator{ctx.openFileUri(firmwarePath)})
			{
				if(entry.type() == FS::file_type::regular && entry.name().ends_with("/config.ini"))
				{
					auto name = FS::basename(FS::dirname(entry.name()));
					machineNames.emplace_back(name);
					log.info("found machine:{}", name);
				}
			}
		}
		else
		{
			auto machinesPath = FS::uriString(firmwarePath, "Machines");
			ctx.forEachInDirectoryUri(machinesPath, [&machineNames, &machinesPath, ctx](auto &entry)
			{
				auto configPath = FS::uriString(machinesPath, entry.name(), "config.ini");
				if(!ctx.fileUriExists(configPath))
				{
					//log.debug("{} doesn't exist", configPath);
					return true;
				}
				machineNames.emplace_back(entry.name());
				log.info("found machine:{}", entry.name());
				return true;
			});
		}
	}
	catch(...)
	{
		log.warn("no machines in:{}", firmwarePath);
	}
	if constexpr(Config::envIsAndroid)
	{
		// asset directory implementation skips directories, manually add bundled machines
		machineNames.emplace_back("MSX - C-BIOS");
		machineNames.emplace_back("MSX2 - C-BIOS");
		machineNames.emplace_back("MSX2+ - C-BIOS");
	}
	else
	{
		try
		{
			for(auto &entry : ctx.openAssetDirectory("Machines"))
			{
				machineNames.emplace_back(entry.name());
				log.info("added asset path machine:{}", entry.name());
			}
		}
		catch(...)
		{
			log.error("error opening asset directory");
		}
	}
	std::ranges::sort(machineNames, caselessLexCompare);
	// remove any duplicates
	const auto [firstDupe, lastDupe] = std::ranges::unique(machineNames);
	machineNames.erase(firstDupe, lastDupe);
	return machineNames;
}

class CustomSystemOptionView : public SystemOptionView, public MainAppHelper
{
	using MainAppHelper::system;

	std::vector<FS::FileString> machineNames{};
	std::vector<TextMenuItem> machineItems{};

	enum class MachineType {msx, coleco};

	template<MachineType type>
	MultiChoiceMenuItem::Config machineItemConfig()
	{
		return
		{
			.onSetDisplayString = [](auto idx, Gfx::Text& t)
			{
				if(idx == -1)
				{
					t.resetString("None");
					return true;
				}
				return false;
			},
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				if(!machineItems.size())
				{
					log.error("no machine definitions are present");
					return;
				}
				for(auto &&[idx, i] : enumerate(machineItems))
				{
					i.onSelect = [this, name = machineNames[idx].data()](Input::Event)
					{
						if(type == MachineType::coleco)
							system().setDefaultColecoMachineName(name);
						else
							system().setDefaultMachineName(name);
					};
				}
				item.defaultOnSelect(view, e);
			}
		};
	}

	MultiChoiceMenuItem msxMachine
	{
		"Default MSX Machine", attachParams(), 0,
		machineItems,
		machineItemConfig<MachineType::msx>()
	};

	MultiChoiceMenuItem colecoMachine
	{
		"Default Coleco Machine", attachParams(), 0,
		machineItems,
		machineItemConfig<MachineType::coleco>()
	};

	void reloadMachineItem()
	{
		machineItems.clear();
		machineNames = readMachinesNames(appContext(), system().firmwarePath());
		for(const auto &name : machineNames)
		{
			machineItems.emplace_back(name, attachParams(), nullptr);
		}
		msxMachine.setSelected(findIndex(machineNames, std::string_view{system().optionDefaultMachineNameStr}));
		colecoMachine.setSelected(findIndex(machineNames, std::string_view{system().optionDefaultColecoMachineNameStr}));
	}

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO", attachParams(),
		system().optionSkipFdcAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().optionSkipFdcAccess = item.flipBoolValue(*this);
		}
	};

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		reloadMachineItem();
		item.emplace_back(&skipFdcAccess);
		item.emplace_back(&msxMachine);
		item.emplace_back(&colecoMachine);
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper
{
	using MainAppHelper::app;
	using MainAppHelper::system;
	friend class FilePathOptionView;

	std::string machinePathMenuEntryStr(IG::CStringView path) const
	{
		return std::format("BIOS: {}", appContext().fileUriDisplayName(path));
	}

	TextMenuItem machineFilePath
	{
		machinePathMenuEntryStr(system().firmwarePath_), attachParams(),
		[this](Input::Event e)
		{
			pushAndShow(makeViewWithName<DataFolderSelectView>("BIOS",
				app().validSearchPath(system().firmwarePath_),
				[this](CStringView path, FS::file_type type)
				{
					if(type == FS::file_type::none)
					{
						system().setFirmwarePath(path, type);
					}
					else
					{
						try
						{
							system().setFirmwarePath(path, type);
						}
						catch(std::exception &err)
						{
							app().postErrorMessage(err.what());
							return false;
						}
					}
					machineFilePath.compile(machinePathMenuEntryStr(path));
					return true;
				}), e);
			postDraw();
		}
	};

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&machineFilePath);
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

class MsxIOControlView : public TableView, public MainAppHelper
{
public:
	static const char *hdSlotPrefix[4];

	void updateHDText(int slot)
	{
		hdSlot[slot].setName(std::format("{} {}", hdSlotPrefix[slot], hdName[slot]));
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

	void onHDMediaChange(std::string_view name, int slot)
	{
		hdName[slot] = name;
		updateHDText(slot);
		hdSlot[slot].place();
	}

	void addHDFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		auto fPicker = FilePicker::forMediaChange(attachParams(), e,
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot, dismissPreviousView](FSPicker &picker, std::string_view path, std::string_view name, Input::Event e)
			{
				auto id = diskGetHdDriveId(slot / 2, slot % 2);
				log.info("inserting hard drive id:{}", id);
				if(insertDisk(app(), name.data(), id))
				{
					onHDMediaChange(name.data(), slot);
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
		if(hdName[slot].size())
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
		{u"", attachParams(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 0); }},
		{u"", attachParams(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 1); }},
		{u"", attachParams(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 2); }},
		{u"", attachParams(), [this](TextMenuItem &item, Input::Event e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];

	void updateROMText(int slot)
	{
		romSlot[slot].setName(std::format("{} {}", romSlotPrefix[slot], system().cartName[slot]));
	}

	void onROMMediaChange(std::string_view name, int slot)
	{
		system().cartName[slot] = name;
		updateROMText(slot);
		romSlot[slot].place();
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		auto fPicker = FilePicker::forMediaChange(attachParams(), e,
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM),
			[this, slot, dismissPreviousView](FSPicker &picker, std::string_view path, std::string_view name, Input::Event e)
			{
				if(insertROM(app(), name.data(), slot))
				{
					onROMMediaChange(name.data(), slot);
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
		{u"", attachParams(), [this](Input::Event e) { onSelectROM(e, 0); }},
		{u"", attachParams(), [this](Input::Event e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];

	void updateDiskText(int slot)
	{
		diskSlot[slot].setName(std::format("{} {}", diskSlotPrefix[slot], system().diskName[slot]));
	}

	void onDiskMediaChange(std::string_view name, int slot)
	{
		system().diskName[slot] = name;
		updateDiskText(slot);
		diskSlot[slot].place();
	}

	void addDiskFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		auto fPicker = FilePicker::forMediaChange(attachParams(), e,
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot, dismissPreviousView](FSPicker &picker, std::string_view path, std::string_view name, Input::Event e)
			{
				log.info("inserting disk in slot:{}", slot);
				if(insertDisk(app(), name.data(), slot))
				{
					onDiskMediaChange(name.data(), slot);
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			});
		app().pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectDisk(Input::Event e, uint8_t slot)
	{
		if(system().diskName[slot].size())
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
		{u"", attachParams(), [this](Input::Event e) { onSelectDisk(e, 0); }},
		{u"", attachParams(), [this](Input::Event e) { onSelectDisk(e, 1); }}
	};

	StaticArrayList<MenuItem*, 9> item;

public:
	MsxIOControlView(ViewAttachParams attach):
		TableView
		{
			"IO Control",
			attach,
			item
		}
	{
		for(auto slot : iotaCount(2))
		{
			updateROMText(slot);
			romSlot[slot].setActive((int)slot < boardInfo.cartridgeCount);
			item.emplace_back(&romSlot[slot]);
		}
		for(auto slot : iotaCount(2))
		{
			updateDiskText(slot);
			diskSlot[slot].setActive((int)slot < boardInfo.diskdriveCount);
			item.emplace_back(&diskSlot[slot]);
		}
		for(auto slot : iotaCount(4))
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

class CustomSystemActionsView : public SystemActionsView, public MainAppHelper
{
	using MainAppHelper::system;
	using MainAppHelper::app;

private:
	TextMenuItem msxIOControl
	{
		"ROM/Disk Control", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active())
			{
				pushAndShow(makeView<MsxIOControlView>(), e);
			}
			else if(system().hasContent() && system().activeBoardType != BOARD_MSX)
			{
				app().postMessage(2, false, "Only used in MSX mode");
			}
		}
	};

	std::vector<FS::FileString> machineNames{};
	std::vector<TextMenuItem> machineItems{};

	MultiChoiceMenuItem msxMachine
	{
		"Machine Type", attachParams(),
		0,
		machineItems,
		{
			.onSetDisplayString = [](auto idx, Gfx::Text& t)
			{
				if(idx == -1)
				{
					t.resetString("None");
					return true;
				}
				return false;
			},
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				if(!machineItems.size())
				{
					return;
				}
				item.defaultOnSelect(view, e);
			}
		},
	};

	void reloadMachineItem()
	{
		machineItems.clear();
		machineNames = readMachinesNames(appContext(), system().firmwarePath());
		for(const auto &name : machineNames)
		{
			machineItems.emplace_back(name, attachParams(),
			[this, name = name.data()](Input::Event e)
			{
				app().pushAndShowModalView(makeView<YesNoAlertView>("Change machine type and reset emulation?",
					YesNoAlertView::Delegates
					{
						.onYes = [this, name]
						{
							try
							{
								system().setCurrentMachineName(app(), name);
							}
							catch(std::exception &err)
							{
								app().postMessage(3, true, err.what());
								return;
							}
							auto machineName = currentMachineName();
							system().optionSessionMachineNameStr = machineName;
							msxMachine.setSelected(findIndex(machineNames, machineName));
							system().sessionOptionSet();
							dismissPrevious();
						}
					}), e);
				return false;
			});
		}
		msxMachine.setSelected(findIndex(machineNames, currentMachineName()));
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
	CustomSystemActionsView(ViewAttachParams attach): SystemActionsView{attach, true}
	{
		reloadItems();
	}

	void onShow()
	{
		SystemActionsView::onShow();
		msxIOControl.setActive(system().hasContent() && system().activeBoardType == BOARD_MSX);
		msxMachine.setSelected(findIndex(machineNames, currentMachineName()), *this);
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

class SoundMixerView : public TableView, public MainAppHelper
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
		TextHeadingMenuItem{"PSG", attachParams(),},
		TextHeadingMenuItem{"SCC", attachParams(),},
		TextHeadingMenuItem{"MSX-MUSIC", attachParams(),},
		TextHeadingMenuItem{"MSX-AUDIO", attachParams(),},
		TextHeadingMenuItem{"MoonSound", attachParams(),},
		TextHeadingMenuItem{"Yamaha SFG", attachParams(),},
		TextHeadingMenuItem{"PCM", attachParams(),}
	};

	BoolMenuItem makeEnableChannel(MixerAudioType type)
	{
		return
		{
			"Output", attachParams(),
			system().mixerEnableOption(type),
			[this, type](BoolMenuItem &item, View &, Input::Event)
			{
				system().setMixerEnableOption(type, item.flipBoolValue(*this));
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
			TextMenuItem{"Default Value", attachParams(),
				[this, type]()
				{
					system().setMixerVolumeOption(type, -1);
				}},
			TextMenuItem{"Custom Value", attachParams(),
				[this, type = (uint8_t)type, idx](Input::Event e)
				{
					pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 0 to 100", "",
						[this, type, idx](CollectTextInputView&, auto val)
						{
							if(val >= 0 && val <= 100)
							{
								system().setMixerVolumeOption((MixerAudioType)type, val);
								volumeLevel[idx].setSelected(std::size(volumeLevelItem[idx]) - 1, *this);
								dismissPrevious();
								return true;
							}
							else
							{
								app().postErrorMessage("Value not in range");
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
			"Volume", attachParams(),
			1,
			volumeLevelItem[idx],
			{
				.onSetDisplayString = [this, type](auto idx, Gfx::Text &t)
				{
					t.resetString(std::format("{}%", system().mixerVolumeOption(type)));
					return true;
				}
			},
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
			TextMenuItem{"Default Value", attachParams(),
				[this, type]()
				{
					system().setMixerPanOption(type, -1);
				}},
			TextMenuItem{"Custom Value", attachParams(),
				[this, type = (uint8_t)type, idx](Input::Event e)
				{
					pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 0 to 100", "",
						[this, type, idx](CollectTextInputView&, auto val)
						{
							if(val >= 0 && val <= 100)
							{
								system().setMixerPanOption((MixerAudioType)type, val);
								panLevel[idx].setSelected(std::size(panLevelItem[idx]) - 1, *this);
								dismissPrevious();
								return true;
							}
							else
							{
								app().postErrorMessage("Value not in range");
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
			"Pan", attachParams(),
			1,
			panLevelItem[idx],
			{
				.onSetDisplayString = [this, type](auto idx, Gfx::Text &t)
				{
					t.resetString(std::format("{}%", system().mixerPanOption(type)));
					return true;
				}
			},
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
	CustomAudioOptionView(ViewAttachParams attach, EmuAudio& audio): AudioOptionView{attach, audio, true}
	{
		loadStockItems();
		item.emplace_back(&mixer);
	}

protected:
	TextMenuItem mixer
	{
		"Sound Mixer", attachParams(),
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
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach, audio);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		default: return nullptr;
	}
}

}

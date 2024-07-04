/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/SystemOptionView.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/VideoOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include <emuframework/SystemActionsView.hh>
#include <emuframework/MainMenuView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/viewUtils.hh>
#include "MainApp.hh"
#include "VicePlugin.hh"
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/IO.hh>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

extern "C"
{
	#include "cartridge.h"
	#include "diskimage.h"
	#include "sid/sid.h"
	#include "sid/sid-resources.h"
	#include "vicii.h"
	#include "drive.h"
	#include "datasette/datasette.h"
	#include "c64model.h"
	#include "c64dtvmodel.h"
	#include "c128model.h"
	#include "cbm2model.h"
	#include "petmodel.h"
	#include "plus4model.h"
	#include "vic20model.h"
}

namespace EmuEx
{

constexpr SystemLogger log{"C64.emu"};

using MainAppHelper = EmuAppHelperBase<MainApp>;

constexpr std::string_view driveMenuPrefix[4]
{
	"Disk 8",
	"Disk 9",
	"Disk 10",
	"Disk 11",
};

constexpr std::string_view insertEjectMenuStr[]
{
	"Insert File",
	"Eject"
};

constexpr int defaultNTSCModel[]
{
	C64MODEL_C64_NTSC,
	C64MODEL_C64_NTSC,
	DTVMODEL_V3_NTSC,
	C128MODEL_C128_NTSC,
	C64MODEL_C64_NTSC,
	CBM2MODEL_610_NTSC,
	CBM2MODEL_510_NTSC,
	PETMODEL_8032,
	PLUS4MODEL_PLUS4_NTSC,
	VIC20MODEL_VIC20_NTSC
};

constexpr int defaultPALModel[]
{
	C64MODEL_C64_PAL,
	C64MODEL_C64_PAL,
	DTVMODEL_V3_PAL,
	C128MODEL_C128_PAL,
	C64MODEL_C64_NTSC,
	CBM2MODEL_610_PAL,
	CBM2MODEL_510_PAL,
	PETMODEL_8032,
	PLUS4MODEL_PLUS4_PAL,
	VIC20MODEL_VIC20_PAL
};

constexpr size_t maxModels = std::max({int(C64MODEL_NUM), int(DTVMODEL_NUM), int(C128MODEL_NUM),
	int(CBM2MODEL_NUM), PETMODEL_NUM, PLUS4MODEL_NUM, VIC20MODEL_NUM});

static int tapeCounter = 0;

static std::span<const drive_type_info_t> driveInfoList(C64System &sys)
{
	auto infoList = sys.plugin.machine_drive_get_type_info_list();
	size_t size{};
	for(auto l = infoList; l->name != nullptr; l++)
		size++;
	return {infoList, size};
}

class CustomVideoOptionView : public VideoOptionView, public MainAppHelper
{
	using MainAppHelper::system;
	using MainAppHelper::app;

	BoolMenuItem cropNormalBorders
	{
		"Crop Normal Borders", attachParams(),
		system().optionCropNormalBorders,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().optionCropNormalBorders = item.flipBoolValue(*this);
			if(system().hasContent())
			{
				system().resetCanvasSourcePixmap(system().activeCanvas);
			}
		}
	};

	TextMenuItem borderModeItem[4]
	{
		{"Normal", attachParams(), {.id = VICII_NORMAL_BORDERS}},
		{"Full",   attachParams(), {.id = VICII_FULL_BORDERS}},
		{"Debug",  attachParams(), {.id = VICII_DEBUG_BORDERS}},
		{"None",   attachParams(), {.id = VICII_NO_BORDERS}},
	};

	MultiChoiceMenuItem borderMode
	{
		"Borders", attachParams(),
		MenuId{system().borderMode()},
		borderModeItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ system().setBorderMode(item.id); }
		}
	};

	std::vector<std::string> paletteName{};
	std::vector<TextMenuItem> paletteItem{};

	MultiChoiceMenuItem defaultPalette
	{
		"Default Palette", attachParams(),
		[this]() -> int
		{
			if(system().defaultPaletteName.empty())
				return 0;
			else
				return IG::findIndex(paletteName, system().defaultPaletteName) + 1;
		}(),
		paletteItem
	};

	TextHeadingMenuItem colorSettingsHeading{"Color Settings", attachParams()};

	template<int max = 200>
	DualTextMenuItem::SelectDelegate colorSettingDel(ColorSetting setting)
	{
		return [=, this](const Input::Event &e)
		{
			pushAndShowNewCollectValueRangeInputView<int, 0, max>(attachParams(), e,
			max == 200 ? "Input 0 to 200" : "Input 0 to 400", "",
			[=, this](CollectTextInputView&, auto val)
			{
				val *= 10;
				system().setColorSetting(setting, val);
				colorSettings[size_t(setting)].set2ndName(system().colorSettingAsString(setting));
				return true;
			});
			return false;
		};
	}

	DualTextMenuItem colorSettings[5]
	{
		{"Saturation", system().colorSettingAsString(ColorSetting::Saturation), attachParams(), colorSettingDel(ColorSetting::Saturation)},
		{"Contrast",   system().colorSettingAsString(ColorSetting::Contrast),   attachParams(), colorSettingDel(ColorSetting::Contrast)},
		{"Brightness", system().colorSettingAsString(ColorSetting::Brightness), attachParams(), colorSettingDel(ColorSetting::Brightness)},
		{"Gamma",      system().colorSettingAsString(ColorSetting::Gamma),      attachParams(), colorSettingDel<400>(ColorSetting::Gamma)},
		{"Tint",       system().colorSettingAsString(ColorSetting::Tint),       attachParams(), colorSettingDel(ColorSetting::Tint)}
	};

public:
	CustomVideoOptionView(ViewAttachParams attach, EmuVideoLayer &layer): VideoOptionView{attach, layer, true},
		paletteName{system().systemFilesWithExtension(".vpl")}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&cropNormalBorders);
		item.emplace_back(&borderMode);
		paletteItem.emplace_back("Internal", attachParams(),
			[this](Input::Event)
			{
				system().defaultPaletteName.clear();
			});
		for(const auto &name : paletteName)
		{
			paletteItem.emplace_back(IG::withoutDotExtension(name), attachParams(),
				[this, name = name.data()](Input::Event)
				{
					system().defaultPaletteName = name;
				});
		}
		item.emplace_back(&defaultPalette);
		item.emplace_back(&colorSettingsHeading);
		for(auto &i : colorSettings)
		{
			item.emplace_back(&i);
		}
	}
};

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper
{
	using MainAppHelper::system;

	TextMenuItem sidEngineItem[2]
	{
		{"FastSID", attachParams(), {.id = SID_ENGINE_FASTSID}},
		{"ReSID",   attachParams(), {.id = SID_ENGINE_RESID}},
	};

	MultiChoiceMenuItem sidEngine
	{
		"SID Engine", attachParams(),
		MenuId{system().sidEngine()},
		sidEngineItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ system().setSidEngine(item.id); }
		}
	};

	TextMenuItem reSidSamplingItem[4]
	{
		{"Fast",            attachParams(), {.id = SID_RESID_SAMPLING_FAST}},
		{"Interpolation",   attachParams(), {.id = SID_RESID_SAMPLING_INTERPOLATION}},
		{"Resampling",      attachParams(), {.id = SID_RESID_SAMPLING_RESAMPLING}},
		{"Fast Resampling", attachParams(), {.id = SID_RESID_SAMPLING_FAST_RESAMPLING}},
	};

	MultiChoiceMenuItem reSidSampling
	{
		"ReSID Sampling", attachParams(),
		MenuId{system().reSidSampling()},
		reSidSamplingItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ system().setReSidSampling(item.id); }
		}
	};

public:
	CustomAudioOptionView(ViewAttachParams attach, EmuAudio& audio): AudioOptionView{attach, audio, true}
	{
		loadStockItems();
		item.emplace_back(&sidEngine);
		item.emplace_back(&reSidSampling);
	}
};

class CustomSystemOptionView : public SystemOptionView, public MainAppHelper
{
	using MainAppHelper::system;

	StaticArrayList<TextMenuItem, maxModels> defaultModelItem;

	MultiChoiceMenuItem defaultModel
	{
		"Default Model", attachParams(),
		MenuId{system().defaultModel},
		defaultModelItem
	};

	BoolMenuItem defaultTrueDriveEmu
	{
		"Default True Drive Emulation", attachParams(),
		system().defaultDriveTrueEmulation,
		[this](BoolMenuItem &item)
		{
			system().defaultDriveTrueEmulation = item.flipBoolValue(*this);
		}
	};

	TextMenuItem joystickModeItems[3]
	{
		{toString(JoystickMode::Port1),    attachParams(), {.id = JoystickMode::Port1}},
		{toString(JoystickMode::Port2),    attachParams(), {.id = JoystickMode::Port2}},
		{toString(JoystickMode::Keyboard), attachParams(), {.id = JoystickMode::Keyboard}},
	};

	MultiChoiceMenuItem joystickMode
	{
		"Default Main Joystick Mode", attachParams(),
		MenuId{system().defaultJoystickMode},
		joystickModeItems,
		{
			.defaultItemOnSelect = [this](MenuItem &item)
			{
				system().defaultJoystickMode = JoystickMode(item.id.val);
			}
		}
	};

public:
	CustomSystemOptionView(ViewAttachParams attach):
		SystemOptionView{attach, true},
		defaultModelItem
		{
			[this]()
			{
				decltype(defaultModelItem) items{};
				for(auto i = system().plugin.modelIdBase;
					auto &name : system().plugin.modelNames)
				{
					items.emplace_back(name, attachParams(), [this](TextMenuItem &item)
					{
						system().defaultModel = item.id;
					}, MenuItem::Config{.id = i++});
				}
				return items;
			}()
		}
	{
		loadStockItems();
		item.emplace_back(&defaultModel);
		item.emplace_back(&defaultTrueDriveEmu);
		item.emplace_back(&joystickMode);
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper
{
	using MainAppHelper::app;
	using MainAppHelper::system;
	friend class FilePathOptionView;

	TextMenuItem systemFilePath
	{
		sysPathMenuEntryStr(system().sysFilePath[0]), attachParams(),
		[this](Input::Event e)
		{
			auto view = makeViewWithName<DataFolderSelectView>("VICE System Files",
				app().validSearchPath(system().sysFilePath[0]),
				[this](CStringView path, FS::file_type type)
				{
					const auto &sysFilePath = system().sysFilePath;
					if(type == FS::file_type::none && sysFilePath.size() > 1)
					{
						system().setSystemFilesPath(path, type);
						app().postMessage(5, false, std::format("Using fallback paths:\n{}\n{}", sysFilePath[1], sysFilePath[2]));
					}
					else
					{
						try
						{
							system().setSystemFilesPath(path, type);
						}
						catch(std::exception &err)
						{
							app().postErrorMessage(err.what());
							return false;
						}
					}
					systemFilePath.compile(sysPathMenuEntryStr(path));
					return true;
				});
			view->appendItem(downloadSystemFiles);
			pushAndShow(std::move(view), e);
		}
	};

	TextMenuItem downloadSystemFiles
	{
		"Download VICE System Files", attachParams(),
		[this](Input::Event e)
		{
			pushAndShowModal(makeView<YesNoAlertView>(
				"Open the C64.emu setup page? From there, download C64.emu.zip to your device and select it as an archive in the previous menu.",
				YesNoAlertView::Delegates{.onYes = [this]{ appContext().openURL("https://www.explusalpha.com/contents/c64-emu"); }}), e);
		}
	};

	std::string sysPathMenuEntryStr(IG::CStringView path)
	{
		return std::format("VICE System Files: {}", appContext().fileUriDisplayName(path));
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemFilePath);
	}
};

class DatasetteControlsView : public TableView, public MainAppHelper
{
public:
	public:
	DatasetteControlsView(ViewAttachParams attach):
		TableView
		{
			"Datasette Controls",
			attach,
			menuItem
		}
	{}

private:
	TextMenuItem stop
	{
		"Stop", attachParams(),
		[this]()
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_STOP);
			app().showEmulation();
		}
	};

	TextMenuItem start
	{
		"Start", attachParams(),
		[this]()
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_START);
			app().showEmulation();
		}
	};

	TextMenuItem forward
	{
		"Forward", attachParams(),
		[this]()
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_FORWARD);
			app().showEmulation();
		}
	};

	TextMenuItem rewind
	{
		"Rewind", attachParams(),
		[this]()
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_REWIND);
			app().showEmulation();
		}
	};

	TextMenuItem record
	{
		"Record", attachParams(),
		[this]()
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_RECORD);
			app().showEmulation();
		}
	};

	TextMenuItem reset
	{
		"Reset", attachParams(),
		[this](TextMenuItem &, View &view, Input::Event)
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_RESET);
			updateTapeCounter();
			view.place();
			app().postMessage("Tape reset");
		}
	};

	TextMenuItem resetCounter
	{
		"Reset Counter", attachParams(),
		[this](TextMenuItem &, View &view, Input::Event)
		{
			system().enterCPUTrap();
			system().plugin.datasette_control(0, DATASETTE_CONTROL_RESET_COUNTER);
			updateTapeCounter();
			view.place();
			app().postMessage("Tape counter reset");
		}
	};

	TextMenuItem tapeCounter
	{
		u"", attachParams(), nullptr
	};

	std::array<MenuItem*, 8> menuItem
	{
		&stop,
		&start,
		&forward,
		&rewind,
		&record,
		&reset,
		&resetCounter,
		&tapeCounter
	};

	void updateTapeCounter()
	{
		tapeCounter.setName(std::format("Tape Counter: {}", EmuEx::tapeCounter));
	}

	void onShow() final
	{
		updateTapeCounter();
		tapeCounter.place();
	}
};

class C64IOControlView : public TableView, public MainAppHelper
{
private:
	void updateTapeText()
	{
		auto name = system().plugin.tape_get_file_name(0);
		tapeSlot.setName(std::format("Tape: {}", name ? appContext().fileUriDisplayName(name) : ""));
		datasetteControls.setActive(name);
	}

public:
	void onTapeMediaChange()
	{
		updateTapeText();
		tapeSlot.place();
	}

	void addTapeFilePickerView(Input::Event e, bool dismissPreviousView)
	{
		app().pushAndShowModalView(
			FilePicker::forMediaChange(attachParams(), e, hasC64TapeExtension,
			[this, dismissPreviousView](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event e)
			{
				system().enterCPUTrap();
				if(system().plugin.tape_image_attach(1, path.data()) == 0)
				{
					onTapeMediaChange();
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			}, false), e);
	}

private:
	TextMenuItem tapeSlot
	{
		u"", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto name = system().plugin.tape_get_file_name(0);
			if(name && strlen(name))
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Tape Drive", std::size(insertEjectMenuStr));
				multiChoiceView->appendItem(insertEjectMenuStr[0],
					[this](View &view, Input::Event e)
					{
						addTapeFilePickerView(e, true);
					});
				multiChoiceView->appendItem(insertEjectMenuStr[1],
					[this](View &view, Input::Event e)
					{
						system().enterCPUTrap();
						system().plugin.tape_image_detach(1);
						onTapeMediaChange();
						view.dismiss();
					});
				pushAndShow(std::move(multiChoiceView), e);
			}
			else
			{
				addTapeFilePickerView(e, false);
			}
		}
	};

	TextMenuItem datasetteControls
	{
		"Datasette Controls", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			pushAndShow(makeView<DatasetteControlsView>(), e);
		}
	};

	void updateROMText()
	{
		auto name = system().plugin.cartridge_get_file_name(system().plugin.cart_getid_slotmain());
		romSlot.setName(std::format("ROM: {}", name ? appContext().fileUriDisplayName(name) : ""));
	}

public:
	void onROMMediaChange()
	{
		updateROMText();
		romSlot.place();
	}

	void addCartFilePickerView(Input::Event e, bool dismissPreviousView)
	{
		app().pushAndShowModalView(
			FilePicker::forMediaChange(attachParams(), e, hasC64CartExtension,
			[this, dismissPreviousView](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event e)
			{
				system().enterCPUTrap();
				if(system().plugin.cartridge_attach_image(systemCartType(system().currSystem), path.data()) == 0)
				{
					onROMMediaChange();
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			}, false), e);
	}

private:
	TextMenuItem romSlot
	{
		u"", attachParams(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto cartFilename = system().plugin.cartridge_get_file_name(system().plugin.cart_getid_slotmain());
			if(cartFilename && strlen(cartFilename))
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Cartridge Slot", std::size(insertEjectMenuStr));
				multiChoiceView->appendItem(insertEjectMenuStr[0],
					[this](View &view, Input::Event e)
					{
						addCartFilePickerView(e, true);
					});
				multiChoiceView->appendItem(insertEjectMenuStr[1],
					[this](View &view, Input::Event e)
					{
						system().enterCPUTrap();
						system().plugin.cartridge_detach_image(-1);
						onROMMediaChange();
						view.dismiss();
					});
				pushAndShow(std::move(multiChoiceView), e);
			}
			else
			{
				addCartFilePickerView(e, false);
			}
		}
	};

	void updateDiskText(int slot)
	{
		auto name = system().plugin.file_system_get_disk_name(slot+8, 0);
		diskSlot[slot].setName(std::format("{}: {}", driveMenuPrefix[slot], name ? appContext().fileUriDisplayName(name) : ""));
	}

	void onDiskMediaChange(int slot)
	{
		updateDiskText(slot);
		diskSlot[slot].place();
	}

	void addDiskFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		app().pushAndShowModalView(
			FilePicker::forMediaChange(attachParams(), e, hasC64DiskExtension,
			[this, slot, dismissPreviousView](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event e)
			{
				log.info("inserting disk in unit:{}", slot+8);
				system().enterCPUTrap();
				if(system().plugin.file_system_attach_disk(slot+8, 0, path.data()) == 0)
				{
					onDiskMediaChange(slot);
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			}, false), e);
	}

public:
	void onSelectDisk(Input::Event e, uint8_t slot)
	{
		auto name = system().plugin.file_system_get_disk_name(slot+8, 0);
		if(name && strlen(name))
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Disk Drive", std::size(insertEjectMenuStr));
			multiChoiceView->appendItem(insertEjectMenuStr[0],
				[this, slot](View &view, Input::Event e)
				{
					addDiskFilePickerView(e, slot, true);
				});
			multiChoiceView->appendItem(insertEjectMenuStr[1],
				[this, slot](View &view, Input::Event e)
				{
					system().enterCPUTrap();
					system().plugin.file_system_detach_disk(slot+8, 0);
					onDiskMediaChange(slot);
					view.dismiss();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
		else
		{
			addDiskFilePickerView(e, slot, false);
		}
	}

private:
	TextMenuItem diskSlot[4]
	{
		{u"", attachParams(), [this](Input::Event e) { onSelectDisk(e, 0); }},
		{u"", attachParams(), [this](Input::Event e) { onSelectDisk(e, 1); }},
		{u"", attachParams(), [this](Input::Event e) { onSelectDisk(e, 2); }},
		{u"", attachParams(), [this](Input::Event e) { onSelectDisk(e, 3); }},
	};

	int8_t currDriveTypeSlot = 0;

	StaticArrayList<TextMenuItem, 18> driveTypeItem;

	MultiChoiceMenuItem drive8Type
	{
		"Drive 8 Type", attachParams(),
		MenuId{system().driveType(8)},
		driveTypeItem,
		{
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				currDriveTypeSlot = 0;
				item.defaultOnSelect(view, e);
			}
		}
	};

	MultiChoiceMenuItem drive9Type
	{
		"Drive 9 Type", attachParams(),
		MenuId{system().driveType(9)},
		driveTypeItem,
		{
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				currDriveTypeSlot = 1;
				item.defaultOnSelect(view, e);
			}
		}
	};

	MultiChoiceMenuItem drive10Type
	{
		"Drive 10 Type", attachParams(),
		MenuId{system().driveType(10)},
		driveTypeItem,
		{
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				currDriveTypeSlot = 2;
				item.defaultOnSelect(view, e);
			}
		}
	};

	MultiChoiceMenuItem drive11Type
	{
		"Drive 11 Type", attachParams(),
		MenuId{system().driveType(11)},
		driveTypeItem,
		{
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				currDriveTypeSlot = 3;
				item.defaultOnSelect(view, e);
			}
		}
	};

	TextHeadingMenuItem mediaOptions{"Media Options", attachParams()};

	StaticArrayList<MenuItem*, 12> item;

public:
	C64IOControlView(ViewAttachParams attach):
		TableView
		{
			"System & Media",
			attach,
			item
		},
		driveTypeItem
		{
			[this]()
			{
				decltype(driveTypeItem) items{};
				for(const auto &entry : driveInfoList(system()))
				{
					items.emplace_back(entry.name, attachParams(), [this](TextMenuItem &item)
					{
						auto slot = currDriveTypeSlot;
						assumeExpr(slot < 4);
						system().sessionOptionSet();
						system().setDriveType(slot + 8, item.id);
						onDiskMediaChange(slot);
					}, MenuItem::Config{.id = entry.id});
				}
				return items;
			}()
		}
	{
		if(system().plugin.cartridge_attach_image_)
		{
			updateROMText();
			item.emplace_back(&romSlot);
		}
		for(auto slot : iotaCount(4))
		{
			updateDiskText(slot);
			item.emplace_back(&diskSlot[slot]);
		}
		updateTapeText();
		item.emplace_back(&tapeSlot);
		item.emplace_back(&datasetteControls);
		item.emplace_back(&mediaOptions);
		item.emplace_back(&drive8Type);
		item.emplace_back(&drive9Type);
		item.emplace_back(&drive10Type);
		item.emplace_back(&drive11Type);
	}
};

class Vic20MemoryExpansionsView : public TableView, public MainAppHelper
{
	void setRamBlock(const char *name, bool on)
	{
		system().sessionOptionSet();
		system().enterCPUTrap();
		system().setIntResource(name, on);
	}

	BoolMenuItem block0
	{
		"Block 0 (3KB @ $0400-$0FFF)", attachParams(),
		(bool)system().intResource("RamBlock0"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock0", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block1
	{
		"Block 1 (8KB @ $2000-$3FFF)", attachParams(),
		(bool)system().intResource("RamBlock1"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock1", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block2
	{
		"Block 2 (8KB @ $4000-$5FFF)", attachParams(),
		(bool)system().intResource("RamBlock2"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock2", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block3
	{
		"Block 3 (8KB @ $6000-$7FFF)", attachParams(),
		(bool)system().intResource("RamBlock3"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock3", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block5
	{
		"Block 5 (8KB @ $A000-$BFFF)", attachParams(),
		(bool)system().intResource("RamBlock5"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock5", item.flipBoolValue(*this));
		}
	};

	std::array<MenuItem*, 5> menuItem
	{
		&block0,
		&block1,
		&block2,
		&block3,
		&block5
	};

public:
	Vic20MemoryExpansionsView(ViewAttachParams attach):
		TableView
		{
			"Memory Expansions",
			attach,
			menuItem
		}
	{}
};

class MachineOptionView : public TableView, public MainAppHelper
{
	StaticArrayList<TextMenuItem, maxModels> modelItem;

	MultiChoiceMenuItem model
	{
		"Model", attachParams(),
		MenuId{system().model()},
		modelItem
	};

	TextMenuItem vic20MemExpansions
	{
		"Memory Expansions", attachParams(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<Vic20MemoryExpansionsView>(), e);
		}
	};

	BoolMenuItem autostartWarp
	{
		"Autostart Fast-forward", attachParams(),
		system().autostartWarp(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().setAutostartWarp(item.flipBoolValue(*this));
		}
	};

	BoolMenuItem autostartTDE
	{
		"Autostart Handles TDE", attachParams(),
		system().autostartTDE(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().setAutostartTDE(item.flipBoolValue(*this));
		}
	};

	BoolMenuItem autostartBasicLoad
	{
		"Load To BASIC Start (Disk)", attachParams(),
		bool(system().intResource("AutostartBasicLoad")),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().setIntResource("AutostartBasicLoad", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem trueDriveEmu
	{
		"True Drive Emulation (TDE)", attachParams(),
		system().driveTrueEmulation(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().setDriveTrueEmulation(item.flipBoolValue(*this));
		}
	};

	TextHeadingMenuItem videoHeader{system().videoChipStr(), attachParams()};

	std::vector<std::string> paletteName{};
	std::vector<TextMenuItem> paletteItem{};

	MultiChoiceMenuItem palette
	{
		"Palette", attachParams(),
		[this]() -> int
		{
			if(!system().usingExternalPalette())
				return 0;
			else
				return IG::findIndex(paletteName, system().externalPaletteName()) + 1;
		}(),
		paletteItem
	};

	TextHeadingMenuItem cartHeader{"Cartridges", attachParams()};

	TextMenuItem reuItem[9]
	{
		{"Off",    attachParams(), {.id = 0}},
		{"128KiB", attachParams(), {.id = 128}},
		{"256KiB", attachParams(), {.id = 256}},
		{"512KiB", attachParams(), {.id = 512}},
		{"1MiB",   attachParams(), {.id = 1024}},
		{"2MiB",   attachParams(), {.id = 2048}},
		{"4MiB",   attachParams(), {.id = 4096}},
		{"8MiB",   attachParams(), {.id = 8192}},
		{"16MiB",  attachParams(), {.id = 16384}},
	};

	MultiChoiceMenuItem reu
	{
		"Ram Expansion Module", attachParams(),
		MenuId{system().intResource("REU") ? system().intResource("REUsize") : 0},
		reuItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				system().sessionOptionSet();
				system().setReuSize(item.id);
			}
		}
	};

	StaticArrayList<MenuItem*, 10> menuItem;

public:
	MachineOptionView(ViewAttachParams attach):
		TableView
		{
			"Machine Options",
			attach,
			menuItem
		},
		modelItem
		{
			[this]()
			{
				decltype(modelItem) items{};
				for(auto i = system().plugin.modelIdBase;
					auto &name : system().plugin.modelNames)
				{
					items.emplace_back(name, attachParams(), [this](TextMenuItem &item)
					{
						system().sessionOptionSet();
						system().setModel(item.id);
					}, MenuItem::Config{.id = i++});
				}
				return items;
			}()
		},
		paletteName{system().systemFilesWithExtension(".vpl")}
	{
		menuItem.emplace_back(&model);
		if(system().currSystem == ViceSystem::VIC20)
		{
			menuItem.emplace_back(&vic20MemExpansions);
		}
		menuItem.emplace_back(&trueDriveEmu);
		menuItem.emplace_back(&autostartTDE);
		menuItem.emplace_back(&autostartBasicLoad);
		menuItem.emplace_back(&autostartWarp);
		menuItem.emplace_back(&videoHeader);
		paletteItem.emplace_back("Internal", attachParams(),
			[this](Input::Event)
			{
				system().sessionOptionSet();
				system().setPaletteResources({});
				log.info("set internal palette");
			});
		for(const auto &name : paletteName)
		{
			paletteItem.emplace_back(IG::withoutDotExtension(name), attachParams(),
				[this, name = name.data()](Input::Event)
				{
					system().sessionOptionSet();
					system().setPaletteResources(name);
					log.info("set palette:{}", name);
				});
		}
		menuItem.emplace_back(&palette);
		if(system().currSystemIsC64Or128())
		{
			menuItem.emplace_back(&cartHeader);
			menuItem.emplace_back(&reu);
		}
	}
};

class CustomSystemActionsView : public SystemActionsView, public MainAppHelper
{
	using MainAppHelper::system;
	using MainAppHelper::app;

	TextMenuItem c64IOControl
	{
		"Media Control", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			pushAndShow(makeView<C64IOControlView>(), e);
		}
	};

	TextMenuItem options
	{
		"Machine Options", attachParams(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(system().hasContent())
			{
				pushAndShow(makeView<MachineOptionView>(), e);
			}
		}
	};

	TextMenuItem quickSettings
	{
		"Apply Quick Settings & Restart", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto multiChoiceView = makeViewWithName<TextTableView>(item, 5);
			multiChoiceView->appendItem("NTSC w/ True Drive Emu",
				[this]()
				{
					system().sessionOptionSet();
					system().setDriveTrueEmulation(true);
					app().reloadSystem();
					system().setModel(defaultNTSCModel[std::to_underlying(system().currSystem)]);
					app().showEmulation();
				});
			multiChoiceView->appendItem("NTSC",
				[this]()
				{
					system().sessionOptionSet();
					system().setDriveTrueEmulation(false);
					app().reloadSystem();
					system().setModel(defaultNTSCModel[std::to_underlying(system().currSystem)]);
					app().showEmulation();
				});
			multiChoiceView->appendItem("PAL w/ True Drive Emu",
				[this]()
				{
					system().sessionOptionSet();
					system().setDriveTrueEmulation(true);
					app().reloadSystem();
					system().setModel(defaultPALModel[std::to_underlying(system().currSystem)]);
					app().showEmulation();
				});
			multiChoiceView->appendItem("PAL",
				[this]()
				{
					system().sessionOptionSet();
					system().setDriveTrueEmulation(false);
					app().reloadSystem();
					system().setModel(defaultPALModel[std::to_underlying(system().currSystem)]);
					app().showEmulation();
				});
			multiChoiceView->appendItem("Just Restart",
				[this]()
				{
					app().reloadSystem();
					app().showEmulation();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
	};

	TextMenuItem joystickModeItems[4]
	{
		{toString(JoystickMode::Auto),     attachParams(), {.id = JoystickMode::Auto}},
		{toString(JoystickMode::Port1),    attachParams(), {.id = JoystickMode::Port1}},
		{toString(JoystickMode::Port2),    attachParams(), {.id = JoystickMode::Port2}},
		{toString(JoystickMode::Keyboard), attachParams(), {.id = JoystickMode::Keyboard}},
	};

	MultiChoiceMenuItem joystickMode
	{
		"Main Joystick Mode", attachParams(),
		MenuId{system().effectiveJoystickMode},
		joystickModeItems,
		{
			.defaultItemOnSelect = [this](MenuItem &item, View &view, const Input::Event &)
			{
				system().sessionOptionSet();
				system().setJoystickMode(JoystickMode(item.id.val));
				view.dismiss();
				return false; // active item is set in onShow()
			}
		}
	};

	BoolMenuItem warpMode
	{
		"Warp Mode", attachParams(),
		(bool)*system().plugin.warp_mode_enabled,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().plugin.vsync_set_warp_mode(item.flipBoolValue(*this));
		}
	};

	BoolMenuItem autostartOnLaunch
	{
		"Autostart On Launch", attachParams(),
		system().optionAutostartOnLaunch,
		[this](BoolMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionAutostartOnLaunch = item.flipBoolValue(*this);
		}
	};

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&c64IOControl);
		item.emplace_back(&options);
		item.emplace_back(&quickSettings);
		item.emplace_back(&joystickMode);
		item.emplace_back(&warpMode);
		item.emplace_back(&autostartOnLaunch);
		loadStandardItems();
	}

public:
	CustomSystemActionsView(ViewAttachParams attach): SystemActionsView{attach, true}
	{
		reloadItems();
	}

	void onShow() final
	{
		SystemActionsView::onShow();
		c64IOControl.setActive(system().hasContent());
		options.setActive(system().hasContent());
		warpMode.setBoolValue(*system().plugin.warp_mode_enabled, *this);
		joystickMode.setSelected(MenuId{system().effectiveJoystickMode}, *this);
	}
};

class CustomMainMenuView : public MainMenuView, public MainAppHelper
{
	using MainAppHelper::app;
	using MainAppHelper::system;

	TextMenuItem systemPlugin
	{
		u"", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>(item, VicePlugin::SYSTEMS);
			for(auto i : iotaCount(VicePlugin::SYSTEMS))
			{
				multiChoiceView->appendItem(VicePlugin::systemName(ViceSystem(i)),
					[this, i](View &view, Input::Event e)
					{
						system().optionViceSystem = ViceSystem(i);
						view.dismiss(false);
						app().pushAndShowModalView(makeView<YesNoAlertView>("Changing systems needs app restart, exit now?",
							YesNoAlertView::Delegates{.onYes = [this]{ appContext().exit(); }}), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	};

	FS::FileString newMediaName;
	FS::PathString newMediaPath;

	TextMenuItem startWithBlankDisk
	{
		"Start System With Blank Disk", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectTextInputView(attachParams(), e, "Input Disk Name", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!strlen(str))
						{
							app().postMessage(true, "Name can't be blank");
							return true;
						}
						newMediaName = str;
						newMediaName.append(".d64");
						auto fPicker = FilePicker::forMediaCreation(attachParams());
						fPicker->setOnSelectPath(
							[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
							{
								newMediaPath = FS::uriString(path, newMediaName);
								picker.dismiss();
								if(e.keyEvent() && e.keyEvent()->isDefaultCancelButton())
								{
									// picker was cancelled
									app().unpostMessage();
									return;
								}
								if(appContext().fileUriExists(newMediaPath))
								{
									app().pushAndShowModalView(makeView<YesNoAlertView>("Disk image already exists, overwrite?",
										YesNoAlertView::Delegates
										{
											.onYes = [this](Input::Event e)
											{
												createDiskAndLaunch(newMediaPath.data(), newMediaName, e);
											}
										}), e);
									return;
								}
								createDiskAndLaunch(newMediaPath.data(), newMediaName, e);
							});
						view.dismiss(false);
						app().pushAndShowModalView(std::move(fPicker));
						app().postMessage("Set directory to save disk");
					}
					else
					{
						view.dismiss();
					}
					return false;
				});
		}
	};

	void launchMedia(const char *mediaPath, Input::Event e)
	{
		app().createSystemWithMedia({}, mediaPath, appContext().fileUriDisplayName(mediaPath), e, {SYSTEM_FLAG_NO_AUTOSTART}, attachParams(),
			[this](Input::Event e)
			{
				app().launchSystem(e);
			});
	}

	void createDiskAndLaunch(const char *diskPath, std::string_view diskName, Input::Event e)
	{
		if(system().plugin.vdrive_internal_create_format_disk_image(diskPath,
			IG::format<FS::FileString>("{},dsk", diskName).data(),
			DISK_IMAGE_TYPE_D64) == -1)
		{
			app().postMessage(true, "Error creating disk image");
			return;
		}
		launchMedia(diskPath, e);
	};

	TextMenuItem startWithBlankTape
	{
		"Start System With Blank Tape", attachParams(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectTextInputView(attachParams(), e, "Input Tape Name", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!strlen(str))
						{
							app().postMessage(true, "Name can't be blank");
							return true;
						}
						newMediaName = str;
						newMediaName.append(".tap");
						auto fPicker = FilePicker::forMediaCreation(attachParams());
						fPicker->setOnSelectPath(
							[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
							{
								newMediaPath = FS::uriString(path, newMediaName);
								picker.dismiss();
								if(e.keyEvent() && e.keyEvent()->isDefaultCancelButton())
								{
									// picker was cancelled
									app().unpostMessage();
									return;
								}
								if(appContext().fileUriExists(newMediaPath))
								{
									app().pushAndShowModalView(makeView<YesNoAlertView>("Tape image already exists, overwrite?",
										YesNoAlertView::Delegates
										{
											.onYes = [this](Input::Event e)
											{
												createTapeAndLaunch(newMediaPath.data(), e);
											}
										}), e);
									return;
								}
								createTapeAndLaunch(newMediaPath.data(), e);
							});
						view.dismiss(false);
						app().pushAndShowModalView(std::move(fPicker));
						app().postMessage("Set directory to save disk");
					}
					else
					{
						view.dismiss();
					}
					return false;
				});
		}
	};

	void createTapeAndLaunch(const char *tapePath, Input::Event e)
	{
		if(system().plugin.cbmimage_create_image(tapePath, DISK_IMAGE_TYPE_TAP) < 0)
		{
			app().postMessage(true, "Error creating tape image");
			return;
		}
		launchMedia(tapePath, e);
	};

	TextMenuItem loadNoAutostart
	{
		"Open Content (No Autostart)", attachParams(),
		[this](Input::Event e)
		{
			pushAndShow(FilePicker::forLoading(attachParams(), e, false, {SYSTEM_FLAG_NO_AUTOSTART}), e, false);
		}
	};

	void reloadItems() final
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&loadNoAutostart);
		item.emplace_back(&startWithBlankDisk);
		item.emplace_back(&startWithBlankTape);
		systemPlugin.setName(std::format("System: {}", VicePlugin::systemName(system().currSystem)));
		item.emplace_back(&systemPlugin);
		loadStandardItems();
	}

public:
	CustomMainMenuView(ViewAttachParams attach): MainMenuView{attach, true}
	{
		reloadItems();
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return std::make_unique<CustomMainMenuView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<CustomVideoOptionView>(attach, videoLayer);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach, audio);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		default: return nullptr;
	}
}

}

CLINK void ui_display_tape_counter(int port, int counter)
{
	//log.info("tape counter:{}", counter);
	EmuEx::tapeCounter = counter;
}

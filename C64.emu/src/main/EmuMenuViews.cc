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

#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/FilePicker.hh>
#include "MainApp.hh"
#include "VicePlugin.hh"
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>

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

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

constexpr std::string_view driveMenuPrefix[4]
{
	"Disk 8",
	"Disk 9",
	"Disk 10",
	"Disk 11",
};

constexpr const char *driveResName[4]
{
	"Drive8Type",
	"Drive9Type",
	"Drive10Type",
	"Drive11Type",
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

constexpr size_t maxModels = std::max({C64MODEL_NUM, DTVMODEL_NUM, C128MODEL_NUM,
	CBM2MODEL_NUM, PETMODEL_NUM, PLUS4MODEL_NUM, VIC20MODEL_NUM});

static int tapeCounter = 0;

static std::span<const drive_type_info_t> driveInfoList(C64System &sys)
{
	auto infoList = sys.plugin.machine_drive_get_type_info_list();
	size_t size{};
	for(auto l = infoList; l->name != nullptr; l++)
		size++;
	return {infoList, size};
}

class CustomVideoOptionView : public VideoOptionView, public MainAppHelper<CustomVideoOptionView>
{
	using MainAppHelper<CustomVideoOptionView>::system;

	BoolMenuItem cropNormalBorders
	{
		"Crop Normal Borders", &defaultFace(),
		(bool)system().optionCropNormalBorders,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().optionCropNormalBorders = item.flipBoolValue(*this);
			if(system().hasContent())
			{
				system().resetCanvasSourcePixmap(system().activeCanvas);
			}
		}
	};

	TextMenuItem::SelectDelegate setBorderModeDel()
	{
		return [this](TextMenuItem &item)
		{
			system().optionBorderMode = item.id();
			system().setBorderMode(item.id());
		};
	}

	TextMenuItem borderModeItem[4]
	{
		{"Normal", &defaultFace(), setBorderModeDel(), VICII_NORMAL_BORDERS},
		{"Full",   &defaultFace(), setBorderModeDel(), VICII_FULL_BORDERS},
		{"Debug",  &defaultFace(), setBorderModeDel(), VICII_DEBUG_BORDERS},
		{"None",   &defaultFace(), setBorderModeDel(), VICII_NO_BORDERS},
	};

	MultiChoiceMenuItem borderMode
	{
		"Borders", &defaultFace(),
		system().optionBorderMode >= std::size(borderModeItem) ? VICII_NORMAL_BORDERS : (int)system().optionBorderMode,
		borderModeItem
	};

	std::vector<std::string> paletteName{};
	std::vector<TextMenuItem> paletteItem{};

	MultiChoiceMenuItem defaultPalette
	{
		"Default Palette", &defaultFace(),
		[this]()
		{
			if(system().defaultPaletteName.empty())
				return 0;
			else
				return IG::findIndex(paletteName, system().defaultPaletteName) + 1;
		}(),
		paletteItem
	};

public:
	CustomVideoOptionView(ViewAttachParams attach):
		VideoOptionView{attach, true},
		paletteName{system().systemFilesWithExtension(".vpl")}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&cropNormalBorders);
		item.emplace_back(&borderMode);
		paletteItem.emplace_back("Internal", &defaultFace(),
			[this](Input::Event)
			{
				system().defaultPaletteName.clear();
			});
		for(const auto &name : paletteName)
		{
			paletteItem.emplace_back(IG::stringWithoutDotExtension(name), &defaultFace(),
				[this, name = name.data()](Input::Event)
				{
					system().defaultPaletteName = name;
				});
		}
		item.emplace_back(&defaultPalette);
	}
};

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::system;

	TextMenuItem sidEngineItem[2]
	{
		{"FastSID", &defaultFace(), setSidEngineDel(), SID_ENGINE_FASTSID},
		{"ReSID",   &defaultFace(), setSidEngineDel(), SID_ENGINE_RESID},
	};

	MultiChoiceMenuItem sidEngine
	{
		"SID Engine", &defaultFace(),
		[this]()
		{
			int engine = system().intResource("SidEngine");
			logMsg("current SID engine: %d", engine);
			if((unsigned)engine >= std::size(sidEngineItem))
			{
				return SID_ENGINE_FASTSID;
			}
			return engine;
		}(),
		sidEngineItem
	};

	TextMenuItem reSidSamplingItem[4]
	{
		{"Fast",            &defaultFace(), setReSidSamplingDel(), SID_RESID_SAMPLING_FAST},
		{"Interpolation",   &defaultFace(), setReSidSamplingDel(), SID_RESID_SAMPLING_INTERPOLATION},
		{"Resampling",      &defaultFace(), setReSidSamplingDel(), SID_RESID_SAMPLING_RESAMPLING},
		{"Fast Resampling", &defaultFace(), setReSidSamplingDel(), SID_RESID_SAMPLING_FAST_RESAMPLING},
	};

	MultiChoiceMenuItem reSidSampling
	{
		"ReSID Sampling", &defaultFace(),
		system().optionReSidSampling.val,
		reSidSamplingItem
	};

	TextMenuItem::SelectDelegate setSidEngineDel()
	{
		return [this](TextMenuItem &item)
		{
			system().optionSidEngine = item.id();
			system().setSidEngine(item.id());
		};
	}

	TextMenuItem::SelectDelegate setReSidSamplingDel()
	{
		return [this](TextMenuItem &item)
		{
			system().optionReSidSampling = item.id();
			system().setReSidSampling(item.id());
		};
	}

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&sidEngine);
		item.emplace_back(&reSidSampling);
	}
};

class CustomSystemOptionView : public SystemOptionView, public MainAppHelper<CustomSystemOptionView>
{
	using MainAppHelper<CustomSystemOptionView>::system;

	StaticArrayList<TextMenuItem, maxModels> defaultModelItem{};

	MultiChoiceMenuItem defaultModel
	{
		"Default Model", &defaultFace(),
		(MenuItem::Id)system().optionDefaultModel.val,
		defaultModelItem
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
					items.emplace_back(name, &defaultFace(), [this](TextMenuItem &item)
					{
						system().setDefaultModel(item.id());
					}, i++);
				}
				return items;
			}()
		}
	{
		loadStockItems();
		item.emplace_back(&defaultModel);
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper<CustomFilePathOptionView>
{
	using MainAppHelper<CustomFilePathOptionView>::app;
	using MainAppHelper<CustomFilePathOptionView>::system;

	TextMenuItem systemFilePath
	{
		sysPathMenuEntryStr(system().firmwarePath()), &defaultFace(),
		[this](Input::Event e)
		{
			auto view = makeFirmwarePathMenu("VICE System Files", true, 1);
			view->appendItem("Download VICE System Files",
				[this](Input::Event e)
				{
					auto ynAlertView = makeView<YesNoAlertView>(
						"Open the C64.emu setup page? From there, download C64.emu.zip to your device and select it as an archive in the previous menu.");
					ynAlertView->setOnYes(
						[this](Input::Event e)
						{
							appContext().openURL("https://www.explusalpha.com/contents/c64-emu");
						});
					pushAndShowModal(std::move(ynAlertView), e);
				});
			pushAndShow(std::move(view), e);
		}
	};

	bool onFirmwarePathChange(IG::CStringView path, bool isDir) final
	{
		if(isDir && !appContext().fileUriExists(FS::uriString(path, "DRIVES")))
		{
			app().postErrorMessage("Path is missing DRIVES folder");
			return false;
		}
		systemFilePath.compile(sysPathMenuEntryStr(path), renderer(), projP);
		auto &sysFilePath = system().sysFilePath;
		sysFilePath[0] = path;
		if(!path.size())
		{
			if constexpr(Config::envIsLinux)
				app().postMessage(5, false, fmt::format("Using fallback paths:\n{}\n{}", sysFilePath[3], sysFilePath[4]));
			else
			{
				app().postMessage(5, false, fmt::format("Using fallback paths:\n{}\n{}", sysFilePath[1], sysFilePath[2]));
			}
		}
		return true;
	}

	std::string sysPathMenuEntryStr(IG::CStringView path)
	{
		return fmt::format("VICE System Files: {}", path.size() ? appContext().fileUriDisplayName(path) : "");
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemFilePath);
	}
};

class DatasetteControlsView : public TableView, public MainAppHelper<DatasetteControlsView>
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
		"Stop", &defaultFace(),
		[this]()
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_STOP);
			app().showEmulation();
		}
	};

	TextMenuItem start
	{
		"Start", &defaultFace(),
		[this]()
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_START);
			app().showEmulation();
		}
	};

	TextMenuItem forward
	{
		"Forward", &defaultFace(),
		[this]()
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_FORWARD);
			app().showEmulation();
		}
	};

	TextMenuItem rewind
	{
		"Rewind", &defaultFace(),
		[this]()
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_REWIND);
			app().showEmulation();
		}
	};

	TextMenuItem record
	{
		"Record", &defaultFace(),
		[this]()
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_RECORD);
			app().showEmulation();
		}
	};

	TextMenuItem reset
	{
		"Reset", &defaultFace(),
		[this](TextMenuItem &, View &view, Input::Event)
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_RESET);
			updateTapeCounter();
			view.place();
			app().postMessage("Tape reset");
		}
	};

	TextMenuItem resetCounter
	{
		"Reset Counter", &defaultFace(),
		[this](TextMenuItem &, View &view, Input::Event)
		{
			system().plugin.datasette_control(0, DATASETTE_CONTROL_RESET_COUNTER);
			updateTapeCounter();
			view.place();
			app().postMessage("Tape counter reset");
		}
	};

	TextMenuItem tapeCounter
	{
		{}, &defaultFace(), nullptr
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
		tapeCounter.setName(fmt::format("Tape Counter: {}", EmuEx::tapeCounter));
	}

	void onShow() final
	{
		updateTapeCounter();
		tapeCounter.compile(renderer(), projP);
	}
};

class C64IOControlView : public TableView, public MainAppHelper<C64IOControlView>
{
private:
	void updateTapeText()
	{
		auto name = system().plugin.tape_get_file_name(0);
		tapeSlot.setName(fmt::format("Tape: {}", name ? appContext().fileUriDisplayName(name) : ""));
		datasetteControls.setActive(name);
	}

public:
	void onTapeMediaChange()
	{
		updateTapeText();
		tapeSlot.compile(renderer(), projP);
	}

	void addTapeFilePickerView(Input::Event e, bool dismissPreviousView)
	{
		app().pushAndShowModalView(
			EmuFilePicker::makeForMediaChange(attachParams(), e, hasC64TapeExtension,
			[this, dismissPreviousView](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event e)
			{
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
		{}, &defaultFace(),
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
		"Datasette Controls", &defaultFace(),
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
		romSlot.setName(fmt::format("ROM: {}", name ? appContext().fileUriDisplayName(name) : ""));
	}

public:
	void onROMMediaChange()
	{
		updateROMText();
		romSlot.compile(renderer(), projP);
	}

	void addCartFilePickerView(Input::Event e, bool dismissPreviousView)
	{
		app().pushAndShowModalView(
			EmuFilePicker::makeForMediaChange(attachParams(), e, hasC64CartExtension,
			[this, dismissPreviousView](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event e)
			{
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
		{}, &defaultFace(),
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
		diskSlot[slot].setName(fmt::format("{}: {}", driveMenuPrefix[slot], name ? appContext().fileUriDisplayName(name) : ""));
	}

	void onDiskMediaChange(int slot)
	{
		updateDiskText(slot);
		diskSlot[slot].compile(renderer(), projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		app().pushAndShowModalView(
			EmuFilePicker::makeForMediaChange(attachParams(), e, hasC64DiskExtension,
			[this, slot, dismissPreviousView](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event e)
			{
				logMsg("inserting disk in unit %d", slot+8);
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
		{{}, &defaultFace(), [this](Input::Event e) { onSelectDisk(e, 0); }},
		{{}, &defaultFace(), [this](Input::Event e) { onSelectDisk(e, 1); }},
		{{}, &defaultFace(), [this](Input::Event e) { onSelectDisk(e, 2); }},
		{{}, &defaultFace(), [this](Input::Event e) { onSelectDisk(e, 3); }},
	};

	unsigned currDriveTypeSlot = 0;

	StaticArrayList<TextMenuItem, 18> driveTypeItem{};

	MultiChoiceMenuItem drive8Type
	{
		"Drive 8 Type", &defaultFace(),
		(MenuItem::Id)system().intResource(driveResName[0]),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 0;
			item.defaultOnSelect(view, e);
		}
	};

	MultiChoiceMenuItem drive9Type
	{
		"Drive 9 Type", &defaultFace(),
		(MenuItem::Id)system().intResource(driveResName[1]),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 1;
			item.defaultOnSelect(view, e);
		}
	};

	MultiChoiceMenuItem drive10Type
	{
		"Drive 10 Type", &defaultFace(),
		(MenuItem::Id)system().intResource(driveResName[2]),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 2;
			item.defaultOnSelect(view, e);
		}
	};

	MultiChoiceMenuItem drive11Type
	{
		"Drive 11 Type", &defaultFace(),
		(MenuItem::Id)system().intResource(driveResName[3]),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 3;
			item.defaultOnSelect(view, e);
		}
	};

	TextHeadingMenuItem mediaOptions{"Media Options", &defaultBoldFace()};

	StaticArrayList<MenuItem*, 12> item{};

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
					items.emplace_back(entry.name, &defaultFace(), [this](TextMenuItem &item)
					{
						auto slot = currDriveTypeSlot;
						assumeExpr(slot < 4);
						system().sessionOptionSet();
						system().setIntResource(driveResName[slot], item.id());
						onDiskMediaChange(slot);
					}, entry.id);
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
		iterateTimes(4, slot)
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

class Vic20MemoryExpansionsView : public TableView, public MainAppHelper<Vic20MemoryExpansionsView>
{
	void setRamBlock(const char *name, bool on)
	{
		system().sessionOptionSet();
		system().setIntResource(name, on);
	}

	BoolMenuItem block0
	{
		"Block 0 (3KB @ $0400-$0FFF)", &defaultFace(),
		(bool)system().intResource("RamBlock0"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock0", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block1
	{
		"Block 1 (8KB @ $2000-$3FFF)", &defaultFace(),
		(bool)system().intResource("RamBlock1"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock1", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block2
	{
		"Block 2 (8KB @ $4000-$5FFF)", &defaultFace(),
		(bool)system().intResource("RamBlock2"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock2", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block3
	{
		"Block 3 (8KB @ $6000-$7FFF)", &defaultFace(),
		(bool)system().intResource("RamBlock3"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock3", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block5
	{
		"Block 5 (8KB @ $A000-$BFFF)", &defaultFace(),
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

class MachineOptionView : public TableView, public MainAppHelper<MachineOptionView>
{
	StaticArrayList<TextMenuItem, maxModels> modelItem{};

	MultiChoiceMenuItem model
	{
		"Model", &defaultFace(),
		(MenuItem::Id)system().sysModel(),
		modelItem
	};

	TextMenuItem vic20MemExpansions
	{
		"Memory Expansions", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<Vic20MemoryExpansionsView>(), e);
		}
	};

	BoolMenuItem autostartWarp
	{
		"Autostart Fast-forward", &defaultFace(),
		(bool)system().optionAutostartWarp,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionAutostartWarp = item.flipBoolValue(*this);
			system().setAutostartWarp(system().optionAutostartWarp);
		}
	};

	BoolMenuItem autostartTDE
	{
		"Autostart Handles TDE", &defaultFace(),
		(bool)system().optionAutostartTDE,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionAutostartTDE = item.flipBoolValue(*this);
			system().setAutostartTDE(system().optionAutostartTDE);
		}
	};

	BoolMenuItem autostartBasicLoad
	{
		"Autostart Basic Load (Omit ',1')", &defaultFace(),
		(bool)system().optionAutostartBasicLoad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionAutostartBasicLoad = item.flipBoolValue(*this);
			system().setAutostartBasicLoad(system().optionAutostartBasicLoad);
		}
	};

	BoolMenuItem trueDriveEmu
	{
		"True Drive Emulation (TDE)", &defaultFace(),
		(bool)system().optionDriveTrueEmulation,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionDriveTrueEmulation = item.flipBoolValue(*this);
			system().setDriveTrueEmulation(system().optionDriveTrueEmulation);
		}
	};

	TextHeadingMenuItem videoHeader{system().videoChipStr(), &defaultFace()};

	std::vector<std::string> paletteName{};
	std::vector<TextMenuItem> paletteItem{};

	MultiChoiceMenuItem palette
	{
		"Palette", &defaultFace(),
		[this]()
		{
			if(!system().usingExternalPalette())
				return 0;
			else
				return IG::findIndex(paletteName, system().externalPaletteName()) + 1;
		}(),
		paletteItem
	};

	TextHeadingMenuItem cartHeader{"Cartridges", &defaultFace()};

	TextMenuItem::SelectDelegate setReuDel(int val)
	{
		return [this, val](){ system().setRuntimeReuSize(val); };
	}

	TextMenuItem reuItem[9]
	{
		{"Off",    &defaultFace(), setReuDel(0)},
		{"128KiB", &defaultFace(), setReuDel(128)},
		{"256KiB", &defaultFace(), setReuDel(256)},
		{"512KiB", &defaultFace(), setReuDel(512)},
		{"1MiB",   &defaultFace(), setReuDel(1024)},
		{"2MiB",   &defaultFace(), setReuDel(2048)},
		{"4MiB",   &defaultFace(), setReuDel(4096)},
		{"8MiB",   &defaultFace(), setReuDel(8192)},
		{"16MiB",  &defaultFace(), setReuDel(16384)},
	};

	MultiChoiceMenuItem reu
	{
		"Ram Expansion Module", &defaultFace(),
		[this]()
		{
			system().sessionOptionSet();
			if(!system().intResource("REU"))
				return 0;
			switch(system().intResource("REUsize"))
			{
				default: return 0;
				case 128: return 1;
				case 256: return 2;
				case 512: return 3;
				case 1024: return 4;
				case 2048: return 5;
				case 4096: return 6;
				case 8192: return 7;
				case 16384: return 8;
			}
		}(),
		reuItem
	};

	StaticArrayList<MenuItem*, 10> menuItem{};

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
					items.emplace_back(name, &defaultFace(), [this](TextMenuItem &item)
					{
						system().sessionOptionSet();
						system().optionModel = item.id();
						system().setSysModel(item.id());
					}, i++);
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
		paletteItem.emplace_back("Internal", &defaultFace(),
			[this](Input::Event)
			{
				system().sessionOptionSet();
				system().setPaletteResources({});
				logMsg("set internal palette");
			});
		for(const auto &name : paletteName)
		{
			paletteItem.emplace_back(IG::stringWithoutDotExtension(name), &defaultFace(),
				[this, name = name.data()](Input::Event)
				{
					system().sessionOptionSet();
					system().setPaletteResources(name);
					logMsg("set palette:%s", name);
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

class CustomSystemActionsView : public EmuSystemActionsView, public MainAppHelper<CustomSystemActionsView>
{
	using MainAppHelper<CustomSystemActionsView>::system;
	using MainAppHelper<CustomSystemActionsView>::app;

	TextMenuItem c64IOControl
	{
		"Media Control", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			pushAndShow(makeView<C64IOControlView>(), e);
		}
	};

	TextMenuItem options
	{
		"Machine Options", &defaultFace(),
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
		"Apply Quick Settings & Restart", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto multiChoiceView = makeViewWithName<TextTableView>(item, 4);
			multiChoiceView->appendItem("NTSC w/ True Drive Emu",
				[this]()
				{
					system().sessionOptionSet();
					system().optionDriveTrueEmulation = 1;
					system().optionModel = defaultNTSCModel[to_underlying(system().currSystem)];
					app().reloadSystem();
				});
			multiChoiceView->appendItem("NTSC",
				[this]()
				{
					system().sessionOptionSet();
					system().optionDriveTrueEmulation = 0;
					system().optionModel = defaultNTSCModel[to_underlying(system().currSystem)];
					app().reloadSystem();
				});
			multiChoiceView->appendItem("PAL w/ True Drive Emu",
				[this]()
				{
					system().sessionOptionSet();
					system().optionDriveTrueEmulation = 1;
					system().optionModel = defaultPALModel[to_underlying(system().currSystem)];
					app().reloadSystem();
				});
			multiChoiceView->appendItem("PAL",
				[this]()
				{
					system().sessionOptionSet();
					system().optionDriveTrueEmulation = 0;
					system().optionModel = defaultPALModel[to_underlying(system().currSystem)];
					app().reloadSystem();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
	};

	TextMenuItem joystickModeItem[3]
	{
		{
			"Normal", &defaultFace(),
			[this]()
			{
				system().sessionOptionSet();
				system().setJoystickMode(JoystickMode::NORMAL);
			},
		},
		{
			"Swapped", &defaultFace(),
			[this]()
			{
				system().sessionOptionSet();
				system().setJoystickMode(JoystickMode::SWAPPED);
			},
		},
		{
			"Keyboard Cursor", &defaultFace(),
			[this]()
			{
				system().sessionOptionSet();
				system().setJoystickMode(JoystickMode::KEYBOARD);
			},
		},
	};

	MultiChoiceMenuItem joystickMode
	{
		"Joystick Mode", &defaultFace(),
		(int)system().optionSwapJoystickPorts,
		joystickModeItem
	};

	BoolMenuItem warpMode
	{
		"Warp Mode", &defaultFace(),
		(bool)*system().plugin.warp_mode_enabled,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().plugin.vsync_set_warp_mode(item.flipBoolValue(*this));
		}
	};

	BoolMenuItem autostartOnLaunch
	{
		"Autostart On Launch", &defaultFace(),
		(bool)system().optionAutostartOnLaunch,
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
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		reloadItems();
	}

	void onShow() final
	{
		EmuSystemActionsView::onShow();
		c64IOControl.setActive(system().hasContent());
		options.setActive(system().hasContent());
		warpMode.setBoolValue(*system().plugin.warp_mode_enabled);
	}
};

class CustomMainMenuView : public EmuMainMenuView, public MainAppHelper<CustomSystemActionsView>
{
	using MainAppHelper<CustomSystemActionsView>::app;
	using MainAppHelper<CustomSystemActionsView>::system;

	TextMenuItem systemPlugin
	{
		{}, &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>(item, VicePlugin::SYSTEMS);
			iterateTimes(VicePlugin::SYSTEMS, i)
			{
				multiChoiceView->appendItem(VicePlugin::systemName((ViceSystem)i),
					[this, i](View &view, Input::Event e)
					{
						system().optionViceSystem = i;
						auto ynAlertView = makeView<YesNoAlertView>("Changing systems needs app restart, exit now?");
						ynAlertView->setOnYes(
							[this]()
							{
								appContext().exit();
							});
						view.dismiss(false);
						app().pushAndShowModalView(std::move(ynAlertView), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	};

	FS::FileString newMediaName;
	FS::PathString newMediaPath;

	TextMenuItem startWithBlankDisk
	{
		"Start System With Blank Disk", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e, "Input Disk Name", "",
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
						auto fPicker = EmuFilePicker::makeForMediaCreation(attachParams());
						fPicker->setOnSelectPath(
							[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
							{
								newMediaPath = FS::uriString(path, newMediaName);
								picker.dismiss();
								if(e.keyEvent() && e.asKeyEvent().isDefaultCancelButton())
								{
									// picker was cancelled
									app().unpostMessage();
									return;
								}
								if(appContext().fileUriExists(newMediaPath))
								{
									auto ynAlertView = makeView<YesNoAlertView>("Disk image already exists, overwrite?");
									ynAlertView->setOnYes(
										[this](Input::Event e)
										{
											createDiskAndLaunch(newMediaPath.data(), newMediaName, e);
										});
									app().pushAndShowModalView(std::move(ynAlertView), e);
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
				app().launchSystem(e, false);
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
		"Start System With Blank Tape", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e, "Input Tape Name", "",
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
						auto fPicker = EmuFilePicker::makeForMediaCreation(attachParams());
						fPicker->setOnSelectPath(
							[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
							{
								newMediaPath = FS::uriString(path, newMediaName);
								picker.dismiss();
								if(e.keyEvent() && e.asKeyEvent().isDefaultCancelButton())
								{
									// picker was cancelled
									app().unpostMessage();
									return;
								}
								if(appContext().fileUriExists(newMediaPath))
								{
									//EmuApp::printfMessage(3, true, "%s already exists");
									auto ynAlertView = makeView<YesNoAlertView>("Tape image already exists, overwrite?");
									ynAlertView->setOnYes(
										[this](Input::Event e)
										{
											createTapeAndLaunch(newMediaPath.data(), e);
										});
									app().pushAndShowModalView(std::move(ynAlertView), e);
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
		"Open Content (No Autostart)", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(EmuFilePicker::makeForLoading(attachParams(), e, false, {SYSTEM_FLAG_NO_AUTOSTART}), e, false);
		}
	};

	void reloadItems()
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&loadNoAutostart);
		item.emplace_back(&startWithBlankDisk);
		item.emplace_back(&startWithBlankTape);
		systemPlugin.setName(fmt::format("System: {}", VicePlugin::systemName(system().currSystem)));
		item.emplace_back(&systemPlugin);
		loadStandardItems();
	}

public:
	CustomMainMenuView(ViewAttachParams attach): EmuMainMenuView{attach, true}
	{
		reloadItems();
		app().setOnMainMenuItemOptionChanged([this](){ reloadItems(); });
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return std::make_unique<CustomMainMenuView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<CustomVideoOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		default: return nullptr;
	}
}

}

CLINK void ui_display_tape_counter(int port, int counter)
{
	//logMsg("tape counter:%d", counter);
	EmuEx::tapeCounter = counter;
}

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

#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/Base.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/FilePicker.hh>
#include "internal.hh"
#include "VicePlugin.hh"

extern "C"
{
	#include "cartridge.h"
	#include "diskimage.h"
	#include "sid/sid.h"
	#include "sid/sid-resources.h"
	#include "vicii.h"
	#include "drive.h"
	#include "datasette.h"
	#include "c64model.h"
	#include "c64dtvmodel.h"
	#include "c128model.h"
	#include "cbm2model.h"
	#include "petmodel.h"
	#include "plus4model.h"
	#include "vic20model.h"
}

static constexpr const char *driveMenuPrefix[4]
{
	"Disk 8",
	"Disk 9",
	"Disk 10",
	"Disk 11",
};

static constexpr const char *driveResName[4]
{
	"Drive8Type",
	"Drive9Type",
	"Drive10Type",
	"Drive11Type",
};

static constexpr int driveTypeVal[18]
{
	DRIVE_TYPE_NONE,
	DRIVE_TYPE_1540,
	DRIVE_TYPE_1541,
	DRIVE_TYPE_1541II,
	DRIVE_TYPE_1551,
	DRIVE_TYPE_1570,
	DRIVE_TYPE_1571,
	DRIVE_TYPE_1571CR,
	DRIVE_TYPE_1581,
	DRIVE_TYPE_2000,
	DRIVE_TYPE_4000,
	DRIVE_TYPE_2031,
	DRIVE_TYPE_2040,
	DRIVE_TYPE_3040,
	DRIVE_TYPE_4040,
	DRIVE_TYPE_1001,
	DRIVE_TYPE_8050,
	DRIVE_TYPE_8250
};

static const char *insertEjectMenuStr[]
{
	"Insert File",
	"Eject"
};

static int defaultNTSCModel[]
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

static int defaultPALModel[]
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

static int tapeCounter = 0;

static constexpr const char *driveTypeStr(int type)
{
	switch(type)
	{
		case DRIVE_TYPE_NONE: return "None";
		case DRIVE_TYPE_1540: return "1540";
		case DRIVE_TYPE_1541: return "1541";
		case DRIVE_TYPE_1541II: return "1541-II";
		case DRIVE_TYPE_1551: return "1551";
		case DRIVE_TYPE_1570: return "1570";
		case DRIVE_TYPE_1571: return "1571";
		case DRIVE_TYPE_1571CR: return "1571CR";
		case DRIVE_TYPE_1581: return "1581";
		case DRIVE_TYPE_2000: return "2000";
		case DRIVE_TYPE_4000: return "4000";
		case DRIVE_TYPE_2031: return "2031";
		case DRIVE_TYPE_2040: return "2040";
		case DRIVE_TYPE_3040: return "3040";
		case DRIVE_TYPE_4040: return "4040";
		case DRIVE_TYPE_1001: return "1001";
		case DRIVE_TYPE_8050: return "8050";
		case DRIVE_TYPE_8250: return "8250";
	}
	return "?";
}

class CustomVideoOptionView : public VideoOptionView
{
	BoolMenuItem cropNormalBorders
	{
		"Crop Normal Borders",
		(bool)optionCropNormalBorders,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionCropNormalBorders = item.flipBoolValue(*this);
			if(EmuSystem::gameIsRunning())
			{
				resetCanvasSourcePixmap(activeCanvas);
			}
		}
	};

	TextMenuItem borderModeItem[4]
	{
		{"Normal", [](){ optionBorderMode = VICII_NORMAL_BORDERS; setBorderMode(VICII_NORMAL_BORDERS); }},
		{"Full", [](){ optionBorderMode = VICII_FULL_BORDERS; setBorderMode(VICII_FULL_BORDERS); }},
		{"Debug", [](){ optionBorderMode = VICII_DEBUG_BORDERS; setBorderMode(VICII_DEBUG_BORDERS); }},
		{"None", [](){ optionBorderMode = VICII_NO_BORDERS; setBorderMode(VICII_NO_BORDERS); }},
	};

	MultiChoiceMenuItem borderMode
	{
		"Borders",
		optionBorderMode >= std::size(borderModeItem) ? VICII_NORMAL_BORDERS : (int)optionBorderMode,
		borderModeItem
	};

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&cropNormalBorders);
		item.emplace_back(&borderMode);
	}
};

class CustomAudioOptionView : public AudioOptionView
{
	TextMenuItem sidEngineItem[2]
	{
		{"FastSID", [](){ setSidEngine_(SID_ENGINE_FASTSID); }},
		{"ReSID", [](){ setSidEngine_(SID_ENGINE_RESID); }},
	};

	MultiChoiceMenuItem sidEngine
	{
		"SID Engine",
		[this]()
		{
			int engine = intResource("SidEngine");
			logMsg("current SID engine: %d", engine);
			if((uint)engine >= std::size(sidEngineItem))
			{
				return SID_ENGINE_FASTSID;
			}
			return engine;
		}(),
		sidEngineItem
	};

	TextMenuItem reSidSamplingItem[4]
	{
		{"Fast", [](){ setReSidSampling_(SID_RESID_SAMPLING_FAST); }},
		{"Interpolation", [](){ setReSidSampling_(SID_RESID_SAMPLING_INTERPOLATION); }},
		{"Resampling", [](){ setReSidSampling_(SID_RESID_SAMPLING_RESAMPLING); }},
		{"Fast Resampling", [](){ setReSidSampling_(SID_RESID_SAMPLING_FAST_RESAMPLING); }},
	};

	MultiChoiceMenuItem reSidSampling
	{
		"ReSID Sampling",
		optionReSidSampling,
		reSidSamplingItem
	};

	static void setSidEngine_(int val)
	{
		optionSidEngine = val;
		setSidEngine(val);
	}

	static void setReSidSampling_(int val)
	{
		optionReSidSampling = val;
		setReSidSampling(val);
	}

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&sidEngine);
		item.emplace_back(&reSidSampling);
	}
};

class CustomSystemOptionView : public SystemOptionView
{
	TextHeadingMenuItem defaultsHeading
	{
		"Default Boot Options"
	};

	TextMenuItem defaultC64ModelItem[14]
	{
		{c64ModelStr[0], [](){ setDefaultC64Model(0); }},
		{c64ModelStr[1], [](){ setDefaultC64Model(1); }},
		{c64ModelStr[2], [](){ setDefaultC64Model(2); }},
		{c64ModelStr[3], [](){ setDefaultC64Model(3); }},
		{c64ModelStr[4], [](){ setDefaultC64Model(4); }},
		{c64ModelStr[5], [](){ setDefaultC64Model(5); }},
		{c64ModelStr[6], [](){ setDefaultC64Model(6); }},
		{c64ModelStr[7], [](){ setDefaultC64Model(7); }},
		{c64ModelStr[8], [](){ setDefaultC64Model(8); }},
		{c64ModelStr[9], [](){ setDefaultC64Model(9); }},
		{c64ModelStr[10], [](){ setDefaultC64Model(10); }},
		{c64ModelStr[11], [](){ setDefaultC64Model(11); }},
		{c64ModelStr[12], [](){ setDefaultC64Model(12); }},
		{c64ModelStr[13], [](){ setDefaultC64Model(13); }},
	};

	MultiChoiceMenuItem defaultC64Model
	{
		"C64 Model",
		optionC64Model,
		defaultC64ModelItem
	};

	TextMenuItem defaultDTVModelItem[5]
	{
		{dtvModelStr[0], [](){ setDefaultDTVModel(0); }},
		{dtvModelStr[1], [](){ setDefaultDTVModel(1); }},
		{dtvModelStr[2], [](){ setDefaultDTVModel(2); }},
		{dtvModelStr[3], [](){ setDefaultDTVModel(3); }},
		{dtvModelStr[4], [](){ setDefaultDTVModel(4); }},
	};

	MultiChoiceMenuItem defaultDTVModel
	{
		"DTV Model",
		optionDTVModel,
		defaultDTVModelItem
	};

	TextMenuItem defaultC128ModelItem[4]
	{
		{c128ModelStr[0], [](){ setDefaultC128Model(0); }},
		{c128ModelStr[1], [](){ setDefaultC128Model(1); }},
		{c128ModelStr[2], [](){ setDefaultC128Model(2); }},
		{c128ModelStr[3], [](){ setDefaultC128Model(3); }},
	};

	MultiChoiceMenuItem defaultC128Model
	{
		"C128 Model",
		optionC128Model,
		defaultC128ModelItem
	};

	TextMenuItem defaultSuperCPUModelItem[11]
	{
		{superCPUModelStr[0], [](){ setDefaultSuperCPUModel(0); }},
		{superCPUModelStr[1], [](){ setDefaultSuperCPUModel(1); }},
		{superCPUModelStr[2], [](){ setDefaultSuperCPUModel(2); }},
		{superCPUModelStr[3], [](){ setDefaultSuperCPUModel(3); }},
		{superCPUModelStr[4], [](){ setDefaultSuperCPUModel(4); }},
		{superCPUModelStr[5], [](){ setDefaultSuperCPUModel(5); }},
		{superCPUModelStr[6], [](){ setDefaultSuperCPUModel(6); }},
		{superCPUModelStr[7], [](){ setDefaultSuperCPUModel(7); }},
		{superCPUModelStr[8], [](){ setDefaultSuperCPUModel(8); }},
		{superCPUModelStr[9], [](){ setDefaultSuperCPUModel(9); }},
		{superCPUModelStr[10], [](){ setDefaultSuperCPUModel(10); }},
	};

	MultiChoiceMenuItem defaultSuperCPUModel
	{
		"C64 SuperCPU Model",
		optionSuperCPUModel,
		defaultSuperCPUModelItem
	};

	TextMenuItem defaultCBM2ModelItem[9]
	{
		{cbm2ModelStr[0], [](){ setDefaultCBM2Model(2); }},
		{cbm2ModelStr[1], [](){ setDefaultCBM2Model(3); }},
		{cbm2ModelStr[2], [](){ setDefaultCBM2Model(4); }},
		{cbm2ModelStr[3], [](){ setDefaultCBM2Model(5); }},
		{cbm2ModelStr[4], [](){ setDefaultCBM2Model(6); }},
		{cbm2ModelStr[5], [](){ setDefaultCBM2Model(7); }},
		{cbm2ModelStr[6], [](){ setDefaultCBM2Model(8); }},
		{cbm2ModelStr[7], [](){ setDefaultCBM2Model(9); }},
		{cbm2ModelStr[8], [](){ setDefaultCBM2Model(10); }},
	};

	MultiChoiceMenuItem defaultCBM2Model
	{
		"CBM-II 6x0 Model",
		optionCBM2Model - 2,
		defaultCBM2ModelItem
	};

	TextMenuItem defaultCBM5x0ModelItem[2]
	{
		{cbm5x0ModelStr[0], [](){ setDefaultCBM5x0Model(0); }},
		{cbm5x0ModelStr[1], [](){ setDefaultCBM5x0Model(1); }},
	};

	MultiChoiceMenuItem defaultCBM5x0Model
	{
		"CBM-II 5x0 Model",
		optionCBM5x0Model,
		defaultCBM5x0ModelItem
	};

	TextMenuItem defaultPETModelItem[12]
	{
		{petModelStr[0], [](){ setDefaultPETModel(0); }},
		{petModelStr[1], [](){ setDefaultPETModel(1); }},
		{petModelStr[2], [](){ setDefaultPETModel(2); }},
		{petModelStr[3], [](){ setDefaultPETModel(3); }},
		{petModelStr[4], [](){ setDefaultPETModel(4); }},
		{petModelStr[5], [](){ setDefaultPETModel(5); }},
		{petModelStr[6], [](){ setDefaultPETModel(6); }},
		{petModelStr[7], [](){ setDefaultPETModel(7); }},
		{petModelStr[8], [](){ setDefaultPETModel(8); }},
		{petModelStr[9], [](){ setDefaultPETModel(9); }},
		{petModelStr[10], [](){ setDefaultPETModel(10); }},
		{petModelStr[11], [](){ setDefaultPETModel(11); }},
	};

	MultiChoiceMenuItem defaultPetModel
	{
		"PET Model",
		optionPETModel,
		defaultPETModelItem
	};

	TextMenuItem defaultPlus4ModelItem[6]
	{
		{plus4ModelStr[0], [](){ setDefaultPlus4Model(0); }},
		{plus4ModelStr[1], [](){ setDefaultPlus4Model(1); }},
		{plus4ModelStr[2], [](){ setDefaultPlus4Model(2); }},
		{plus4ModelStr[3], [](){ setDefaultPlus4Model(3); }},
		{plus4ModelStr[4], [](){ setDefaultPlus4Model(4); }},
		{plus4ModelStr[5], [](){ setDefaultPlus4Model(5); }},
	};

	MultiChoiceMenuItem defaultPlus4Model
	{
		"Plus/4 Model",
		optionPlus4Model,
		defaultPlus4ModelItem
	};

	TextMenuItem defaultVIC20ModelItem[3]
	{
		{vic20ModelStr[0], [](){ setDefaultVIC20Model(0); }},
		{vic20ModelStr[1], [](){ setDefaultVIC20Model(1); }},
		{vic20ModelStr[2], [](){ setDefaultVIC20Model(2); }},
	};

	MultiChoiceMenuItem defaultVIC20Model
	{
		"VIC-20 Model",
		optionVIC20Model,
		defaultVIC20ModelItem
	};

	TextMenuItem systemFilePath
	{
		nullptr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pushAndShowFirmwareFilePathMenu("VICE System File Path", e);
		}
	};

	void onFirmwarePathChange(const char *path, Input::Event e) final
	{
		systemFilePath.compile(makeSysPathMenuEntryStr().data(), renderer(), projP);
		sysFilePath[0] = firmwareBasePath;
		if(!strlen(path))
		{
			if(Config::envIsLinux && !Config::MACHINE_IS_PANDORA)
				EmuApp::printfMessage(5, false, "Using default paths:\n%s\n%s\n%s", EmuApp::assetPath().data(), "~/.local/share/C64.emu", "/usr/share/games/vice");
			else
				EmuApp::printfMessage(4, false, "Using default path:\n%s/C64.emu", Base::sharedStoragePath().data());
		}
	}

	static std::array<char, 256> makeSysPathMenuEntryStr()
	{
		return string_makePrintf<256>("VICE System File Path: %s", strlen(firmwareBasePath.data()) ? FS::basename(firmwareBasePath).data() : "Default");
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		systemFilePath.setName(makeSysPathMenuEntryStr().data());
		item.emplace_back(&systemFilePath);
		item.emplace_back(&defaultsHeading);
		item.emplace_back(&defaultC64Model);
		item.emplace_back(&defaultDTVModel);
		item.emplace_back(&defaultC128Model);
		item.emplace_back(&defaultSuperCPUModel);
		item.emplace_back(&defaultCBM2Model);
		item.emplace_back(&defaultCBM5x0Model);
		item.emplace_back(&defaultPetModel);
		item.emplace_back(&defaultPlus4Model);
		item.emplace_back(&defaultVIC20Model);
	}
};

class DatasetteControlsView : public TableView
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
		"Stop",
		[this]()
		{
			plugin.datasette_control(DATASETTE_CONTROL_STOP);
			EmuApp::showEmuation();
		}
	};

	TextMenuItem start
	{
		"Start",
		[]()
		{
			plugin.datasette_control(DATASETTE_CONTROL_START);
			EmuApp::showEmuation();
		}
	};

	TextMenuItem forward
	{
		"Forward",
		[]()
		{
			plugin.datasette_control(DATASETTE_CONTROL_FORWARD);
			EmuApp::showEmuation();
		}
	};

	TextMenuItem rewind
	{
		"Rewind",
		[]()
		{
			plugin.datasette_control(DATASETTE_CONTROL_REWIND);
			EmuApp::showEmuation();
		}
	};

	TextMenuItem record
	{
		"Record",
		[]()
		{
			plugin.datasette_control(DATASETTE_CONTROL_RECORD);
			EmuApp::showEmuation();
		}
	};

	TextMenuItem reset
	{
		"Reset",
		[this](TextMenuItem &, View &view, Input::Event)
		{
			plugin.datasette_control(DATASETTE_CONTROL_RESET);
			updateTapeCounter();
			view.place();
			EmuApp::postMessage("Tape reset");
		}
	};

	TextMenuItem resetCounter
	{
		"Reset Counter",
		[this](TextMenuItem &, View &view, Input::Event)
		{
			plugin.datasette_control(DATASETTE_CONTROL_RESET_COUNTER);
			updateTapeCounter();
			view.place();
			EmuApp::postMessage("Tape counter reset");
		}
	};

	TextMenuItem tapeCounter
	{
		nullptr, nullptr
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
		tapeCounter.setName(string_makePrintf<20>("Tape Counter: %d", ::tapeCounter).data());
	}

	void onShow() final
	{
		updateTapeCounter();
		tapeCounter.compile(renderer(), projP);
	}
};

class C64IOControlView : public TableView
{
private:
	void updateTapeText()
	{
		auto name = plugin.tape_get_file_name();
		tapeSlot.setName(string_makePrintf<1024>("Tape: %s", name ? FS::basename(name).data() : "").data());
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
		EmuApp::pushAndShowModalView(
			EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(), hasC64TapeExtension,
			[this, dismissPreviousView](FSPicker &picker, const char* name, Input::Event e)
			{
				auto path = picker.makePathString(name);
				if(plugin.tape_image_attach(1, path.data()) == 0)
				{
					onTapeMediaChange();
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			}), e);
	}

private:
	TextMenuItem tapeSlot
	{
		nullptr,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto name = plugin.tape_get_file_name();
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
						plugin.tape_image_detach(1);
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
		"Datasette Controls",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			pushAndShow(makeView<DatasetteControlsView>(), e);
		}
	};

	void updateROMText()
	{
		auto name = plugin.cartridge_get_file_name(plugin.cart_getid_slotmain());
		romSlot.setName(string_makePrintf<1024>("ROM: %s", name ? FS::basename(name).data() : "").data());
	}

public:
	void onROMMediaChange()
	{
		updateROMText();
		romSlot.compile(renderer(), projP);
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

	void addCartFilePickerView(Input::Event e, bool dismissPreviousView)
	{
		EmuApp::pushAndShowModalView(
			EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(), hasC64CartExtension,
			[this, dismissPreviousView](FSPicker &picker, const char* name, Input::Event e)
			{
				auto path = picker.makePathString(name);
				if(plugin.cartridge_attach_image(systemCartType(currSystem), path.data()) == 0)
				{
					onROMMediaChange();
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			}), e);
	}

private:
	TextMenuItem romSlot
	{
		nullptr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto cartFilename = plugin.cartridge_get_file_name(plugin.cart_getid_slotmain());
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
						plugin.cartridge_detach_image(-1);
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
		auto name = plugin.file_system_get_disk_name(slot+8);
		diskSlot[slot].setName(string_makePrintf<1024>("%s: %s", driveMenuPrefix[slot], name ? FS::basename(name).data() : "").data());
	}

	void onDiskMediaChange(int slot)
	{
		updateDiskText(slot);
		diskSlot[slot].compile(renderer(), projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8_t slot, bool dismissPreviousView)
	{
		EmuApp::pushAndShowModalView(
			EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(), hasC64DiskExtension,
			[this, slot, dismissPreviousView](FSPicker &picker, const char* name, Input::Event e)
			{
				auto path = picker.makePathString(name);
				logMsg("inserting disk in unit %d", slot+8);
				if(plugin.file_system_attach_disk(slot+8, path.data()) == 0)
				{
					onDiskMediaChange(slot);
					if(dismissPreviousView)
						dismissPrevious();
				}
				picker.dismiss();
			}), e);
	}

public:
	void onSelectDisk(Input::Event e, uint8_t slot)
	{
		auto name = plugin.file_system_get_disk_name(slot+8);
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
					plugin.file_system_detach_disk(slot+8);
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
		{nullptr, [this](Input::Event e) { onSelectDisk(e, 0); }},
		{nullptr, [this](Input::Event e) { onSelectDisk(e, 1); }},
		{nullptr, [this](Input::Event e) { onSelectDisk(e, 2); }},
		{nullptr, [this](Input::Event e) { onSelectDisk(e, 3); }},
	};

	bool setDriveType(bool isActive, uint slot, int type)
	{
		assumeExpr(slot < 4);
		if(!isActive)
		{
			EmuApp::printfMessage(3, true, "Cannot use on %s", VicePlugin::systemName(currSystem));
			return false;
		}
		plugin.resources_set_int(driveResName[slot], type);
		onDiskMediaChange(slot);
		return true;
	}

	uint currDriveTypeSlot = 0;

	TextMenuItem driveTypeItem[18]
	{
		{
			driveTypeStr(DRIVE_TYPE_NONE),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_NONE); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1540),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1540); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1541),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1541); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1541II),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1541II); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1551),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1551); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1570),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1570); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1571),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1571); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1571CR),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1571CR); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1581),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1581); },
		},
		{
			driveTypeStr(DRIVE_TYPE_2000),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_2000); },
		},
		{
			driveTypeStr(DRIVE_TYPE_4000),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_4000); },
		},
		{
			driveTypeStr(DRIVE_TYPE_2031),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_2031); },
		},
		{
			driveTypeStr(DRIVE_TYPE_2040),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_2040); },
		},
		{
			driveTypeStr(DRIVE_TYPE_3040),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_3040); },
		},
		{
			driveTypeStr(DRIVE_TYPE_4040),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_4040); },
		},
		{
			driveTypeStr(DRIVE_TYPE_1001),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_1001); },
		},
		{
			driveTypeStr(DRIVE_TYPE_8050),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_8050); },
		},
		{
			driveTypeStr(DRIVE_TYPE_8250),
			[this](TextMenuItem &item, View &, Input::Event){ return setDriveType(item.active(), currDriveTypeSlot, DRIVE_TYPE_8250); },
		}
	};

	static int driveTypeMenuIdx(int type)
	{
		switch(type)
		{
			default:
			case DRIVE_TYPE_NONE: return 0;
			case DRIVE_TYPE_1540: return 1;
			case DRIVE_TYPE_1541: return 2;
			case DRIVE_TYPE_1541II: return 3;
			case DRIVE_TYPE_1551: return 4;
			case DRIVE_TYPE_1570: return 5;
			case DRIVE_TYPE_1571: return 6;
			case DRIVE_TYPE_1571CR: return 7;
			case DRIVE_TYPE_1581: return 8;
			case DRIVE_TYPE_2000: return 9;
			case DRIVE_TYPE_4000: return 10;
			case DRIVE_TYPE_2031: return 11;
			case DRIVE_TYPE_2040: return 12;
			case DRIVE_TYPE_3040: return 13;
			case DRIVE_TYPE_4040: return 14;
			case DRIVE_TYPE_1001: return 15;
			case DRIVE_TYPE_8050: return 16;
			case DRIVE_TYPE_8250: return 17;
		}
	}

	MultiChoiceMenuItem drive8Type
	{
		"Drive 8 Type",
		driveTypeMenuIdx(intResource(driveResName[0])),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 0;
			iterateTimes(std::size(driveTypeItem), i)
			{
				driveTypeItem[i].setActive(plugin.drive_check_type(driveTypeVal[i], currDriveTypeSlot));
			}
			item.defaultOnSelect(view, e);
		}
	};

	MultiChoiceMenuItem drive9Type
	{
		"Drive 9 Type",
		driveTypeMenuIdx(intResource(driveResName[1])),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 1;
			iterateTimes(std::size(driveTypeItem), i)
			{
				driveTypeItem[i].setActive(plugin.drive_check_type(driveTypeVal[i], currDriveTypeSlot));
			}
			item.defaultOnSelect(view, e);
		}
	};

	MultiChoiceMenuItem drive10Type
	{
		"Drive 10 Type",
		driveTypeMenuIdx(intResource(driveResName[2])),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 2;
			iterateTimes(std::size(driveTypeItem), i)
			{
				driveTypeItem[i].setActive(plugin.drive_check_type(driveTypeVal[i], currDriveTypeSlot));
			}
			item.defaultOnSelect(view, e);
		}
	};

	MultiChoiceMenuItem drive11Type
	{
		"Drive 11 Type",
		driveTypeMenuIdx(intResource(driveResName[3])),
		driveTypeItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			currDriveTypeSlot = 3;
			iterateTimes(std::size(driveTypeItem), i)
			{
				driveTypeItem[i].setActive(plugin.drive_check_type(driveTypeVal[i], currDriveTypeSlot));
			}
			item.defaultOnSelect(view, e);
		}
	};

	TextHeadingMenuItem mediaOptions{"Media Options"};

	StaticArrayList<MenuItem*, 12> item{};

public:
	C64IOControlView(ViewAttachParams attach):
		TableView
		{
			"System & Media",
			attach,
			item
		}
	{
		if(plugin.cartridge_attach_image_)
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

class Vic20MemoryExpansionsView : public TableView
{
	void setRamBlock(const char *name, bool on)
	{
		EmuSystem::sessionOptionSet();
		plugin.resources_set_int(name, on);
	}

	BoolMenuItem block0
	{
		"Block 0 (3KB @ $0400-$0FFF)",
		(bool)intResource("RamBlock0"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock0", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block1
	{
		"Block 1 (8KB @ $2000-$3FFF)",
		(bool)intResource("RamBlock1"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock1", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block2
	{
		"Block 2 (8KB @ $4000-$5FFF)",
		(bool)intResource("RamBlock2"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock2", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block3
	{
		"Block 3 (8KB @ $6000-$7FFF)",
		(bool)intResource("RamBlock3"),
		[this](BoolMenuItem &item)
		{
			setRamBlock("RamBlock3", item.flipBoolValue(*this));
		}
	};

	BoolMenuItem block5
	{
		"Block 5 (8KB @ $A000-$BFFF)",
		(bool)intResource("RamBlock5"),
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

class MachineOptionView : public TableView
{
	std::vector<TextMenuItem> modelItem{};

	MultiChoiceMenuItem model
	{
		"Model",
		[]()
		{
			auto modelVal = sysModel();
			auto baseVal = currSystem == VICE_SYSTEM_CBM2 ? 2 : 0;
			if(modelVal < baseVal || modelVal >= plugin.models + baseVal)
			{
				return baseVal;
			}
			return modelVal;
		}(),
		modelItem
	};

	TextMenuItem vic20MemExpansions
	{
		"Memory Expansions",
		[this](Input::Event e)
		{
			pushAndShow(makeView<Vic20MemoryExpansionsView>(), e);
		}
	};

	BoolMenuItem autostartWarp
	{
		"Autostart Fast-forward",
		(bool)optionAutostartWarp,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionAutostartWarp = item.flipBoolValue(*this);
			setAutostartWarp(optionAutostartWarp);
		}
	};

	BoolMenuItem autostartTDE
	{
		"Autostart Handles TDE",
		(bool)optionAutostartTDE,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionAutostartTDE = item.flipBoolValue(*this);
			setAutostartTDE(optionAutostartTDE);
		}
	};

	BoolMenuItem autostartBasicLoad
	{
		"Autostart Basic Load (Omit ',1')",
		(bool)optionAutostartBasicLoad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionAutostartBasicLoad = item.flipBoolValue(*this);
			setAutostartBasicLoad(optionAutostartBasicLoad);
		}
	};

	BoolMenuItem trueDriveEmu
	{
		"True Drive Emulation (TDE)",
		(bool)optionDriveTrueEmulation,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionDriveTrueEmulation = item.flipBoolValue(*this);
			setDriveTrueEmulation(optionDriveTrueEmulation);
			if(!optionDriveTrueEmulation && !optionVirtualDeviceTraps)
			{
				EmuApp::postMessage("Enable Virtual Device Traps to use disks without TDE");
			}
		}
	};

	BoolMenuItem virtualDeviceTraps
	{
		"Virtual Device Traps",
		(bool)optionVirtualDeviceTraps,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionVirtualDeviceTraps = item.flipBoolValue(*this);
			setVirtualDeviceTraps(optionVirtualDeviceTraps);
		}
	};

	StaticArrayList<MenuItem*, 7> menuItem{};

public:
	MachineOptionView(ViewAttachParams attach):
		TableView
		{
			"Machine Options",
			attach,
			menuItem
		}
	{
		modelItem.reserve(plugin.models);
		auto baseVal = currSystem == VICE_SYSTEM_CBM2 ? 2 : 0;
		iterateTimes(plugin.models, i)
		{
			int val = baseVal + i;
			modelItem.emplace_back(plugin.modelStr[i],
				[val]()
				{
					EmuSystem::sessionOptionSet();
					optionModel = val;
					setSysModel(val);
				});
		}
		menuItem.emplace_back(&model);
		if(currSystem == VICE_SYSTEM_VIC20)
		{
			menuItem.emplace_back(&vic20MemExpansions);
		}
		menuItem.emplace_back(&trueDriveEmu);
		menuItem.emplace_back(&autostartTDE);
		menuItem.emplace_back(&autostartBasicLoad);
		menuItem.emplace_back(&autostartWarp);
		menuItem.emplace_back(&virtualDeviceTraps);
	}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
	TextMenuItem c64IOControl
	{
		"Media Control",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			pushAndShow(makeView<C64IOControlView>(), e);
		}
	};

	TextMenuItem options
	{
		"Machine Options",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				pushAndShow(makeView<MachineOptionView>(), e);
			}
		}
	};

	TextMenuItem quickSettings
	{
		"Apply Quick Settings & Restart",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto multiChoiceView = makeViewWithName<TextTableView>(item, 4);
			multiChoiceView->appendItem("NTSC w/ True Drive Emu",
				[this]()
				{
					EmuSystem::sessionOptionSet();
					optionVirtualDeviceTraps = 0;
					optionDriveTrueEmulation = 1;
					optionModel = defaultNTSCModel[currSystem];
					EmuApp::reloadGame();
				});
			multiChoiceView->appendItem("NTSC",
				[this]()
				{
					EmuSystem::sessionOptionSet();
					optionVirtualDeviceTraps = 1;
					optionDriveTrueEmulation = 0;
					optionModel = defaultNTSCModel[currSystem];
					EmuApp::reloadGame();
				});
			multiChoiceView->appendItem("PAL w/ True Drive Emu",
				[this]()
				{
					EmuSystem::sessionOptionSet();
					optionVirtualDeviceTraps = 0;
					optionDriveTrueEmulation = 1;
					optionModel = defaultPALModel[currSystem];
					EmuApp::reloadGame();
				});
			multiChoiceView->appendItem("PAL",
				[this]()
				{
					EmuSystem::sessionOptionSet();
					optionVirtualDeviceTraps = 1;
					optionDriveTrueEmulation = 0;
					optionModel = defaultPALModel[currSystem];
					EmuApp::reloadGame();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
	};

	BoolMenuItem swapJoystickPorts
	{
		"Swap Joystick Ports",
		(bool)optionSwapJoystickPorts,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionSwapJoystickPorts = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem warpMode
	{
		"Warp Mode",
		(bool)*plugin.warp_mode_enabled,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			*plugin.warp_mode_enabled = item.flipBoolValue(*this);
		}
	};

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&c64IOControl);
		item.emplace_back(&options);
		item.emplace_back(&quickSettings);
		item.emplace_back(&swapJoystickPorts);
		item.emplace_back(&warpMode);
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
		c64IOControl.setActive(EmuSystem::gameIsRunning());
		options.setActive(EmuSystem::gameIsRunning());
		warpMode.setBoolValue(*plugin.warp_mode_enabled);
	}
};

class CustomMainMenuView : public EmuMainMenuView
{
	TextMenuItem system
	{
		nullptr,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>(item, VicePlugin::SYSTEMS);
			iterateTimes(VicePlugin::SYSTEMS, i)
			{
				multiChoiceView->appendItem(VicePlugin::systemName((ViceSystem)i),
					[this, i](View &view, Input::Event e)
					{
						optionViceSystem = i;
						auto ynAlertView = makeView<YesNoAlertView>("Changing systems needs app restart, exit now?");
						ynAlertView->setOnYes(
							[]()
							{
								Base::exit();
							});
						view.dismiss(false);
						EmuApp::pushAndShowModalView(std::move(ynAlertView), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	};

	FS::FileString newDiskName;
	FS::PathString newDiskPath;

	TextMenuItem startWithBlankDisk
	{
		"Start System With Blank Disk",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e, "Input Disk Name", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!strlen(str))
						{
							EmuApp::postMessage(true, "Name can't be blank");
							return 1;
						}
						string_copy(newDiskName, str);
						auto fPicker = EmuFilePicker::makeForMediaCreation(attachParams(), Input::defaultEvent());
						fPicker->setOnClose(
							[this](FSPicker &picker, Input::Event e)
							{
								newDiskPath = FS::makePathStringPrintf("%s/%s.d64", picker.path().data(), newDiskName.data());
								picker.dismiss();
								if(e.isDefaultCancelButton())
								{
									// picker was cancelled
									EmuApp::unpostMessage();
									return;
								}
								if(FS::exists(newDiskPath))
								{
									//EmuApp::printfMessage(3, true, "%s already exists");
									auto ynAlertView = makeView<YesNoAlertView>("Disk image already exists, overwrite?");
									ynAlertView->setOnYes(
										[this](Input::Event e)
										{
											createDiskAndLaunch(newDiskPath.data(), newDiskName.data(), e);
										});
									EmuApp::pushAndShowModalView(std::move(ynAlertView), e);
									return;
								}
								createDiskAndLaunch(newDiskPath.data(), newDiskName.data(), e);
							});
						view.dismiss(false);
						EmuApp::pushAndShowModalView(std::move(fPicker), Input::defaultEvent());
						EmuApp::postMessage("Set directory to save disk");
					}
					else
					{
						view.dismiss();
					}
					return 0;
				});
		}
	};

	static void createDiskAndLaunch(const char *diskPath, const char *diskName, Input::Event e)
	{
		if(plugin.vdrive_internal_create_format_disk_image(diskPath,
			FS::makeFileStringPrintf("%s,dsk", diskName).data(),
			DISK_IMAGE_TYPE_D64) == -1)
		{
			EmuApp::postMessage(true, "Error creating disk image");
			return;
		}
		autostartOnLoad = false;
		EmuApp::createSystemWithMedia({}, diskPath, "", e,
			[](Input::Event e)
			{
				EmuApp::launchSystem(e, false, true);
			});
	};

	void reloadItems()
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&startWithBlankDisk);
		system.setName(string_makePrintf<34>("System: %s", VicePlugin::systemName(currSystem)).data());
		item.emplace_back(&system);
		loadStandardItems();
	}

public:
	CustomMainMenuView(ViewAttachParams attach): EmuMainMenuView{attach, true}
	{
		reloadItems();
		EmuApp::setOnMainMenuItemOptionChanged([this](){ reloadItems(); });
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
		default: return nullptr;
	}
}

CLINK void ui_display_tape_counter(int counter)
{
	//logMsg("tape counter:%d", counter);
	tapeCounter = counter;
}

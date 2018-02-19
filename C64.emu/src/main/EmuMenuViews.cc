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
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuMainMenuView.hh>
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
			resetCanvasSourcePixmap(activeCanvas);
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
		optionBorderMode >= IG::size(borderModeItem) ? VICII_NORMAL_BORDERS : (int)optionBorderMode,
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
			if((uint)engine >= IG::size(sidEngineItem))
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
	BoolMenuItem autostartWarp
	{
		"Autostart Fast-forward",
		(bool)optionAutostartWarp,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
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
			optionAutostartTDE = item.flipBoolValue(*this);
			setAutostartTDE(optionAutostartTDE);		}
	};

	TextHeadingMenuItem defaultsHeading
	{
		"Default Boot Options"
	};

	BoolMenuItem trueDriveEmu
	{
		"True Drive Emulation (TDE)",
		(bool)optionDriveTrueEmulation,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionDriveTrueEmulation = item.flipBoolValue(*this);
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
			optionVirtualDeviceTraps = item.flipBoolValue(*this);
		}
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

	char systemFilePathStr[256]{};

	TextMenuItem systemFilePath
	{
		systemFilePathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pushAndShowFirmwarePathMenu("System File Path", e);
			postDraw();
		}
	};

	void onFirmwarePathChange(const char *path, Input::Event e) final
	{
		printSysPathMenuEntryStr(systemFilePathStr);
		systemFilePath.compile(renderer(), projP);
		sysFilePath[0] = firmwareBasePath;
		if(!strlen(path))
		{
			if(Config::envIsLinux && !Config::MACHINE_IS_PANDORA)
				EmuApp::printfMessage(5, false, "Using default paths:\n%s\n%s\n%s", EmuApp::assetPath().data(), "~/.local/share/C64.emu", "/usr/share/games/vice");
			else
				EmuApp::printfMessage(4, false, "Using default path:\n%s/C64.emu", Base::sharedStoragePath().data());
		}
	}

	template <size_t S>
	static void printSysPathMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "System File Path: %s", strlen(firmwareBasePath.data()) ? FS::basename(firmwareBasePath).data() : "Default");
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		printSysPathMenuEntryStr(systemFilePathStr);
		item.emplace_back(&systemFilePath);
		item.emplace_back(&autostartTDE);
		item.emplace_back(&autostartWarp);
		item.emplace_back(&defaultsHeading);
		item.emplace_back(&trueDriveEmu);
		item.emplace_back(&virtualDeviceTraps);
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

static const char *insertEjectMenuStr[] { "Insert File", "Eject" };

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
	void onTapeMediaChange()
	{
		updateTapeText();
		tapeSlot.compile(renderer(), projP);
	}

	void addTapeFilePickerView(Input::Event e)
	{
		auto &fPicker = *EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(), hasC64TapeExtension,
			[this](FSPicker &picker, const char* name, Input::Event e)
			{
				auto path = picker.makePathString(name);
				if(plugin.tape_image_attach(1, path.data()) == 0)
				{
					onTapeMediaChange();
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(fPicker, e);
	}

private:
	TextMenuItem tapeSlot
	{
		tapeSlotStr,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto name = plugin.tape_get_file_name();
			if(name && strlen(name))
			{
				auto &multiChoiceView = *new TextTableView{"Tape Drive", attachParams(), IG::size(insertEjectMenuStr)};
				multiChoiceView.appendItem(insertEjectMenuStr[0],
					[this](TextMenuItem &, View &, Input::Event e)
					{
						addTapeFilePickerView(e);
						postDraw();
						popAndShow();
					});
				multiChoiceView.appendItem(insertEjectMenuStr[1],
					[this](TextMenuItem &, View &, Input::Event e)
					{
						plugin.tape_image_detach(1);
						onTapeMediaChange();
						popAndShow();
					});
				pushAndShow(multiChoiceView, e);
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

	void addCartFilePickerView(Input::Event e)
	{
		auto &fPicker = *EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(), hasC64CartExtension,
			[this](FSPicker &picker, const char* name, Input::Event e)
			{
				auto path = picker.makePathString(name);
				if(plugin.cartridge_attach_image(systemCartType(currSystem), path.data()) == 0)
				{
					onROMMediaChange();
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(fPicker, e);
	}

private:
	TextMenuItem romSlot
	{
		romSlotStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto cartFilename = plugin.cartridge_get_file_name(plugin.cart_getid_slotmain());
			if(cartFilename && strlen(cartFilename))
			{
				auto &multiChoiceView = *new TextTableView{"Cartridge Slot", attachParams(), IG::size(insertEjectMenuStr)};
				multiChoiceView.appendItem(insertEjectMenuStr[0],
					[this](TextMenuItem &, View &, Input::Event e)
					{
						addCartFilePickerView(e);
						postDraw();
						popAndShow();
					});
				multiChoiceView.appendItem(insertEjectMenuStr[1],
					[this](TextMenuItem &, View &, Input::Event e)
					{
						plugin.cartridge_detach_image(-1);
						onROMMediaChange();
						popAndShow();
					});
				pushAndShow(multiChoiceView, e);
			}
			else
			{
				addCartFilePickerView(e);
			}
			window().postDraw();
		}
	};

	char diskSlotStr[4][1024]{};

	void updateDiskText(int slot)
	{
		auto name = plugin.file_system_get_disk_name(slot+8);
		string_printf(diskSlotStr[slot], "%s: %s", driveMenuPrefix[slot], name ? FS::basename(name).data() : "");
	}

	void onDiskMediaChange(int slot)
	{
		updateDiskText(slot);
		diskSlot[slot].compile(renderer(), projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8 slot)
	{
		auto &fPicker = *EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(), hasC64DiskExtension,
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				auto path = picker.makePathString(name);
				logMsg("inserting disk in unit %d", slot+8);
				if(plugin.file_system_attach_disk(slot+8, path.data()) == 0)
				{
					onDiskMediaChange(slot);
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(fPicker, e);
	}

public:
	void onSelectDisk(Input::Event e, uint8 slot)
	{
		auto name = plugin.file_system_get_disk_name(slot+8);
		if(name && strlen(name))
		{
			auto &multiChoiceView = *new TextTableView{"Disk Drive", attachParams(), IG::size(insertEjectMenuStr)};
			multiChoiceView.appendItem(insertEjectMenuStr[0],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addDiskFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView.appendItem(insertEjectMenuStr[1],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					plugin.file_system_detach_disk(slot+8);
					onDiskMediaChange(slot);
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

private:
	TextMenuItem diskSlot[4]
	{
		{diskSlotStr[0], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 0); }},
		{diskSlotStr[1], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 1); }},
		{diskSlotStr[2], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 2); }},
		{diskSlotStr[3], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 3); }},
	};

	BoolMenuItem trueDriveEmu
	{
		"True Drive Emulation (TDE)",
		driveTrueEmulation(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			setDriveTrueEmulation(item.flipBoolValue(*this));
			if(!driveTrueEmulation() && !virtualDeviceTraps())
			{
				EmuApp::postMessage("Enable Virtual Device Traps to use disks without TDE");
			}
		}
	};

	BoolMenuItem virtualDeviceTraps_
	{
		"Virtual Device Traps",
		virtualDeviceTraps(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			setVirtualDeviceTraps(item.flipBoolValue(*this));
		}
	};

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
			iterateTimes(IG::size(driveTypeItem), i)
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
			iterateTimes(IG::size(driveTypeItem), i)
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
			iterateTimes(IG::size(driveTypeItem), i)
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
			iterateTimes(IG::size(driveTypeItem), i)
			{
				driveTypeItem[i].setActive(plugin.drive_check_type(driveTypeVal[i], currDriveTypeSlot));
			}
			item.defaultOnSelect(view, e);
		}
	};

	TextHeadingMenuItem mediaOptions{"Media Options"};
	TextHeadingMenuItem systemOptions{"System Options"};

	StaticArrayList<MenuItem*, 15> item{};

public:
	C64IOControlView(ViewAttachParams attach):
		TableView
		{
			"System & Media",
			attach,
			item
		}
	{
		modelItem.reserve(plugin.models);
		auto baseVal = currSystem == VICE_SYSTEM_CBM2 ? 2 : 0;
		iterateTimes(plugin.models, i)
		{
			int val = baseVal + i;
			modelItem.emplace_back(plugin.modelStr[i], [val](){ setSysModel(val); });
		}

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
		item.emplace_back(&mediaOptions);
		item.emplace_back(&drive8Type);
		item.emplace_back(&drive9Type);
		item.emplace_back(&drive10Type);
		item.emplace_back(&drive11Type);
		item.emplace_back(&trueDriveEmu);
		item.emplace_back(&virtualDeviceTraps_);
		item.emplace_back(&systemOptions);
		item.emplace_back(&model);
	}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
	BoolMenuItem swapJoystickPorts
	{
		"Swap Joystick Ports",
		(bool)optionSwapJoystickPorts,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
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

	TextMenuItem c64IOControl
	{
		"System & Media Control",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto &c64IoMenu = *new C64IOControlView{attachParams()};
			pushAndShow(c64IoMenu, e);
		}
	};

	TextMenuItem quickSettings
	{
		"Apply Runtime Settings & Autostart",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!item.active())
				return;
			auto &multiChoiceView = *new TextTableView{item.t.str, attachParams(), 4};
			multiChoiceView.appendItem("1. NTSC & True Drive Emu",
				[this]()
				{
					popAndShow();
					EmuApp::reloadGame();
					setVirtualDeviceTraps(false);
					setDriveTrueEmulation(true);
					setDefaultNTSCModel();
				});
			multiChoiceView.appendItem("2. NTSC",
				[this]()
				{
					popAndShow();
					EmuApp::reloadGame();
					setVirtualDeviceTraps(true);
					setDriveTrueEmulation(false);
					setDefaultNTSCModel();
				});
			multiChoiceView.appendItem("3. PAL & True Drive Emu",
				[this]()
				{
					popAndShow();
					EmuApp::reloadGame();
					setVirtualDeviceTraps(false);
					setDriveTrueEmulation(true);
					setDefaultPALModel();
				});
			multiChoiceView.appendItem("4. PAL",
				[this]()
				{
					popAndShow();
					EmuApp::reloadGame();
					setVirtualDeviceTraps(true);
					setDriveTrueEmulation(false);
					setDefaultPALModel();
				});
			pushAndShow(multiChoiceView, e);
		}
	};

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&c64IOControl);
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
		quickSettings.setActive(EmuSystem::gameIsRunning());
		swapJoystickPorts.setBoolValue(optionSwapJoystickPorts);
		warpMode.setBoolValue(*plugin.warp_mode_enabled);
	}
};

class CustomMainMenuView : public EmuMainMenuView
{
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
			auto &multiChoiceView = *new TextTableView{item.t.str, attachParams(), systems};
			iterateTimes(IG::size(systemPresent), i)
			{
				if(!systemPresent[i])
				{
					continue;
				}
				multiChoiceView.appendItem(VicePlugin::systemName((ViceSystem)i),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						optionViceSystem = i;
						popAndShow();
						auto &ynAlertView = *new YesNoAlertView{attachParams(), "Changing systems needs app restart, exit now?"};
						ynAlertView.setOnYes(
							[](TextMenuItem &, View &, Input::Event e)
							{
								Base::exit();
							});
						EmuApp::pushAndShowModalView(ynAlertView, e);
					});
			}
			pushAndShow(multiChoiceView, e);
		}
	};

	FS::FileString newDiskName;

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
						auto &fPicker = *EmuFilePicker::makeForMediaCreation(attachParams(), Input::defaultEvent());
						fPicker.setOnClose(
							[this](FSPicker &picker, Input::Event e)
							{
								auto path = FS::makePathStringPrintf("%s/%s.d64", picker.path().data(), newDiskName.data());
								picker.dismiss();
								if(e.isDefaultCancelButton())
								{
									// picker was cancelled
									EmuApp::unpostMessage();
									return;
								}
								if(FS::exists(path))
								{
									EmuApp::postMessage(true, "File already exists");
									return;
								}
								if(plugin.vdrive_internal_create_format_disk_image(path.data(),
									FS::makeFileStringPrintf("%s,dsk", newDiskName.data()).data(),
									DISK_IMAGE_TYPE_D64) == -1)
								{
									EmuApp::postMessage(true, "Error creating disk image");
									return;
								}
								autostartOnLoad = false;
								EmuApp::createSystemWithMedia({}, path.data(), "", e,
									[this](Input::Event e)
									{
										EmuApp::launchSystemWithResumePrompt(renderer(), e, true);
									});
							});
						view.dismiss();
						EmuApp::pushAndShowModalView(fPicker, Input::defaultEvent());
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

	void reloadItems()
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&startWithBlankDisk);
		string_printf(systemStr, "System: %s", VicePlugin::systemName(currSystem));
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

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new CustomMainMenuView(attach);
		case ViewID::SYSTEM_ACTIONS: return new CustomSystemActionsView(attach);
		case ViewID::VIDEO_OPTIONS: return new CustomVideoOptionView(attach);
		case ViewID::AUDIO_OPTIONS: return new CustomAudioOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new CustomSystemOptionView(attach);
		default: return nullptr;
	}
}

#pragma once
#include <OptionView.hh>

static void setTrueDriveEmu(bool on)
{
	optionDriveTrueEmulation = on;
	if(c64IsInit)
	{
		resources_set_int("DriveTrueEmulation", on);
	}
}

class SystemOptionView : public OptionView
{
	BoolMenuItem trueDriveEmu {"True Drive Emulation (TDE)", BoolMenuItem::SelectDelegate::create<&trueDriveEmuHandler>()};

	static void trueDriveEmuHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		setTrueDriveEmu(item.on);
	}

	BoolMenuItem autostartWarp {"Autostart Fast-forward", BoolMenuItem::SelectDelegate::create<&autostartWarpHandler>()};

	static void autostartWarpHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		resources_set_int("AutostartWarp", item.on);
	}

	BoolMenuItem autostartTDE {"Autostart Handles TDE", BoolMenuItem::SelectDelegate::create<&autostartTDEHandler>()};

	static void autostartTDEHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		resources_set_int("AutostartHandleTrueDriveEmulation", item.on);
	}

	BoolMenuItem cropNormalBorders {"Crop Normal Borders", BoolMenuItem::SelectDelegate::create<&cropNormalBordersHandler>()};

	static void cropNormalBordersHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		optionCropNormalBorders = item.on;
		c64VidActiveX = 0; // force pixmap to update on next frame
	}

	MultiChoiceSelectMenuItem c64Model {"C64 Model", MultiChoiceMenuItem::ValueDelegate::create<&c64ModelSet>()};

	void c64ModelInit()
	{
		static const char *str[] =
		{
			"C64 PAL",
			"C64C PAL",
			"C64 old PAL",
			"C64 NTSC",
			"C64C NTSC",
			"C64 old NTSC",
			"Drean"
		};
		auto model = c64model_get();
		if(model >= (int)sizeofArray(str))
		{
			model = 0;
		}
		c64Model.init(str, model, sizeofArray(str));
	}

	static void c64ModelSet(MultiChoiceMenuItem &, int val)
	{
		setC64Model(val);
	}

	MultiChoiceSelectMenuItem borderMode {"Border Mode", MultiChoiceMenuItem::ValueDelegate::create<&borderModeSet>()};

	void borderModeInit()
	{
		static const char *str[] =
		{
			"Normal",
			"Full",
			"Debug",
			"None"
		};
		auto mode = intResource("VICIIBorderMode");
		if(mode >= (int)sizeofArray(str))
		{
			mode = VICII_NORMAL_BORDERS;
		}
		borderMode.init(str, mode, sizeofArray(str));
	}

	static void borderModeSet(MultiChoiceMenuItem &, int val)
	{
		resources_set_int("VICIIBorderMode", val);
	}

	MultiChoiceSelectMenuItem sidEngine {"SID Engine", MultiChoiceMenuItem::ValueDelegate::create<&sidEngineSet>()};
	static constexpr int sidEngineChoiceMap[]
	{
		SID_ENGINE_FASTSID,
		#if defined(HAVE_RESID)
		SID_ENGINE_RESID,
		#endif
		#if defined(HAVE_RESID_FP)
		SID_ENGINE_RESID_FP,
		#endif
	};

	void sidEngineInit()
	{
		static const char *str[] =
		{
			"FastSID",
			#if defined(HAVE_RESID)
			"ReSID",
			#endif
			#if defined(HAVE_RESID_FP)
			"ReSID-FP"
			#endif
		};
		auto engine = intResource("SidEngine");
		uint idx = 0;
		forEachInArray(sidEngineChoiceMap, e)
		{
			if(*e == engine)
			{
				idx = e_i;
				break;
			}
		}
		sidEngine.init(str, idx, sizeofArray(str));
	}

	static void sidEngineSet(MultiChoiceMenuItem &, int val)
	{
		assert(val <= (int)sizeofArray(sidEngineChoiceMap));
		logMsg("setting SID engine: %d", sidEngineChoiceMap[val]);
		resources_set_int("SidEngine", sidEngineChoiceMap[val]);
	}

public:
	constexpr SystemOptionView() { }

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
		c64ModelInit(); item[items++] = &c64Model;
		trueDriveEmu.init(optionDriveTrueEmulation); item[items++] = &trueDriveEmu;
		autostartTDE.init(intResource("AutostartHandleTrueDriveEmulation")); item[items++] = &autostartTDE;
		autostartWarp.init(intResource("AutostartWarp")); item[items++] = &autostartWarp;
	}
};

constexpr int SystemOptionView::sidEngineChoiceMap[];

static const char *insertEjectMenuStr[] { "Insert File", "Eject" };
static int c64DiskExtensionFsFilter(const char *name, int type);
static int c64TapeExtensionFsFilter(const char *name, int type);
static int c64CartExtensionFsFilter(const char *name, int type);
extern int mem_cartridge_type;

class C64IOControlView : public BaseMenuView
{
private:

	char tapeSlotStr[1024] {0};

	void updateTapeText()
	{
		auto name = tape_get_file_name();
		string_printf(tapeSlotStr, "Tape: %s", name ? string_basename(name) : "");
	}

	void onTapeMediaChange(const char *name)
	{
		updateTapeText();
		tapeSlot.compile();
	}

	void onSelectTapeFile(const char* name, const Input::Event &e)
	{
		if(tape_image_attach(1, name) == 0)
		{
			onTapeMediaChange(name);
		}
		View::removeModalView();
	}

	bool onSelectTapeAction(int action, const Input::Event &e)
	{
		if(action == 0)
		{
			removeModalView();
			fPicker.init(!e.isPointer(), c64TapeExtensionFsFilter, 1);
			fPicker.onSelectFileDelegate().bind<template_mfunc(C64IOControlView, onSelectTapeFile)>(this);
			fPicker.onCloseDelegate().bind<&FSPicker::onCloseModal>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
		else
		{
			tape_image_detach(1);
			onTapeMediaChange("");
			removeModalView();
		}
		return 0;
	}

	void onSelectTape(TextMenuItem &item, const Input::Event &e)
	{
		if(!item.active) return;
		if(tape_get_file_name() && strlen(tape_get_file_name()))
		{
			multiChoiceView.init(insertEjectMenuStr, sizeofArray(insertEjectMenuStr), !e.isPointer());
			multiChoiceView.placeRect(Gfx::viewportRect());
			multiChoiceView.onSelectDelegate().bind<template_mfunc(C64IOControlView, onSelectTapeAction)>(this);
			modalView = &multiChoiceView;
		}
		else
		{
			fPicker.init(!e.isPointer(), c64TapeExtensionFsFilter, 1);
			fPicker.onSelectFileDelegate().bind<template_mfunc(C64IOControlView, onSelectTapeFile)>(this);
			fPicker.onCloseDelegate().bind<&FSPicker::onCloseModal>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
		}
		Base::displayNeedsUpdate();
	}

	TextMenuItem tapeSlot {TextMenuItem::SelectDelegate::create<template_mfunc(C64IOControlView, onSelectTape)>(this)};

	char romSlotStr[1024] {0};

	void updateROMText()
	{
		auto name = cartridge_get_file_name(mem_cartridge_type);
		string_printf(romSlotStr, "ROM: %s", name ? string_basename(name) : "");
	}

	void onROMMediaChange(const char *name)
	{
		updateROMText();
		romSlot.compile();
	}

	void onSelectROMFile(const char* name, const Input::Event &e)
	{
		if(cartridge_attach_image(CARTRIDGE_CRT, name) == 0)
		{
			onROMMediaChange(name);
		}
		View::removeModalView();
	}

	bool onSelectROMAction(int action, const Input::Event &e)
	{
		if(action == 0)
		{
			removeModalView();
			fPicker.init(!e.isPointer(), c64CartExtensionFsFilter, 1);
			fPicker.onSelectFileDelegate().bind<template_mfunc(C64IOControlView, onSelectROMFile)>(this);
			fPicker.onCloseDelegate().bind<&FSPicker::onCloseModal>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
		else if(action == 1)
		{
			cartridge_detach_image(-1);
			onROMMediaChange("");
			removeModalView();
		}
		return 0;
	}

	void onSelectROM(TextMenuItem &, const Input::Event &e)
	{
		if(cartridge_get_file_name(mem_cartridge_type) && strlen(cartridge_get_file_name(mem_cartridge_type)))
		{
			multiChoiceView.init(insertEjectMenuStr, sizeofArray(insertEjectMenuStr), !e.isPointer());
			multiChoiceView.placeRect(Gfx::viewportRect());
			multiChoiceView.onSelectDelegate().bind<template_mfunc(C64IOControlView, onSelectROMAction)>(this);
			modalView = &multiChoiceView;
		}
		else
		{
			fPicker.init(!e.isPointer(), c64CartExtensionFsFilter, 1);
			fPicker.onSelectFileDelegate().bind<template_mfunc(C64IOControlView, onSelectROMFile)>(this);
			fPicker.onCloseDelegate().bind<&FSPicker::onCloseModal>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
		}
		Base::displayNeedsUpdate();
	}

	TextMenuItem romSlot {TextMenuItem::SelectDelegate::create<template_mfunc(C64IOControlView, onSelectROM)>(this)};

	static constexpr const char *diskSlotPrefix[2] {"Disk #8:", "Disk #9:"};
	char diskSlotStr[2][1024] { {0} };

	void updateDiskText(int slot)
	{
		auto name = file_system_get_disk_name(slot+8);
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot], name ? string_basename(name) : "");
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		updateDiskText(slot);
		diskSlot[slot].compile();
	}

	template <int SLOT>
	void onSelectDiskFile(const char* name, const Input::Event &e)
	{
		logMsg("inserting disk in unit %d", SLOT+8);
		if(file_system_attach_disk(SLOT+8, name) == 0)
		{
			onDiskMediaChange(name, SLOT);
		}
		View::removeModalView();
	}

	template <int SLOT>
	bool onSelectDiskAction(int action, const Input::Event &e)
	{
		if(action == 0)
		{
			removeModalView();
			fPicker.init(!e.isPointer(), c64DiskExtensionFsFilter, 1);
			fPicker.onSelectFileDelegate().bind<template_mfunc(C64IOControlView, onSelectDiskFile<SLOT>)>(this);
			fPicker.onCloseDelegate().bind<&FSPicker::onCloseModal>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
		else
		{
			file_system_detach_disk(SLOT+8);
			onDiskMediaChange("", SLOT);
			removeModalView();
		}
		return 0;
	}

	template <int SLOT>
	void onSelectDisk(TextMenuItem &, const Input::Event &e)
	{
		if(file_system_get_disk_name(SLOT+8) && strlen(file_system_get_disk_name(SLOT+8)))
		{
			multiChoiceView.init(insertEjectMenuStr, sizeofArray(insertEjectMenuStr), !e.isPointer());
			multiChoiceView.placeRect(Gfx::viewportRect());
			multiChoiceView.onSelectDelegate().bind<template_mfunc(C64IOControlView, onSelectDiskAction<SLOT>)>(this);
			modalView = &multiChoiceView;
		}
		else
		{
			fPicker.init(!e.isPointer(), c64DiskExtensionFsFilter, 1);
			fPicker.onSelectFileDelegate().bind<template_mfunc(C64IOControlView, onSelectDiskFile<SLOT>)>(this);
			fPicker.onCloseDelegate().bind<&FSPicker::onCloseModal>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
		}
		Base::displayNeedsUpdate();
	}

	TextMenuItem diskSlot[2]
	{
		{TextMenuItem::SelectDelegate::create<template_mfunc(C64IOControlView, onSelectDisk<0>)>(this)},
		{TextMenuItem::SelectDelegate::create<template_mfunc(C64IOControlView, onSelectDisk<1>)>(this)}
	};

	MenuItem *item[9] {nullptr};
public:
	constexpr C64IOControlView(): BaseMenuView("IO Control") { }

	void init(bool highlightFirst)
	{
		uint i = 0;
		updateROMText();
		romSlot.init(romSlotStr); item[i++] = &romSlot;

		iterateTimes(1, slot)
		{
			updateDiskText(slot);
			diskSlot[slot].init(diskSlotStr[slot]); item[i++] = &diskSlot[slot];
		}

		updateTapeText();
		tapeSlot.init(tapeSlotStr); item[i++] = &tapeSlot;
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static C64IOControlView c64IoMenu;

constexpr const char *C64IOControlView::diskSlotPrefix[2];

class SystemMenuView : public MenuView
{
	static void swapJoystickPortsHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		optionSwapJoystickPorts = item.on;
	}

	BoolMenuItem swapJoystickPorts {"Swap Joystick Ports", BoolMenuItem::SelectDelegate::create<&swapJoystickPortsHandler>()};

	static void c64IOControlHandler(TextMenuItem &item, const Input::Event &e)
	{
		if(item.active)
		{
			FsSys::chdir(EmuSystem::gamePath);// Stay in active media's directory
			c64IoMenu.init(!e.isPointer());
			viewStack.pushAndShow(&c64IoMenu);
		}
	}

	TextMenuItem c64IOControl {"ROM/Disk/Tape Control", TextMenuItem::SelectDelegate::create<&c64IOControlHandler>()};

	static bool onSelectQuickSettings(int action, const Input::Event &e)
	{
		removeModalView();
		switch(action)
		{
			bcase 1:
				setTrueDriveEmu(1);
				setC64Model(C64MODEL_C64_NTSC);
			bcase 2:
				setTrueDriveEmu(0);
				setC64Model(C64MODEL_C64_NTSC);
			bcase 3:
				setTrueDriveEmu(1);
				setC64Model(C64MODEL_C64_PAL);
			bcase 4:
				setTrueDriveEmu(0);
				setC64Model(C64MODEL_C64_PAL);
		}
		return 0;
	}

	static void onSetQuickSettings(TextMenuItem &item, const Input::Event &e)
	{
		static const char *str[] =
		{
			"Cancel",
			"1. NTSC & True Drive Emu",
			"2. NTSC",
			"3. PAL & True Drive Emu",
			"4. PAL",
		};
		multiChoiceView.init(str, sizeofArray(str), !e.isPointer(), LC2DO);
		multiChoiceView.placeRect(Gfx::viewportRect());
		multiChoiceView.onSelectDelegate().bind<&onSelectQuickSettings>();
		modalView = &multiChoiceView;
	}

	TextMenuItem quickSettings {"Apply Quick C64 Settings", TextMenuItem::SelectDelegate::create<&onSetQuickSettings>()};

public:
	constexpr SystemMenuView() { }

	void onShow()
	{
		MenuView::onShow();
		c64IOControl.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		c64IOControl.init(); item[items++] = &c64IOControl;
		quickSettings.init(); item[items++] = &quickSettings;
		swapJoystickPorts.init(optionSwapJoystickPorts); item[items++] = &swapJoystickPorts;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};

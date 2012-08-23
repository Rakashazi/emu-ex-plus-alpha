#pragma once
#include "OptionView.hh"
#include <util/cLang.h>

static void setupMDInput();

class CDBIOSFilePicker
{
	static char *path;
public:
	static void onSelectFile(const char* name, const InputEvent &e)
	{
		snprintf(path, sizeof(FsSys::cPath), "%s/%s", FsSys::workDir(), name);
		logMsg("set bios %s", path);
		View::removeModalView();
	}

	static void onClose(const InputEvent &e)
	{
		View::removeModalView();
	}

	static void init(bool highlightFirst, uint region)
	{
		switch(region)
		{
			bdefault: path = cdBiosUSAPath;
			bcase REGION_JAPAN_NTSC: path = optionCDBiosJpnPath;
			bcase REGION_EUROPE: path = optionCDBiosEurPath;
		}
		fPicker.init(highlightFirst, mdROMFsFilter);
		fPicker.onSelectFileDelegate().bind<&CDBIOSFilePicker::onSelectFile>();
		fPicker.onCloseDelegate().bind<&CDBIOSFilePicker::onClose>();
	}
};

char *CDBIOSFilePicker::path = cdBiosUSAPath;

class MdOptionView : public OptionView
{
private:

	BoolMenuItem sixButtonPad, multitap, smsFM, bigEndianSram;

	static void sixButtonPadHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		option6BtnPad = item.on;
		setupMDInput();
		vController.place();
	}

	static void multitapHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		usingMultiTap = item.on;
		setupMDInput();
	}

	static void smsFMHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionSmsFM = item.on;
		config_ym2413_enabled = optionSmsFM;
	}

	void confirmBigEndianSramAlert(const InputEvent &e)
	{
		bigEndianSram.toggle();
		optionBigEndianSram = bigEndianSram.on;
	}

	void bigEndianSramHandler(BoolMenuItem &item, const InputEvent &e)
	{
		ynAlertView.init("Warning, this changes the format of SRAM saves files. "
				"Turn on to make them compatible with other emulators like Gens. "
				"Any SRAM loaded with the incorrect setting will be corrupted.", !e.isPointer());
		ynAlertView.onYesDelegate().bind<MdOptionView, &MdOptionView::confirmBigEndianSramAlert>(this);
		ynAlertView.place(Gfx::viewportRect());
		modalView = &ynAlertView;
	}

	MultiChoiceSelectMenuItem region;

	void regionInit()
	{
		static const char *str[] =
		{
			"Auto", "USA", "Europe", "Japan"
		};
		int setting = 0;
		if(config.region_detect < 4)
		{
			setting = config.region_detect;
		}

		region.init("Game Region", str, setting, sizeofArray(str));
		region.valueDelegate().bind<&regionSet>();
	}

	static void regionSet(MultiChoiceMenuItem &, int val)
	{
		config.region_detect = val;
	}

	struct CdBiosPathMenuItem : public TextMenuItem
	{
		uint region;
		void init(const char *name, uint region) { TextMenuItem::init(name); var_selfs(region); }

		void select(View *view, const InputEvent &e)
		{
			CDBIOSFilePicker::init(!e.isPointer(), region);
			fPicker.place(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
	} cdBiosPath[3];

	MultiChoiceSelectMenuItem inputPorts;

	void inputPortsInit()
	{
		static const char *str[] =
		{
			"Auto", "Gamepads", "Menacer", "Justifier"
		};
		int setting = 0;
		if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_MD_GAMEPAD)
			setting = 1;
		else if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_MENACER)
			setting = 2;
		else if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_JUSTIFIER)
			setting = 3;

		inputPorts.init("Input Ports", str, setting, sizeofArray(str));
		inputPorts.valueDelegate().bind<&inputPortsSet>();
	}

	static void inputPortsSet(MultiChoiceMenuItem &, int val)
	{
		if(val == 0)
		{
			mdInputPortDev[0] = mdInputPortDev[1] = -1;
		}
		else if(val == 1)
		{
			mdInputPortDev[0] = mdInputPortDev[1] = SYSTEM_MD_GAMEPAD;
		}
		else if(val == 2)
		{
			mdInputPortDev[0] = SYSTEM_MD_GAMEPAD; mdInputPortDev[1] = SYSTEM_MENACER;
		}
		else if(val == 3)
		{
			mdInputPortDev[0] = SYSTEM_MD_GAMEPAD; mdInputPortDev[1] = SYSTEM_JUSTIFIER;
		}

		setupMDInput();
	}

	MenuItem *item[24];

public:

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		OptionView::loadAudioItems(item, items);
		smsFM.init("MarkIII FM Sound Unit", optionSmsFM); item[items++] = &smsFM;
		smsFM.selectDelegate().bind<&smsFMHandler>();
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		sixButtonPad.init("6-button Gamepad", option6BtnPad); item[items++] = &sixButtonPad;
		sixButtonPad.selectDelegate().bind<&sixButtonPadHandler>();
		multitap.init("4-Player Adapter", usingMultiTap); item[items++] = &multitap;
		multitap.selectDelegate().bind<&multitapHandler>();
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		bigEndianSram.init("Use Big-Endian SRAM", optionBigEndianSram); item[items++] = &bigEndianSram;
		bigEndianSram.selectDelegate().bind<MdOptionView, &MdOptionView::bigEndianSramHandler>(this);
		regionInit(); item[items++] = &region;
		cdBiosPath[0].init("Select USA CD BIOS", REGION_USA); item[items++] = &cdBiosPath[0];
		cdBiosPath[1].init("Select Japan CD BIOS", REGION_JAPAN_NTSC); item[items++] = &cdBiosPath[1];
		cdBiosPath[2].init("Select Europe CD BIOS", REGION_EUROPE); item[items++] = &cdBiosPath[2];
	}

	void init(uint idx, bool highlightFirst)
	{
		uint i = 0;
		switch(idx)
		{
			bcase 0: loadVideoItems(item, i);
			bcase 1: loadAudioItems(item, i);
			bcase 2: loadInputItems(item, i);
			bcase 3: loadSystemItems(item, i);
			bcase 4: loadGUIItems(item, i);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

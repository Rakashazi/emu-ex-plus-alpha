#pragma once
#include "OptionView.hh"
#include <util/cLang.h>
#include <libgen.h>

static void setupMDInput();

class SystemOptionView : public OptionView
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
		ynAlertView.onYesDelegate().bind<SystemOptionView, &SystemOptionView::confirmBigEndianSramAlert>(this);
		ynAlertView.placeRect(Gfx::viewportRect());
		modalView = &ynAlertView;
	}

	MultiChoiceSelectMenuItem region {"Game Region"};

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

		region.init(str, setting, sizeofArray(str));
		region.valueDelegate().bind<&regionSet>();
	}

	static void regionSet(MultiChoiceMenuItem &, int val)
	{
		optionRegion = val;
		config.region_detect = val;
	}

	static int regionCodeToIdx(int region)
	{
		switch(region)
		{
			default: return 0;
			case REGION_JAPAN_NTSC: return 1;
			case REGION_EUROPE: return 2;
		}
	}

	static FsSys::cPath &regionCodeToStrBuffer(int region)
	{
		switch(region)
		{
			default: return cdBiosUSAPath;
			case REGION_JAPAN_NTSC: return cdBiosJpnPath;
			case REGION_EUROPE: return cdBiosEurPath;
		}
	}

	BiosSelectMenu biosSelectMenu;
	char cdBiosPathStr[3][256] { {0} };
	TextMenuItem cdBiosPath[3];

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S], int region)
	{
		const char *path = "";
		switch(region)
		{
			bdefault: path = cdBiosUSAPath;
			bcase REGION_JAPAN_NTSC: path = cdBiosJpnPath;
			bcase REGION_EUROPE: path = cdBiosEurPath;
		}
		const char *regionStr = "";
		switch(region)
		{
			bdefault: regionStr = "USA";
			bcase REGION_JAPAN_NTSC: regionStr = "Japan";
			bcase REGION_EUROPE: regionStr = "Europe";
		}
		char basenameStr[S];
		strcpy(basenameStr, path);
		string_printf(str, "%s CD BIOS: %s", regionStr, strlen(path) ? basename(basenameStr) : "None set");
	}

	template <int region>
	void cdBiosPathUpdated()
	{
		auto idx = regionCodeToIdx(region);
		logMsg("set bios at idx %d to %s", idx, regionCodeToStrBuffer(region));
		printBiosMenuEntryStr(cdBiosPathStr[idx], region);
		cdBiosPath[idx].compile();
	}

	template <int region>
	void cdBiosPathHandler(TextMenuItem &item, const InputEvent &e)
	{
		biosSelectMenu.init(&regionCodeToStrBuffer(region), mdROMFsFilter, !e.isPointer());
		biosSelectMenu.placeRect(Gfx::viewportRect());
		biosSelectMenu.biosChangeDel.bind<SystemOptionView, &SystemOptionView::cdBiosPathUpdated<region>>(this);
		modalView = &biosSelectMenu;
		Base::displayNeedsUpdate();
	}

	void cdBiosPathInit(MenuItem *item[], uint &items)
	{
		const int region[3] = { REGION_USA, REGION_JAPAN_NTSC, REGION_EUROPE };
		iterateTimes(3, i)
		{
			printBiosMenuEntryStr(cdBiosPathStr[i], region[i]);
			cdBiosPath[i].init(cdBiosPathStr[i]); item[items++] = &cdBiosPath[i];
		}

		cdBiosPath[0].selectDelegate().bind<SystemOptionView, &SystemOptionView::cdBiosPathHandler<REGION_USA>>(this);
		cdBiosPath[1].selectDelegate().bind<SystemOptionView, &SystemOptionView::cdBiosPathHandler<REGION_JAPAN_NTSC>>(this);
		cdBiosPath[2].selectDelegate().bind<SystemOptionView, &SystemOptionView::cdBiosPathHandler<REGION_EUROPE>>(this);
	}

	MultiChoiceSelectMenuItem inputPorts {"Input Ports"};

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

		inputPorts.init(str, setting, sizeofArray(str));
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

public:
	constexpr SystemOptionView() { }

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
		bigEndianSram.selectDelegate().bind<SystemOptionView, &SystemOptionView::bigEndianSramHandler>(this);
		regionInit(); item[items++] = &region;
		cdBiosPathInit(item, items);
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

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};

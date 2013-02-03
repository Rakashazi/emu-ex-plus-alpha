#pragma once
#include "OptionView.hh"
#include <util/cLang.h>
#include <libgen.h>

static void setupMDInput();

class SystemOptionView : public OptionView
{
private:

	BoolMenuItem sixButtonPad, multitap, smsFM, bigEndianSram;

	static void sixButtonPadHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		option6BtnPad = item.on;
		setupMDInput();
		vController.place();
	}

	static void multitapHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		usingMultiTap = item.on;
		setupMDInput();
	}

	static void smsFMHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		optionSmsFM = item.on;
		config_ym2413_enabled = optionSmsFM;
	}

	void confirmBigEndianSramAlert(const Input::Event &e)
	{
		bigEndianSram.toggle();
		optionBigEndianSram = bigEndianSram.on;
	}

	void bigEndianSramHandler(BoolMenuItem &item, const Input::Event &e)
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

	#ifndef NO_SCD
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
	void cdBiosPathHandler(TextMenuItem &item, const Input::Event &e)
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
	#endif

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
		#ifndef NO_SCD
		cdBiosPathInit(item, items);
		#endif
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
#include <TextEntry.hh>

// TODO: refactor/merge cheat view code with GBC.emu & NES.emu

static void refreshCheatViews();

class EditCheatView : public BaseMenuView
{
private:
	TextMenuItem name;
	DualTextMenuItem ggCode {"Code"};
	TextMenuItem remove {"Delete Cheat"};
	MdCheat *cheat = nullptr;
	MenuItem *item[3] = {nullptr};

	static bool strIs16BitGGCode(const char *str)
	{
		return strlen(str) == 9 && str[4] == '-';
	}

	static bool strIs8BitGGCode(const char *str)
	{
		return strlen(str) == 11 && str[3] == '-' && str[7] == '-';
	}

	static bool strIs16BitARCode(const char *str)
	{
		return strlen(str) == 11 && str[6] == ':';
	}

	static bool strIs8BitARCode(const char *str)
	{
		return strlen(str) == 9 && str[6] == ':';
	}

	static bool strIs8BitCode(const char *str)
	{
		return strIs8BitGGCode(str) || strIs8BitARCode(str);
	}

	static bool strIs16BitCode(const char *str)
	{
		return strIs16BitGGCode(str) || strIs16BitARCode(str);
	}

	uint handleGgCodeFromTextInput(const char *str)
	{
		if(str)
		{
			string_copy(cheat->code, str);
			string_toUpper(cheat->code);
			if(!decodeCheat(cheat->code, cheat->address, cheat->data, cheat->origData))
			{
				cheat->code[0]= 0;
				popup.postError("Invalid code");
				Base::displayNeedsUpdate();
				return 1;
			}

			cheatsModified = 1;
			updateCheats();
			ggCode.compile();
			Base::displayNeedsUpdate();
		}
		removeModalView();
		return 0;
	}

	void ggCodeHandler(TextMenuItem &item, const Input::Event &e)
	{
		static const char *inputCode8BitStr = "Input xxx-xxx-xxx (GG) or xxxxxx:xx (AR) code";
		static const char *inputCode16BitStr = "Input xxxx-xxxx (GG) or xxxxxx:xxxx (AR) code";
		textInputView.init(emuSystemIs16Bit() ? inputCode16BitStr : inputCode8BitStr, cheat->code);
		textInputView.onTextDelegate().bind<EditCheatView, &EditCheatView::handleGgCodeFromTextInput>(this);
		textInputView.placeRect(Gfx::viewportRect());
		modalView = &textInputView;
	}

	uint handleNameFromTextInput(const char *str)
	{
		if(str)
		{
			logMsg("setting cheat name %s", str);
			string_copy(cheat->name, str);
			cheatsModified = 1;
			name.compile();
			Base::displayNeedsUpdate();
		}
		removeModalView();
		return 0;
	}

	void nameHandler(TextMenuItem &item, const Input::Event &e)
	{
		textInputView.init("Input description", cheat->name);
		textInputView.onTextDelegate().bind<EditCheatView, &EditCheatView::handleNameFromTextInput>(this);
		textInputView.placeRect(Gfx::viewportRect());
		modalView = &textInputView;
	}

	void removeHandler(TextMenuItem &item, const Input::Event &e)
	{
		cheatList.remove(*cheat);
		cheatsModified = 1;
		refreshCheatViews();
		updateCheats();
		viewStack.popAndShow();
	}

public:
	constexpr EditCheatView(): BaseMenuView("")	{ }

	void init(bool highlightFirst, MdCheat &cheat)
	{
		this->cheat = &cheat;

		uint i = 0;
		name.init(cheat.name); item[i++] = &name;
		name.selectDelegate().bind<EditCheatView, &EditCheatView::nameHandler>(this);

		name_ = "Edit Code";
		ggCode.init(cheat.code); item[i++] = &ggCode;
		ggCode.selectDelegate().bind<EditCheatView, &EditCheatView::ggCodeHandler>(this);

		remove.init(); item[i++] = &remove;
		remove.selectDelegate().bind<EditCheatView, &EditCheatView::removeHandler>(this);
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static EditCheatView editCheatView;

class EditCheatListView : public BaseMenuView
{
private:
	TextMenuItem addGGGS;
	TextMenuItem cheat[maxCheats];
	MenuItem *item[maxCheats + 1] = {nullptr};

	uint handleNameFromTextInput(const char *str)
	{
		if(str)
		{
			MdCheat c;
			string_copy(c.name, str);
			if(!cheatList.addToEnd(c))
			{
				logErr("error adding new cheat");
				removeModalView();
				return 0;
			}
			logMsg("added new cheat, %d total", cheatList.size);
			cheatsModified = 1;
			removeModalView();
			refreshCheatViews();
			editCheatView.init(0, *cheatList.last());
			viewStack.pushAndShow(&editCheatView);
		}
		else
		{
			removeModalView();
		}
		return 0;
	}

	void addGGGSHandler(TextMenuItem &item, const Input::Event &e)
	{
		textInputView.init("Input description");
		textInputView.onTextDelegate().bind<EditCheatListView, &EditCheatListView::handleNameFromTextInput>(this);
		textInputView.placeRect(Gfx::viewportRect());
		modalView = &textInputView;
	}

public:
	constexpr EditCheatListView(): BaseMenuView("Edit Cheats") { }

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		if(i < 1)
			item[i]->select(this, e);
		else
		{
			editCheatView.init(!e.isPointer(), *cheatList.index(i - 1));
			viewStack.pushAndShow(&editCheatView);
		}
	}

	void init(bool highlightFirst)
	{
		uint i = 0;
		addGGGS.init("Add Game Genie / Action Replay Code"); item[i++] = &addGGGS;
		addGGGS.selectDelegate().bind<EditCheatListView, &EditCheatListView::addGGGSHandler>(this);
		int cheats = IG::min(cheatList.size, (int)sizeofArray(cheat));
		auto it = cheatList.iterator();
		iterateTimes(cheats, c)
		{
			auto &thisCheat = it.obj();
			cheat[c].init(thisCheat.name); item[i++] = &cheat[c];
			it.advance();
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static EditCheatListView editCheatListView;

#include <Cheats.hh>

class CheatsView : public BaseCheatsView
{
private:
	struct CheatMenuItem : public BoolMenuItem
	{
		constexpr CheatMenuItem() { }
		MdCheat *cheat = nullptr;
		void init(MdCheat &cheat)
		{
			BoolMenuItem::init(cheat.name, cheat.isOn(), true, View::defaultFace);
			this->cheat = &cheat;
			logMsg("added cheat %s : %s", cheat.name, cheat.code);
		}

		void select(View *view, const Input::Event &e)
		{
			toggle();
			cheat->toggleOn();
			cheatsModified = 1;
			updateCheats();
		}
	} cheat[maxCheats];

public:
	constexpr CheatsView() { }
	void loadCheatItems(MenuItem *item[], uint &i)
	{
		int cheats = IG::min(cheatList.size, (int)sizeofArray(cheat));
		auto it = cheatList.iterator();
		iterateTimes(cheats, c)
		{
			auto &thisCheat = it.obj();
			cheat[c].init(thisCheat); item[i++] = &cheat[c];
			it.advance();
		}
	}
};

static CheatsView cheatsMenu;

static void refreshCheatViews()
{
	editCheatListView.deinit();
	editCheatListView.init(0);
	cheatsMenu.deinit();
	cheatsMenu.init(0);
}

class SystemMenuView : public MenuView
{
	static void cheatsHandler(TextMenuItem &item, const Input::Event &e)
	{
		if(EmuSystem::gameIsRunning())
		{
			cheatsMenu.init(!e.isPointer());
			viewStack.pushAndShow(&cheatsMenu);
		}
	}

	TextMenuItem cheats {"Cheats", TextMenuItem::SelectDelegate::create<&cheatsHandler>()};

public:
	constexpr SystemMenuView() { }

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};

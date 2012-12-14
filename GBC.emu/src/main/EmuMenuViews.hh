#pragma once
#include "OptionView.hh"

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem gbPalette {"GB Palette"};

	void gbPaletteInit()
	{
		static const char *str[] =
		{
			"Original", "Brown", "Red", "Dark Brown", "Pastel", "Orange", "Yellow", "Blue", "Dark Blue", "Gray", "Green", "Dark Green", "Reverse"
		};
		gbPalette.init(str, int(optionGBPal), sizeofArray(str));
		gbPalette.valueDelegate().bind<&gbPaletteSet>();
	}

	static void gbPaletteSet(MultiChoiceMenuItem &, int val)
	{
		optionGBPal.val = val;
		applyGBPalette(val);
	}

	MultiChoiceSelectMenuItem resampler {"Resampler"};
	const char *resamplerName[4] {nullptr};
	void resamplerInit()
	{
		logMsg("%d resamplers", (int)ResamplerInfo::num());
		auto resamplers = IG::min(ResamplerInfo::num(), sizeofArray(resamplerName));
		iterateTimes(resamplers, i)
		{
			ResamplerInfo r = ResamplerInfo::get(i);
			logMsg("%d %s", i, r.desc);
			resamplerName[i] = r.desc;
		}
		resampler.init(resamplerName, int(optionAudioResampler), resamplers);
		resampler.valueDelegate().bind<&resamplerSet>();
	}

	static void resamplerSet(MultiChoiceMenuItem &, int val)
	{
		optionAudioResampler = val;
		EmuSystem::configAudioRate();
	}


	BoolMenuItem reportAsGba {"Report Hardware as GBA", BoolMenuItem::SelectDelegate::create<&reportAsGbaHandler>()};

	static void reportAsGbaHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionReportAsGba = item.on;
	}

	BoolMenuItem fullSaturation {"Saturated GBC Colors", BoolMenuItem::SelectDelegate::create<&fullSaturationHandler>()};

	static void fullSaturationHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionFullGbcSaturation = item.on;
		if(EmuSystem::gameIsRunning())
		{
			gbEmu.refreshPalettes();
		}
	}

public:
	constexpr SystemOptionView() { }

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		OptionView::loadAudioItems(item, items);
		resamplerInit(); item[items++] = &resampler;
	}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		gbPaletteInit(); item[items++] = &gbPalette;
		fullSaturation.init(optionFullGbcSaturation); item[items++] = &fullSaturation;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		reportAsGba.init(optionReportAsGba); item[items++] = &reportAsGba;
	}
};

#include "MenuView.hh"
#include <TextEntry.hh>

static const uint maxCheats = 255;
static CollectTextInputView cheatInputView;
static void refreshCheatViews();

class EditCheatView : public BaseMenuView
{
private:
	TextMenuItem name;
	DualTextMenuItem ggCode {"Code"};
	TextMenuItem remove {"Delete Cheat"};
	GbcCheat *cheat = nullptr;
	MenuItem *item[3] = {nullptr};

	static bool strIsGGCode(const char *str)
	{
		return strlen(str) == 11 && str[3] == '-' && str[7] == '-' &&
			string_isHexValue(&str[0], 3) &&
			string_isHexValue(&str[4], 3) &&
			string_isHexValue(&str[8], 3);
	}

	static bool strIsGSCode(const char *str)
	{
		return strlen(str) == 8 && string_isHexValue(str, 8);
	}

	uint handleGgCodeFromTextInput(const char *str)
	{
		if(str)
		{
			if(!strIsGGCode(str) && !strIsGSCode(str))
			{
				popup.postError("Invalid format");
				Base::displayNeedsUpdate();
				return 1;
			}
			string_copy(cheat->code, str);
			string_toUpper(cheat->code);
			cheatsModified = 1;
			applyCheats();
			ggCode.compile();
			Base::displayNeedsUpdate();
		}
		removeModalView();
		return 0;
	}

	void ggCodeHandler(TextMenuItem &item, const InputEvent &e)
	{
		cheatInputView.init("Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", cheat->code);
		cheatInputView.onTextDelegate().bind<EditCheatView, &EditCheatView::handleGgCodeFromTextInput>(this);
		cheatInputView.placeRect(Gfx::viewportRect());
		modalView = &cheatInputView;
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

	void nameHandler(TextMenuItem &item, const InputEvent &e)
	{
		cheatInputView.init("Input description", cheat->name);
		cheatInputView.onTextDelegate().bind<EditCheatView, &EditCheatView::handleNameFromTextInput>(this);
		cheatInputView.placeRect(Gfx::viewportRect());
		modalView = &cheatInputView;
	}

	void removeHandler(TextMenuItem &item, const InputEvent &e)
	{
		cheatList.remove(*cheat);
		cheatsModified = 1;
		refreshCheatViews();
		applyCheats();
		viewStack.popAndShow();
	}

public:
	constexpr EditCheatView(): BaseMenuView("")	{ }

	void init(bool highlightFirst, GbcCheat &cheat)
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
			GbcCheat c;
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

	void addGGGSHandler(TextMenuItem &item, const InputEvent &e)
	{
		cheatInputView.init("Input description");
		cheatInputView.onTextDelegate().bind<EditCheatListView, &EditCheatListView::handleNameFromTextInput>(this);
		cheatInputView.placeRect(Gfx::viewportRect());
		modalView = &cheatInputView;
	}

public:
	constexpr EditCheatListView(): BaseMenuView("Edit Cheats") { }

	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
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
		addGGGS.init("Add Game Genie / GameShark Code"); item[i++] = &addGGGS;
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
		GbcCheat *cheat = nullptr;
		void init(GbcCheat &cheat)
		{
			BoolMenuItem::init(cheat.name, cheat.isOn(), true, View::defaultFace);
			this->cheat = &cheat;
			logMsg("added cheat %s : %s", cheat.name, cheat.code);
		}

		void select(View *view, const InputEvent &e)
		{
			toggle();
			cheat->toggleOn();
			cheatsModified = 1;
			applyCheats();
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
	static void cheatsHandler(TextMenuItem &item, const InputEvent &e)
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

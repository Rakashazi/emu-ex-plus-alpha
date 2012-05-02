#pragma once

#include "MenuView.hh"

//static void initFDSControlMenu();
//static void initCheatsMenu();

class CheatsView : public BaseMenuView
{
private:
	#if CONFIG_USE_IN_TABLE_NAV
	MenuView::BackMenuItem back;
	#endif

	static const uint maxCheats = 254;
	struct CheatMenuItem : public BoolMenuItem
	{
		constexpr CheatMenuItem(): idx(0) { }
		int idx;
		void init(int idx, const char *name, bool on) { BoolMenuItem::init(name, on); var_selfs(idx); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			FCEUI_ToggleCheat(idx);
		}
	} cheat[maxCheats];

	MenuItem *item[maxCheats + 1];
public:
	constexpr CheatsView(): BaseMenuView("Cheats")
	#ifdef CONFIG_CXX11
	, item CXX11_INIT_LIST({0})
	#endif
	{ }
	void init(bool highlightFirst)
	{
		uint i = 0;
		#if CONFIG_USE_IN_TABLE_NAV
		back.init(); item[i++] = &back;
		#endif
		int cheats = IG::min(fceuCheats, (uint)sizeofArray(cheat));
		iterateTimes(cheats, c)
		{
			char *name;
			int status;
			int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, &status, 0);
			assert(gotCheat);
			if(!gotCheat) continue;
			cheat[c].init(c, name, status); item[i++] = &cheat[c];
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}

	/*void inputEvent(const InputEvent &e)
	{
		if(e.state == INPUT_PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				dismiss(initMainMenu);
				return;
			}

			if(isMenuDismissKey(e))
			{
				if(EmuSystem::gameIsRunning())
				{
					dismiss(startGameFromMenu);
					return;
				}
			}
		}

		BaseMenuView::inputEvent(e);
	}*/
};

class FDSControlView : public BaseMenuView
{
private:
	#if CONFIG_USE_IN_TABLE_NAV
	MenuView::BackMenuItem back;
	#endif

	struct SetSideMenuItem : public TextMenuItem
	{
		constexpr SetSideMenuItem(): side(0) { }
		int side;
		void init(const char *sideStr, int side)
		{
			TextMenuItem::init(sideStr, side < FCEU_FDSSides());
			this->side = side;
		}

		void select(View *view, const InputEvent &e)
		{
			if(side < FCEU_FDSSides())
			{
				FCEU_FDSSetDisk(side);
				//view->dismiss(initMainMenu);
				viewStack.popAndShow();
			}
		}
	} setSide[4];

	struct InsertEjectMenuItem : public TextMenuItem
	{
		constexpr InsertEjectMenuItem() { }
		void init()
		{
			TextMenuItem::init("Eject", FCEU_FDSInserted());
		}

		void select(View *view, const InputEvent &e)
		{
			if(FCEU_FDSInserted())
			{
				FCEU_FDSInsert();
				//view->dismiss(initMainMenu);
				viewStack.popAndShow();
			}
		}
	} insertEject;

	MenuItem *item[6]; //sizeofArrayConst(setSide) + 2 not accepted by older GCC
public:
	constexpr FDSControlView(): BaseMenuView("FDS Control")
	#ifdef CONFIG_CXX11
	, item CXX11_INIT_LIST({0})
	#endif
	{ }

	void init(bool highlightFirst)
	{
		uint i = 0;
		#if CONFIG_USE_IN_TABLE_NAV
		back.init(); item[i++] = &back;
		#endif
		setSide[0].init("Set Disk 1 Side A", 0); item[i++] = &setSide[0];
		setSide[1].init("Set Disk 1 Side B", 1); item[i++] = &setSide[1];
		setSide[2].init("Set Disk 2 Side A", 2); item[i++] = &setSide[2];
		setSide[3].init("Set Disk 2 Side B", 3); item[i++] = &setSide[3];
		insertEject.init(); item[i++] = &insertEject;
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}

	/*void inputEvent(const InputEvent &e)
	{
		if(e.state == INPUT_PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				dismiss(initMainMenu);
				return;
			}

			if(isMenuDismissKey(e))
			{
				if(EmuSystem::gameIsRunning())
				{
					dismiss(startGameFromMenu);
					return;
				}
			}
		}

		BaseMenuView::inputEvent(e);
	}*/
};

static FDSControlView fdsMenu;
static CheatsView cheatsMenu;

class NesMenuView : public MenuView
{
private:
	struct FDSControlMenuItem : public TextMenuItem
	{
		char label[sizeof("FDS Control (Disk 1:A)")];
		void init()
		{
			strcpy(label, "");
			/*if(!isFDS)
				strcpy(label, "FDS Control");
			else if(!FCEU_FDSInserted())
				strcpy(label, "FDS Control (No Disk)");
			else
				sprintf(label, "FDS Control (Disk %d:%c)", (FCEU_FDSCurrentSide()>>1)+1, (FCEU_FDSCurrentSide() & 1)? 'B' : 'A');*/
			TextMenuItem::init(label);
		}

		void refreshActive()
		{
			active = isFDS;
			if(!isFDS)
				strcpy(label, "FDS Control");
			else if(!FCEU_FDSInserted())
				strcpy(label, "FDS Control (No Disk)");
			else
				sprintf(label, "FDS Control (Disk %d:%c)", (FCEU_FDSCurrentSide()>>1)+1, (FCEU_FDSCurrentSide() & 1)? 'B' : 'A');
			compile();
		}

		void select(View *view, const InputEvent &e)
		{
			if(EmuSystem::gameIsRunning() && isFDS)
			{
				fdsMenu.init(!e.isPointer());
				viewStack.pushAndShow(&fdsMenu);
				//view->dismiss(initFDSControlMenu);
			}
			else
				popup.post("Disk System not in use", 2);
		}
	} fdsControl;

	struct CheatsMenuItem : public TextMenuItem
	{
		void init()
		{
			TextMenuItem::init("Cheats");
		}

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning() && fceuCheats;
		}

		void select(View *view, const InputEvent &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(fceuCheats)
				{
					cheatsMenu.init(!e.isPointer());
					viewStack.pushAndShow(&cheatsMenu);
					//view->dismiss(initCheatsMenu);
				}
				else
					popup.printf(5, 1, "Place a valid .cht file in the same directory as the game named %s.cht", EmuSystem::gameName);
			}
		}
	} cheats;

	MenuItem *item[STANDARD_ITEMS + 2];

public:

	void onShow()
	{
		MenuView::onShow();
		cheats.refreshActive();
		fdsControl.refreshActive();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		fdsControl.init(); item[items++] = &fdsControl;
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};

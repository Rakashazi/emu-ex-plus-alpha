#pragma once
#include "OptionView.hh"

static int pceHuFsFilter(const char *name, int type);

class PceSyscardFilePicker// : public BaseFilePicker
{
public:
	static void onSelectFile(const char* name, const InputEvent &e)
	{
		//char fullPath[strlen(FsSys::workDir()) + 1 + strlen(name) + 1];
		snprintf(sysCardPath, sizeof(sysCardPath), "%s/%s", FsSys::workDir(), name);
		/*if(sysCardPath)
			mem_free(sysCardPath);
		sysCardPath = string_dup(fullPath);*/
		logMsg("set system card %s", sysCardPath);
		removeModalView();
	}

	static void onClose(const InputEvent &e)
	{
		removeModalView();
	}

	/*void inputEvent(const InputEvent &e)
	{
		if(e.state == INPUT_PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				onClose();
				return;
			}

			if(isMenuDismissKey(e))
			{
				if(EmuSystem::gameIsRunning())
				{
					removeModalView();
					startGameFromMenu();
					return;
				}
			}
		}

		FSPicker::inputEvent(e);
	}*/

	static void init(bool highlightFirst)
	{
		fPicker.init(highlightFirst, pceHuFsFilter);
		fPicker.onSelectFileDelegate().bind<&PceSyscardFilePicker::onSelectFile>();
		fPicker.onCloseDelegate().bind<&PceSyscardFilePicker::onClose>();
	}
};

class PceOptionView : public OptionView
{
private:
	struct SysCardPathMenuItem : public TextMenuItem
	{
		//PceSyscardFilePicker picker;
		void init() { TextMenuItem::init("Select System Card"); }

		void select(View *view, const InputEvent &e)
		{
			PceSyscardFilePicker::init(!e.isPointer());
			fPicker.place(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
	} sysCardPath;

	struct ArcadeCardMenuItem : public BoolMenuItem
	{
		void init(bool on) { BoolMenuItem::init("Arcade Card", on); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionArcadeCard = on;
		}
	} arcadeCard;

	struct SixButtonPadMenuItem : public BoolMenuItem
	{
		void init(bool on) { BoolMenuItem::init("6-button support", on); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			PCE_Fast::AVPad6Enabled[0] = on;
			PCE_Fast::AVPad6Enabled[1] = on;
			#ifdef INPUT_SUPPORTS_POINTER
			vController.gp.activeFaceBtns = on ? 6 : 2;
			vController.place();
			#endif
		}
	} sixButtonPad;

	MenuItem *item[24];

public:

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		sixButtonPad.init(PCE_Fast::AVPad6Enabled[0]); item[items++] = &sixButtonPad;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		arcadeCard.init(optionArcadeCard); item[items++] = &arcadeCard;
		sysCardPath.init(); item[items++] = &sysCardPath;
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

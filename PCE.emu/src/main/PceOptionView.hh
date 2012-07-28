#pragma once
#include "OptionView.hh"

static int pceHuFsFilter(const char *name, int type);

class PceSyscardFilePicker
{
public:
	static void onSelectFile(const char* name, const InputEvent &e)
	{
		snprintf(sysCardPath, sizeof(sysCardPath), "%s/%s", FsSys::workDir(), name);
		logMsg("set system card %s", sysCardPath);
		removeModalView();
	}

	static void onClose(const InputEvent &e)
	{
		removeModalView();
	}

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
		void init() { TextMenuItem::init("Select System Card"); }

		void select(View *view, const InputEvent &e)
		{
			PceSyscardFilePicker::init(!e.isPointer());
			fPicker.place(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
	} sysCardPath;

	BoolMenuItem arcadeCard, sixButtonPad;

	static void arcadeCardHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionArcadeCard = item.on;
	}

	static void sixButtonPadHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		PCE_Fast::AVPad6Enabled[0] = item.on;
		PCE_Fast::AVPad6Enabled[1] = item.on;
		#ifdef INPUT_SUPPORTS_POINTER
		vController.gp.activeFaceBtns = item.on ? 6 : 2;
		vController.place();
		#endif
	}

	MenuItem *item[24];

public:

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		sixButtonPad.init("6-button support", PCE_Fast::AVPad6Enabled[0]); item[items++] = &sixButtonPad;
		sixButtonPad.selectDelegate().bind<&sixButtonPadHandler>();
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		arcadeCard.init("Arcade Card", optionArcadeCard); item[items++] = &arcadeCard;
		arcadeCard.selectDelegate().bind<&arcadeCardHandler>();
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

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
		View::removeModalView();
	}

	static void onClose(const InputEvent &e)
	{
		View::removeModalView();
	}

	static void init(bool highlightFirst)
	{
		fPicker.init(highlightFirst, pceHuFsFilter);
		fPicker.onSelectFileDelegate().bind<&PceSyscardFilePicker::onSelectFile>();
		fPicker.onCloseDelegate().bind<&PceSyscardFilePicker::onClose>();
	}
};

class SystemOptionView : public OptionView
{
private:
	struct SysCardPathMenuItem : public TextMenuItem
	{
		constexpr SysCardPathMenuItem() { }
		void init() { TextMenuItem::init("Select System Card"); }

		void select(View *view, const InputEvent &e)
		{
			PceSyscardFilePicker::init(!e.isPointer());
			fPicker.placeRect(Gfx::viewportRect());
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

public:
	constexpr SystemOptionView() { }

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
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};

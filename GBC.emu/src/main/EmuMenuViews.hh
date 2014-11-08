#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem gbPalette
	{
		"GB Palette",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionGBPal.val = val;
			applyGBPalette();
		}
	};

	void gbPaletteInit()
	{
		static const char *str[] =
		{
			"Original", "Brown", "Red", "Dark Brown", "Pastel", "Orange", "Yellow", "Blue", "Dark Blue", "Gray", "Green", "Dark Green", "Reverse"
		};
		gbPalette.init(str, int(optionGBPal), sizeofArray(str));
	}

	BoolMenuItem useBuiltinGBPalette
	{
		"Use Built-in GB Palettes",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionUseBuiltinGBPalette = item.on;
			applyGBPalette();
		}
	};

	MultiChoiceSelectMenuItem resampler
	{
		"Resampler",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionAudioResampler = val;
			EmuSystem::configAudioPlayback();
		}
	};

	const char *resamplerName[4] {nullptr};

	void resamplerInit()
	{
		logMsg("%d resamplers", (int)ResamplerInfo::num());
		auto resamplers = std::min(ResamplerInfo::num(), sizeofArray(resamplerName));
		iterateTimes(resamplers, i)
		{
			ResamplerInfo r = ResamplerInfo::get(i);
			logMsg("%d %s", i, r.desc);
			resamplerName[i] = r.desc;
		}
		resampler.init(resamplerName, int(optionAudioResampler), resamplers);
	}

	BoolMenuItem reportAsGba
	{
		"Report Hardware as GBA",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionReportAsGba = item.on;
		}
	};

	BoolMenuItem fullSaturation
	{
		"Saturated GBC Colors",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionFullGbcSaturation = item.on;
			if(EmuSystem::gameIsRunning())
			{
				gbEmu.refreshPalettes();
			}
		}
	};

public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		OptionView::loadAudioItems(item, items);
		resamplerInit(); item[items++] = &resampler;
	}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		gbPaletteInit(); item[items++] = &gbPalette;
		useBuiltinGBPalette.init(optionUseBuiltinGBPalette); item[items++] = &useBuiltinGBPalette;
		fullSaturation.init(optionFullGbcSaturation); item[items++] = &fullSaturation;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		reportAsGba.init(optionReportAsGba); item[items++] = &reportAsGba;
	}
};

class SystemMenuView : public MenuView
{
	TextMenuItem cheats
	{
		"Cheats",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *new CheatsView{window()};
				cheatsMenu.init(!e.isPointer());
				viewStack.pushAndShow(cheatsMenu);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView{win} {}

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};

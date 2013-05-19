#pragma once
#include "OptionView.hh"

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem gbPalette
	{
		"GB Palette",
		[](MultiChoiceMenuItem &, int val)
		{
			optionGBPal.val = val;
			applyGBPalette(val);
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

	MultiChoiceSelectMenuItem resampler
	{
		"Resampler",
		[](MultiChoiceMenuItem &, int val)
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
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionReportAsGba = item.on;
		}
	};

	BoolMenuItem fullSaturation
	{
		"Saturated GBC Colors",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionFullGbcSaturation = item.on;
			if(EmuSystem::gameIsRunning())
			{
				gbEmu.refreshPalettes();
			}
		}
	};

public:
	SystemOptionView() { }

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

#include "EmuCheatViews.hh"
#include "MenuView.hh"

class SystemMenuView : public MenuView
{
	TextMenuItem cheats
	{
		"Cheats",
		[](TextMenuItem &item, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				cheatsMenu.init(!e.isPointer());
				viewStack.pushAndShow(&cheatsMenu);
			}
		}
	};

public:
	SystemMenuView() { }

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

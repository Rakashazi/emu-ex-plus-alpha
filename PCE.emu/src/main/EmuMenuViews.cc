/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/SystemOptionView.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/VideoOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include <emuframework/SystemActionsView.hh>
#include <emuframework/EmuInput.hh>
#include "MainApp.hh"
#include <imagine/fs/FS.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

constexpr std::string_view pceFastText{"pce_fast (Default for general use)"};
constexpr std::string_view pceText{"pce (Better accuracy, higher power usage)"};
constexpr std::string_view changeEmuCoreText{"Really change emulation core? Note that save states from different cores aren't compatible."};

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	BoolMenuItem sixButtonPad
	{
		"6-button Gamepad", &defaultFace(),
		(bool)system().option6BtnPad,
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			system().sessionOptionSet();
			system().option6BtnPad = item.flipBoolValue(*this);
			set6ButtonPadEnabled(app(), system().option6BtnPad);
		}
	};

	BoolMenuItem arcadeCard
	{
		"Arcade Card", &defaultFace(),
		(bool)system().optionArcadeCard,
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			system().sessionOptionSet();
			system().optionArcadeCard = item.flipBoolValue(*this);
			app().promptSystemReloadDueToSetOption(attachParams(), e);
		}
	};

	TextHeadingMenuItem videoHeading{"Video", &defaultBoldFace()};

	TextMenuItem visibleVideoLinesItem[5]
	{
		{"11+224", &defaultFace(), setVisibleVideoLinesDel(11, 234)},
		{"18+224", &defaultFace(), setVisibleVideoLinesDel(18, 241)},
		{"4+232",  &defaultFace(), setVisibleVideoLinesDel(4, 235)},
		{"3+239",  &defaultFace(), setVisibleVideoLinesDel(3, 241)},
		{"0+242",  &defaultFace(), setVisibleVideoLinesDel(0, 241)},
	};

	MultiChoiceMenuItem visibleVideoLines
	{
		"Visible Lines", &defaultFace(),
		[this]()
		{
			switch(system().visibleLines.first)
			{
				default: return 0;
				case 18: return 1;
				case 4: return 2;
				case 3: return 3;
				case 0: return 4;
			}
		}(),
		visibleVideoLinesItem
	};

	TextMenuItem::SelectDelegate setVisibleVideoLinesDel(uint8_t startLine, uint8_t endLine)
	{
		return [=, this]() { system().setVisibleLines({startLine, endLine}); };
	}

	TextMenuItem emuCoreItems[3]
	{
		{"Auto",      &defaultFace(), setEmuCoreDel(), to_underlying(EmuCore::Auto)},
		{pceFastText, &defaultFace(), setEmuCoreDel(), to_underlying(EmuCore::Fast)},
		{pceText,     &defaultFace(), setEmuCoreDel(), to_underlying(EmuCore::Accurate)},
	};

	MultiChoiceMenuItem emuCore
	{
		"Emulation Core", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(asModuleString(system().resolvedCore()));
				return true;
			}
		},
		(MenuItem::Id)system().core,
		emuCoreItems
	};

	TextMenuItem::SelectDelegate setEmuCoreDel()
	{
		return [this](TextMenuItem &item, const Input::Event &e)
		{
			auto c = EmuCore(item.id());
			if(c == system().core)
				return true;
			pushAndShowModal(makeView<YesNoAlertView>(changeEmuCoreText,
				YesNoAlertView::Delegates
				{
					.onYes = [this, c](const Input::Event &e)
					{
						system().sessionOptionSet();
						system().core = c;
						emuCore.setSelected((MenuItem::Id)c);
						dismissPrevious();
						app().promptSystemReloadDueToSetOption(attachParams(), e);
					}
				}), e, false);
			return false;
		};
	}

	std::array<MenuItem*, 5> menuItem
	{
		&sixButtonPad,
		&arcadeCard,
		&emuCore,
		&videoHeading,
		&visibleVideoLines,
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class CustomSystemActionsView : public SystemActionsView
{
private:
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](Input::Event e) { pushAndShow(makeView<ConsoleOptionView>(), e); }
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): SystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		loadStandardItems();
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper<CustomFilePathOptionView>
{
	using MainAppHelper<CustomFilePathOptionView>::app;
	using MainAppHelper<CustomFilePathOptionView>::system;

	TextMenuItem sysCardPath
	{
		biosMenuEntryStr(system().sysCardPath), &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView>("System Card",
				app().validSearchPath(FS::dirnameUri(system().sysCardPath)),
				[this](CStringView path, FS::file_type type)
				{
					system().sysCardPath = path;
					logMsg("set system card:%s", system().sysCardPath.data());
					sysCardPath.compile(biosMenuEntryStr(path), renderer());
					return true;
				}, hasHuCardExtension), e);
		}
	};

	std::string biosMenuEntryStr(std::string_view path) const
	{
		return fmt::format("System Card: {}", appContext().fileUriDisplayName(path));
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&sysCardPath);
	}
};

class CustomVideoOptionView : public VideoOptionView, public MainAppHelper<CustomVideoOptionView>
{
	using  MainAppHelper<CustomVideoOptionView>::app;
	using  MainAppHelper<CustomVideoOptionView>::system;

	BoolMenuItem spriteLimit
	{
		"Sprite Limit", &defaultFace(),
		!system().noSpriteLimit,
		[this](BoolMenuItem &item) { system().setNoSpriteLimit(!item.flipBoolValue(*this)); }
	};

	TextMenuItem visibleVideoLinesItem[5]
	{
		{"11+224", &defaultFace(), setVisibleVideoLinesDel(11, 234)},
		{"18+224", &defaultFace(), setVisibleVideoLinesDel(18, 241)},
		{"4+232",  &defaultFace(), setVisibleVideoLinesDel(4, 235)},
		{"3+239",  &defaultFace(), setVisibleVideoLinesDel(3, 241)},
		{"0+242",  &defaultFace(), setVisibleVideoLinesDel(0, 241)},
	};

	MultiChoiceMenuItem visibleVideoLines
	{
		"Default Visible Lines", &defaultFace(),
		[this]()
		{
			switch(system().defaultVisibleLines.first)
			{
				default: return 0;
				case 18: return 1;
				case 4: return 2;
				case 3: return 3;
				case 0: return 4;
			}
		}(),
		visibleVideoLinesItem
	};

	TextMenuItem::SelectDelegate setVisibleVideoLinesDel(uint8_t startLine, uint8_t endLine)
	{
		return [=, this]() { system().defaultVisibleLines = {startLine, endLine}; };
	}

	BoolMenuItem correctLineAspect
	{
		"Correct Line Aspect Ratio", &defaultFace(),
		system().correctLineAspect,
		[this](BoolMenuItem &item)
		{
			system().correctLineAspect = item.flipBoolValue(*this);
			app().viewController().placeEmuViews();
		}
	};

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&spriteLimit);
		item.emplace_back(&visibleVideoLines);
		item.emplace_back(&correctLineAspect);
	}
};

class CustomSystemOptionView : public SystemOptionView, public MainAppHelper<CustomSystemOptionView>
{
	using MainAppHelper<CustomSystemOptionView>::system;

	TextMenuItem cdSpeedItem[5]
	{
		{"1x", &defaultFace(), setCdSpeedDel(), 1},
		{"2x", &defaultFace(), setCdSpeedDel(), 2},
		{"4x", &defaultFace(), setCdSpeedDel(), 4},
		{"8x", &defaultFace(), setCdSpeedDel(), 8},
	};

	MultiChoiceMenuItem cdSpeed
	{
		"CD Access Speed", &defaultFace(),
		(MenuItem::Id)system().cdSpeed,
		cdSpeedItem
	};

	TextMenuItem::SelectDelegate setCdSpeedDel()
	{
		return [this](TextMenuItem &item) { system().setCdSpeed(item.id()); };
	}

	TextMenuItem emuCoreItems[3]
	{
		{"Auto",      &defaultFace(), setEmuCoreDel(), to_underlying(EmuCore::Auto)},
		{pceFastText, &defaultFace(), setEmuCoreDel(), to_underlying(EmuCore::Fast)},
		{pceText,     &defaultFace(), setEmuCoreDel(), to_underlying(EmuCore::Accurate)},
	};

	MultiChoiceMenuItem emuCore
	{
		"Emulation Core", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(asModuleString(system().resolvedDefaultCore()));
				return true;
			}
		},
		(MenuItem::Id)system().defaultCore,
		emuCoreItems
	};

	TextMenuItem::SelectDelegate setEmuCoreDel()
	{
		return [this](TextMenuItem &item, const Input::Event &e)
		{
			auto c = EmuCore(item.id());
			if(c == system().defaultCore)
				return true;
			pushAndShowModal(makeView<YesNoAlertView>(changeEmuCoreText,
				YesNoAlertView::Delegates
				{
					.onYes = [this, c]
					{
						system().defaultCore = c;
						emuCore.setSelected((MenuItem::Id)c);
						dismissPrevious();
					}
				}), e, false);
			return false;
		};
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&emuCore);
		item.emplace_back(&cdSpeed);
	}
};

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::system;
	using MainAppHelper<CustomAudioOptionView>::app;

	TextHeadingMenuItem mixer{"Mixer", &defaultBoldFace()};

	struct VolumeTypeDesc
	{
		std::string_view name{};
		size_t idx{};
	};

	static constexpr VolumeTypeDesc desc(VolumeType type)
	{
		switch(type)
		{
			case VolumeType::CDDA: return {"CD-DA Volume", 0};
			case VolumeType::ADPCM: return {"ADPCM Volume", 1};
		}
		bug_unreachable("invalid VolumeType");
	}

	using VolumeChoiceItemArr = std::array<TextMenuItem, 3>;

	VolumeChoiceItemArr volumeLevelChoiceItems(VolumeType type)
	{
		return
		{
			TextMenuItem
			{
				"Default", &defaultFace(),
				[=, this]() { system().setVolume(type, 100); },
				100
			},
			TextMenuItem
			{
				"Off", &defaultFace(),
				[=, this]() { system().setVolume(type, 0); },
				0
			},
			TextMenuItem
			{
				"Custom Value", &defaultFace(),
				[=, this](Input::Event e)
				{
					app().pushAndShowNewCollectValueRangeInputView<int, 0, 200>(attachParams(), e, "Input 0 to 200", "",
						[=, this](EmuApp &, auto val)
						{
							system().setVolume(type, val);
							volumeLevel[desc(type).idx].setSelected((MenuItem::Id)val, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, MenuItem::DEFAULT_ID
			}
		};
	}

	std::array<VolumeChoiceItemArr, 2> volumeLevelItem
	{
		volumeLevelChoiceItems(VolumeType::CDDA),
		volumeLevelChoiceItems(VolumeType::ADPCM),
	};

	MultiChoiceMenuItem volumeLevelMenuItem(VolumeType type)
	{
		return
		{
			desc(type).name, &defaultFace(),
			{
				.onSetDisplayString = [this, type](auto idx, Gfx::Text &t)
				{
					t.resetString(fmt::format("{}%", system().volume(type)));
					return true;
				}
			},
			(MenuItem::Id)system().volume(type),
			volumeLevelItem[desc(type).idx]
		};
	}

	std::array<MultiChoiceMenuItem, 2> volumeLevel
	{
		volumeLevelMenuItem(VolumeType::CDDA),
		volumeLevelMenuItem(VolumeType::ADPCM),
	};

	BoolMenuItem adpcmFilter
	{
		"ADPCM Low-pass Filter", &defaultFace(),
		system().adpcmFilter,
		[this](BoolMenuItem &item) { system().setAdpcmFilter(item.flipBoolValue(*this)); }
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&adpcmFilter);
		item.emplace_back(&mixer);
		item.emplace_back(&volumeLevel[0]);
		item.emplace_back(&volumeLevel[1]);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<CustomVideoOptionView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		default: return nullptr;
	}
}

}

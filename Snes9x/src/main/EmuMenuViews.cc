#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <ppu.h>
#endif
#include <emuframework/EmuApp.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include <emuframework/UserPathSelectView.hh>
#include <emuframework/SystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "MainApp.hh"
#include <snes9x.h>
#include <imagine/util/format.hh>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

constexpr bool HAS_NSRT = !IS_SNES9X_VERSION_1_4;

#ifndef SNES9X_VERSION_1_4
class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::system;

	void setDSPInterpolation(uint8_t val)
	{
		logMsg("set DSP interpolation:%u", val);
		system().optionAudioDSPInterpolation = val;
		SNES::dsp.spc_dsp.interpolation = val;
	}

	TextMenuItem dspInterpolationItem[5]
	{
		{"None", &defaultFace(), [this](){ setDSPInterpolation(0); }},
		{"Linear", &defaultFace(), [this](){ setDSPInterpolation(1); }},
		{"Gaussian", &defaultFace(), [this](){ setDSPInterpolation(2); }},
		{"Cubic", &defaultFace(), [this](){ setDSPInterpolation(3); }},
		{"Sinc", &defaultFace(), [this](){ setDSPInterpolation(4); }},
	};

	MultiChoiceMenuItem dspInterpolation
	{
		"DSP Interpolation", &defaultFace(),
		system().optionAudioDSPInterpolation.val,
		dspInterpolationItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&dspInterpolation);
	}
};
#endif

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	BoolMenuItem multitap
	{
		"5-Player Adapter", &defaultFace(),
		(bool)system().optionMultitap,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionMultitap = item.flipBoolValue(*this);
			system().setupSNESInput(app().defaultVController());
		}
	};

	TextMenuItem inputPortsItem[HAS_NSRT ? 5 : 4]
	{
		#ifndef SNES9X_VERSION_1_4
		{"Auto (NSRT)", &defaultFace(), setInputPortsDel(), SNES_AUTO_INPUT},
		#endif
		{"Gamepads",    &defaultFace(), setInputPortsDel(), SNES_JOYPAD},
		{"Superscope",  &defaultFace(), setInputPortsDel(), SNES_SUPERSCOPE},
		{"Justifier",   &defaultFace(), setInputPortsDel(), SNES_JUSTIFIER},
		{"Mouse",       &defaultFace(), setInputPortsDel(), SNES_MOUSE_SWAPPED},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
		(MenuItem::Id)system().snesInputPort,
		inputPortsItem
	};

	TextMenuItem::SelectDelegate setInputPortsDel()
	{
		return [this](TextMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionInputPort = item.id();
			system().snesInputPort = item.id();
			system().setupSNESInput(app().defaultVController());
		};
	}

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", &defaultFace(), [this](Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC", &defaultFace(), [this](Input::Event e){ setVideoSystem(1, e); }},
		{"PAL", &defaultFace(), [this](Input::Event e){ setVideoSystem(2, e); }},
		{"NTSC + PAL Spoof", &defaultFace(), [this](Input::Event e){ setVideoSystem(3, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"System", &defaultFace(),
		system().optionVideoSystem.val,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		system().sessionOptionSet();
		system().optionVideoSystem = val;
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	TextHeadingMenuItem videoHeading{"Video", &defaultBoldFace()};

	BoolMenuItem allowExtendedLines
	{
		"Allow Extended 239/478 Lines", &defaultFace(),
		(bool)system().optionAllowExtendedVideoLines,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionAllowExtendedVideoLines = item.flipBoolValue(*this);
		}
	};

	#ifndef SNES9X_VERSION_1_4
	TextHeadingMenuItem emulationHacks{"Emulation Hacks", &defaultBoldFace()};

	BoolMenuItem blockInvalidVRAMAccess
	{
		"Allow Invalid VRAM Access", &defaultFace(),
		(bool)!system().optionBlockInvalidVRAMAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionBlockInvalidVRAMAccess = !item.flipBoolValue(*this);
			PPU.BlockInvalidVRAMAccess = system().optionBlockInvalidVRAMAccess;
		}
	};

	BoolMenuItem separateEchoBuffer
	{
		"Separate Echo Buffer From Ram", &defaultFace(),
		(bool)system().optionSeparateEchoBuffer,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionSeparateEchoBuffer = item.flipBoolValue(*this);
			SNES::dsp.spc_dsp.separateEchoBuffer = system().optionSeparateEchoBuffer;
		}
	};

	void setSuperFXClock(unsigned val)
	{
		system().sessionOptionSet();
		system().optionSuperFXClockMultiplier = val;
		setSuperFXSpeedMultiplier(system().optionSuperFXClockMultiplier);
	}

	TextMenuItem superFXClockItem[2]
	{
		{"100%", &defaultFace(), [this]() { setSuperFXClock(100); }},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 5 to 250", "",
					[this](EmuApp &app, auto val)
					{
						if(system().optionSuperFXClockMultiplier.isValidVal(val))
						{
							setSuperFXClock(val);
							superFXClock.setSelected(std::size(superFXClockItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	};

	MultiChoiceMenuItem superFXClock
	{
		"SuperFX Clock Multiplier", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{}%", system().optionSuperFXClockMultiplier.val));
				return true;
			}
		},
		[this]()
		{
			if(system().optionSuperFXClockMultiplier.val == 100)
				return 0;
			else
				return 1;
		}(),
		superFXClockItem
	};
	#endif

	std::array<MenuItem*, IS_SNES9X_VERSION_1_4 ? 5 : 9> menuItem
	{
		&inputPorts,
		&multitap,
		&videoHeading,
		&videoSystem,
		&allowExtendedLines,
		#ifndef SNES9X_VERSION_1_4
		&emulationHacks,
		&blockInvalidVRAMAccess,
		&separateEchoBuffer,
		&superFXClock,
		#endif
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
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(system().hasContent())
			{
				pushAndShow(makeView<ConsoleOptionView>(), e);
			}
		}
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
	using MainAppHelper<CustomFilePathOptionView>::system;
	using MainAppHelper<CustomFilePathOptionView>::app;

	TextMenuItem cheatsPath
	{
		cheatsMenuName(appContext(), system().cheatsDir), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Cheats", system().userPath(system().cheatsDir),
				[this](CStringView path)
				{
					logMsg("set cheats path:%s", path.data());
					system().cheatsDir = path;
					cheatsPath.compile(cheatsMenuName(appContext(), path), renderer());
				}), e);
		}
	};

	TextMenuItem patchesPath
	{
		patchesMenuName(appContext(), system().patchesDir), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Patches", system().userPath(system().patchesDir),
				[this](CStringView path)
				{
					logMsg("set patches path:%s", path.data());
					system().patchesDir = path;
					patchesPath.compile(patchesMenuName(appContext(), path), renderer());
				}), e);
		}
	};

	static std::string satMenuName(IG::ApplicationContext ctx, std::string_view userPath)
	{
		return fmt::format("Satellaview Files: {}", userPathToDisplayName(ctx, userPath));
	}

	TextMenuItem satPath
	{
		satMenuName(appContext(), system().satDir), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Satellaview Files", system().userPath(system().satDir),
				[this](CStringView path)
				{
					logMsg("set satellaview files path:%s", path.data());
					system().satDir = path;
					satPath.compile(satMenuName(appContext(), path), renderer());
				}), e);
		}
	};

	TextMenuItem bsxBios
	{
		bsxMenuName(system().bsxBiosPath), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView>("BS-X BIOS",
				app().validSearchPath(FS::dirnameUri(system().bsxBiosPath)),
				[this](CStringView path, FS::file_type type)
				{
					system().bsxBiosPath = path;
					logMsg("set BS-X bios:%s", path.data());
					bsxBios.compile(bsxMenuName(path), renderer());
					return true;
				}, Snes9xSystem::hasBiosExtension), e);
		}
	};

	std::string bsxMenuName(CStringView path) const
	{
		return fmt::format("BS-X BIOS: {}", appContext().fileUriDisplayName(path));
	}

	TextMenuItem sufamiBios
	{
		sufamiMenuName(system().sufamiBiosPath), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView>("Sufami Turbo BIOS",
				app().validSearchPath(FS::dirnameUri(system().sufamiBiosPath)),
				[this](CStringView path, FS::file_type type)
				{
					system().sufamiBiosPath = path;
					logMsg("set Sufami Turbo bios:%s", path.data());
					sufamiBios.compile(sufamiMenuName(path), renderer());
					return true;
				}, Snes9xSystem::hasBiosExtension), e);
		}
	};

	std::string sufamiMenuName(CStringView path) const
	{
		return fmt::format("Sufami Turbo BIOS: {}", appContext().fileUriDisplayName(path));
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&cheatsPath);
		item.emplace_back(&patchesPath);
		item.emplace_back(&satPath);
		item.emplace_back(&bsxBios);
		item.emplace_back(&sufamiBios);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		#ifndef SNES9X_VERSION_1_4
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		#endif
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}

}

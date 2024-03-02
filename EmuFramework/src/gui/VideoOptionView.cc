/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/VideoOptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/EmuOptions.hh>
#include "PlaceVideoView.hh"
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/TextTableView.hh>
#include <format>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"VideoOptionView"};

class DetectFrameRateView final: public View, public EmuAppHelper<DetectFrameRateView>
{
public:
	using DetectFrameRateDelegate = DelegateFunc<void (SteadyClockTime frameTime)>;
	DetectFrameRateDelegate onDetectFrameTime;
	IG::OnFrameDelegate detectFrameRate;
	SteadyClockTime totalFrameTime{};
	SteadyClockTimePoint lastFrameTimestamp{};
	Gfx::Text fpsText;
	int allTotalFrames{};
	int callbacks{};
	std::vector<SteadyClockTime> frameTimeSample{};
	bool useRenderTaskTime = false;

	DetectFrameRateView(ViewAttachParams attach): View(attach),
		fpsText{attach.rendererTask, &defaultFace()}
	{
		defaultFace().precacheAlphaNum(attach.renderer());
		defaultFace().precache(attach.renderer(), ".");
		fpsText.resetString("Preparing to detect frame rate...");
		useRenderTaskTime = !screen()->supportsTimestamps();
		frameTimeSample.reserve(std::round(screen()->frameRate() * 2.));
	}

	~DetectFrameRateView() final
	{
		window().setIntendedFrameRate(0);
		app().setCPUNeedsLowLatency(appContext(), false);
		window().removeOnFrame(detectFrameRate);
	}

	void place() final
	{
		fpsText.compile();
	}

	bool inputEvent(const Input::Event &e) final
	{
		if(e.keyEvent() && e.keyEvent()->pushed(Input::DefaultKey::CANCEL))
		{
			log.info("aborted detection");
			dismiss();
			return true;
		}
		return false;
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		using namespace IG::Gfx;
		cmds.basicEffect().enableAlphaTexture(cmds);
		fpsText.draw(cmds, viewRect().center(), C2DO, ColorName::WHITE);
	}

	bool runFrameTimeDetection(SteadyClockTime timestampDiff, double slack)
	{
		const int framesToTime = frameTimeSample.capacity() * 10;
		allTotalFrames++;
		frameTimeSample.emplace_back(timestampDiff);
		if(frameTimeSample.size() == frameTimeSample.capacity())
		{
			bool stableFrameTime = true;
			SteadyClockTime frameTimeTotal{};
			{
				SteadyClockTime lastFrameTime{};
				for(auto frameTime : frameTimeSample)
				{
					frameTimeTotal += frameTime;
					if(!stableFrameTime)
						continue;
					double frameTimeDiffSecs =
						std::abs(IG::FloatSeconds(lastFrameTime - frameTime).count());
					if(lastFrameTime.count() && frameTimeDiffSecs > slack)
					{
						log.info("frame times differed by:{}", frameTimeDiffSecs);
						stableFrameTime = false;
					}
					lastFrameTime = frameTime;
				}
			}
			auto frameTimeTotalSecs = FloatSeconds(frameTimeTotal);
			auto detectedFrameTimeSecs = frameTimeTotalSecs / (double)frameTimeSample.size();
			auto detectedFrameTime = round<SteadyClockTime>(detectedFrameTimeSecs);
			{
				if(detectedFrameTime.count())
					fpsText.resetString(std::format("{:g}fps", toHz(detectedFrameTimeSecs)));
				else
					fpsText.resetString("0fps");
				fpsText.compile();
			}
			if(stableFrameTime)
			{
				log.info("found frame time:{}", detectedFrameTimeSecs);
				onDetectFrameTime(detectedFrameTime);
				dismiss();
				return false;
			}
			frameTimeSample.erase(frameTimeSample.cbegin());
			postDraw();
		}
		else
		{
			//log.info("waiting for capacity:{}/{}", frameTimeSample.size(), frameTimeSample.capacity());
		}
		if(allTotalFrames >= framesToTime)
		{
			onDetectFrameTime(SteadyClockTime{});
			dismiss();
			return false;
		}
		else
		{
			if(useRenderTaskTime)
				postDraw();
			return true;
		}
	}

	void onAddedToController(ViewController *, const Input::Event &e) final
	{
		lastFrameTimestamp = SteadyClock::now();
		detectFrameRate =
			[this](IG::FrameParams params)
			{
				const int callbacksToSkip = 10;
				callbacks++;
				if(callbacks < callbacksToSkip)
				{
					if(useRenderTaskTime)
						postDraw();
					return true;
				}
				return runFrameTimeDetection(params.timestamp - std::exchange(lastFrameTimestamp, params.timestamp), 0.00175);
			};
		window().addOnFrame(detectFrameRate);
		app().setCPUNeedsLowLatency(appContext(), true);
	}
};

static std::string makeFrameRateStr(VideoSystem vidSys, const OutputTimingManager &mgr)
{
	auto frameTimeOpt = mgr.frameTimeOption(vidSys);
	if(frameTimeOpt == OutputTimingManager::autoOption)
		return "Auto";
	else if(frameTimeOpt == OutputTimingManager::originalOption)
		return "Original";
	else
		return std::format("{:g}Hz", toHz(frameTimeOpt));
}

static const char *autoWindowPixelFormatStr(IG::ApplicationContext ctx)
{
	return ctx.defaultWindowPixelFormat() == PIXEL_RGB565 ? "RGB565" : "RGBA8888";
}

constexpr uint16_t pack(Gfx::DrawableConfig c)
{
	return to_underlying(c.pixelFormat.id()) | to_underlying(c.colorSpace) << sizeof(c.colorSpace) * 8;
}

constexpr Gfx::DrawableConfig unpackDrawableConfig(uint16_t c)
{
	return {PixelFormatID(c & 0xFF), Gfx::ColorSpace(c >> sizeof(Gfx::DrawableConfig::colorSpace) * 8)};
}

VideoOptionView::VideoOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Video Options", attach, item},
	textureBufferModeItem
	{
		[&]
		{
			decltype(textureBufferModeItem) items;
			items.emplace_back("Auto (Set optimal mode)", attach, [this](View &view)
			{
				app().textureBufferMode = Gfx::TextureBufferMode::DEFAULT;
				auto defaultMode = renderer().makeValidTextureBufferMode();
				emuVideo().setTextureBufferMode(system(), defaultMode);
				textureBufferMode.setSelected(MenuId{defaultMode});
				view.dismiss();
				return false;
			}, MenuItem::Config{.id = 0});
			for(auto desc: renderer().textureBufferModes())
			{
				items.emplace_back(desc.name, attach, [this](MenuItem &item)
				{
					app().textureBufferMode = Gfx::TextureBufferMode(item.id.val);
					emuVideo().setTextureBufferMode(system(), Gfx::TextureBufferMode(item.id.val));
				}, MenuItem::Config{.id = desc.mode});
			}
			return items;
		}()
	},
	textureBufferMode
	{
		"GPU Copy Mode", attach,
		MenuId{renderer().makeValidTextureBufferMode(app().textureBufferMode)},
		textureBufferModeItem
	},
	frameIntervalItem
	{
		{"Full (No Skip)", attach, {.id = 0}},
		{"Full",           attach, {.id = 1}},
		{"1/2",            attach, {.id = 2}},
		{"1/3",            attach, {.id = 3}},
		{"1/4",            attach, {.id = 4}},
	},
	frameInterval
	{
		"Frame Rate Target", attach,
		MenuId{app().frameInterval},
		frameIntervalItem,
		MultiChoiceMenuItem::Config
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().frameInterval.setUnchecked(item.id); }
		},
	},
	frameRateItems
	{
		{"Auto (Match screen when rates are similar)", attach,
			[this]
			{
				if(!app().viewController().emuWindowScreen()->frameRateIsReliable())
				{
					app().postErrorMessage("Reported rate potentially unreliable, "
						"using the detected rate may give better results");
				}
				onFrameTimeChange(activeVideoSystem, OutputTimingManager::autoOption);
			}, {.id = OutputTimingManager::autoOption.count()}
		},
		{"Original (Use emulated system's rate)", attach,
			[this]
			{
				onFrameTimeChange(activeVideoSystem, OutputTimingManager::originalOption);
			}, {.id = OutputTimingManager::originalOption.count()}
		},
		{"Detect Custom Rate", attach,
			[this](const Input::Event &e)
			{
				window().setIntendedFrameRate(system().frameRate());
				auto frView = makeView<DetectFrameRateView>();
				frView->onDetectFrameTime =
					[this](SteadyClockTime frameTime)
					{
						if(frameTime.count())
						{
							if(onFrameTimeChange(activeVideoSystem, frameTime))
								dismissPrevious();
						}
						else
						{
							app().postErrorMessage("Detected rate too unstable to use");
						}
					};
				pushAndShowModal(std::move(frView), e);
				return false;
			}
		},
		{"Custom Rate", attach,
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<std::pair<double, double>>(attachParams(), e,
					"Input decimal or fraction", "",
					[this](EmuApp &, auto val)
					{
						if(onFrameTimeChange(activeVideoSystem, fromSeconds<SteadyClockTime>(val.second / val.first)))
						{
							if(activeVideoSystem == VideoSystem::NATIVE_NTSC)
								frameRate.setSelected(defaultMenuId, *this);
							else
								frameRatePAL.setSelected(defaultMenuId, *this);
							dismissPrevious();
							return true;
						}
						else
							return false;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	frameRate
	{
		"Frame Rate", attach,
		app().outputTimingManager.frameTimeOptionAsMenuId(VideoSystem::NATIVE_NTSC),
		frameRateItems,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(makeFrameRateStr(VideoSystem::NATIVE_NTSC, app().outputTimingManager));
				return true;
			},
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
			{
				activeVideoSystem = VideoSystem::NATIVE_NTSC;
				item.defaultOnSelect(view, e);
			},
		},
	},
	frameRatePAL
	{
		"Frame Rate (PAL)", attach,
		app().outputTimingManager.frameTimeOptionAsMenuId(VideoSystem::PAL),
		frameRateItems,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(makeFrameRateStr(VideoSystem::PAL, app().outputTimingManager));
				return true;
			},
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
			{
				activeVideoSystem = VideoSystem::PAL;
				item.defaultOnSelect(view, e);
			},
		},
	},
	frameTimeStats
	{
		"Show Frame Time Stats", attach,
		app().showFrameTimeStats,
		[this](BoolMenuItem &item) { app().showFrameTimeStats = item.flipBoolValue(*this); }
	},
	aspectRatioItem
	{
		[&]()
		{
			StaticArrayList<TextMenuItem, MAX_ASPECT_RATIO_ITEMS> aspectRatioItem;
			for(const auto &i : EmuSystem::aspectRatioInfos())
			{
				aspectRatioItem.emplace_back(i.name, attach, [this](TextMenuItem &item)
				{
					app().setVideoAspectRatio(std::bit_cast<float>(item.id));
				}, MenuItem::Config{.id = std::bit_cast<MenuId>(i.aspect.ratio<float>())});
			}
			if(EmuSystem::hasRectangularPixels)
			{
				aspectRatioItem.emplace_back("Square Pixels", attach, [this]()
				{
					app().setVideoAspectRatio(-1);
				}, MenuItem::Config{.id = std::bit_cast<MenuId>(-1.f)});
			}
			aspectRatioItem.emplace_back("Fill Display", attach, [this]()
			{
				app().setVideoAspectRatio(0);
			}, MenuItem::Config{.id = 0});
			aspectRatioItem.emplace_back("Custom Value", attach, [this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<std::pair<float, float>>(attachParams(), e,
					"Input decimal or fraction", "",
					[this](EmuApp &app, auto val)
					{
						float ratio = val.first / val.second;
						if(app.setVideoAspectRatio(ratio))
						{
							aspectRatio.setSelected(std::bit_cast<MenuId>(ratio), *this);
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
			}, MenuItem::Config{.id = defaultMenuId});
			return aspectRatioItem;
		}()
	},
	aspectRatio
	{
		"Aspect Ratio", attach,
		std::bit_cast<MenuId>(app().videoAspectRatio()),
		aspectRatioItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == aspectRatioItem.size() - 1)
				{
					t.resetString(std::format("{:g}", app().videoAspectRatio()));
					return true;
				}
				return false;
			}
		},
	},
	zoomItem
	{
		{"100%",                  attach, {.id = 100}},
		{"90%",                   attach, {.id = 90}},
		{"80%",                   attach, {.id = 80}},
		{"Integer-only",          attach, {.id = optionImageZoomIntegerOnly}},
		{"Integer-only (Height)", attach, {.id = optionImageZoomIntegerOnlyY}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 10, 200>(attachParams(), e, "Input 10 to 200", "",
					[this](EmuApp &app, auto val)
					{
						app.setVideoZoom(val);
						zoom.setSelected(MenuId{val}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	zoom
	{
		"Content Zoom", attach,
		MenuId{app().imageZoom},
		zoomItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(app().imageZoom <= 200)
				{
					t.resetString(std::format("{}%", app().imageZoom.value()));
					return true;
				}
				return false;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setVideoZoom(item.id); }
		},
	},
	viewportZoomItem
	{
		{"100%", attach, {.id = 100}},
		{"95%", attach,  {.id = 95}},
		{"90%", attach,  {.id = 90}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 50, 100>(attachParams(), e, "Input 50 to 100", "",
					[this](EmuApp &app, auto val)
					{
						app.setViewportZoom(val);
						viewportZoom.setSelected(MenuId{val}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	viewportZoom
	{
		"App Zoom", attach,
		MenuId{app().viewportZoom},
		viewportZoomItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{}%", app().viewportZoom.value()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setViewportZoom(item.id); }
		},
	},
	contentRotationItem
	{
		{"Auto",        attach, {.id = Rotation::ANY}},
		{"Standard",    attach, {.id = Rotation::UP}},
		{"90° Right",   attach, {.id = Rotation::RIGHT}},
		{"Upside Down", attach, {.id = Rotation::DOWN}},
		{"90° Left",    attach, {.id = Rotation::LEFT}},
	},
	contentRotation
	{
		"Content Rotation", attach,
		MenuId{app().contentRotation.value()},
		contentRotationItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setContentRotation(Rotation(item.id.val)); }
		},
	},
	placeVideo
	{
		"Set Video Position", attach,
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			pushAndShowModal(makeView<PlaceVideoView>(*videoLayer, app().defaultVController()), e);
		}
	},
	imgFilter
	{
		"Image Interpolation", attach,
		app().videoLayer.usingLinearFilter(),
		"None", "Linear",
		[this](BoolMenuItem &item)
		{
			videoLayer->setLinearFilter(item.flipBoolValue(*this));
			app().viewController().postDrawToEmuWindows();
		}
	},
	imgEffectItem
	{
		{"Off",         attach, {.id = ImageEffectId::DIRECT}},
		{"hq2x",        attach, {.id = ImageEffectId::HQ2X}},
		{"Scale2x",     attach, {.id = ImageEffectId::SCALE2X}},
		{"Prescale 2x", attach, {.id = ImageEffectId::PRESCALE2X}},
		{"Prescale 3x", attach, {.id = ImageEffectId::PRESCALE3X}},
		{"Prescale 4x", attach, {.id = ImageEffectId::PRESCALE4X}},
	},
	imgEffect
	{
		"Image Effect", attach,
		MenuId{app().videoLayer.effectId()},
		imgEffectItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				videoLayer->setEffect(system(), ImageEffectId(item.id.val), app().videoEffectPixelFormat());
				app().viewController().postDrawToEmuWindows();
			}
		},
	},
	overlayEffectItem
	{
		{"Off",            attach, {.id = 0}},
		{"Scanlines",      attach, {.id = ImageOverlayId::SCANLINES}},
		{"Scanlines 2x",   attach, {.id = ImageOverlayId::SCANLINES_2}},
		{"LCD Grid",       attach, {.id = ImageOverlayId::LCD}},
		{"CRT Mask",       attach, {.id = ImageOverlayId::CRT_MASK}},
		{"CRT Mask .5x",   attach, {.id = ImageOverlayId::CRT_MASK_2}},
		{"CRT Grille",     attach, {.id = ImageOverlayId::CRT_GRILLE}},
		{"CRT Grille .5x", attach, {.id = ImageOverlayId::CRT_GRILLE_2}}
	},
	overlayEffect
	{
		"Overlay Effect", attach,
		MenuId{app().videoLayer.overlayEffectId()},
		overlayEffectItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				videoLayer->setOverlay(ImageOverlayId(item.id.val));
				app().viewController().postDrawToEmuWindows();
			}
		},
	},
	overlayEffectLevelItem
	{
		{"100%", attach, {.id = 100}},
		{"75%",  attach, {.id = 75}},
		{"50%",  attach, {.id = 50}},
		{"25%",  attach, {.id = 25}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 0, 100>(attachParams(), e, "Input 0 to 100", "",
					[this](EmuApp &app, auto val)
					{
						videoLayer->setOverlayIntensity(val / 100.f);
						app.viewController().postDrawToEmuWindows();
						overlayEffectLevel.setSelected(MenuId{val}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	overlayEffectLevel
	{
		"Overlay Effect Level", attach,
		MenuId{app().videoLayer.overlayIntensity() * 100.f},
		overlayEffectLevelItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{}%", int(videoLayer->overlayIntensity() * 100.f)));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				videoLayer->setOverlayIntensity(item.id / 100.f);
				app().viewController().postDrawToEmuWindows();
			}
		},
	},
	imgEffectPixelFormatItem
	{
		{"Auto (Match display format)", attach, {.id = PIXEL_NONE}},
		{"RGBA8888",                    attach, {.id = PIXEL_RGBA8888}},
		{"RGB565",                      attach, {.id = PIXEL_RGB565}},
	},
	imgEffectPixelFormat
	{
		"Effect Color Format", attach,
		MenuId{app().imageEffectPixelFormat},
		imgEffectPixelFormatItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(app().videoEffectPixelFormat().name());
					return true;
				}
				else
					return false;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().imageEffectPixelFormat = PixelFormatID(item.id.val);
				videoLayer->setEffectFormat(app().videoEffectPixelFormat());
				app().viewController().postDrawToEmuWindows();
			}
		},
	},
	windowPixelFormatItem
	{
		[&]
		{
			decltype(windowPixelFormatItem) items;
			auto setWindowDrawableConfigDel = [this](TextMenuItem &item)
			{
				auto conf = unpackDrawableConfig(item.id);
				if(!app().setWindowDrawableConfig(conf))
				{
					app().postMessage("Restart app for option to take effect");
					return;
				}
				renderPixelFormat.updateDisplayString();
				imgEffectPixelFormat.updateDisplayString();
			};
			items.emplace_back("Auto", attach, setWindowDrawableConfigDel, MenuItem::Config{.id = 0});
			for(auto desc: renderer().supportedDrawableConfigs())
			{
				items.emplace_back(desc.name, attach, setWindowDrawableConfigDel, MenuItem::Config{.id = pack(desc.config)});
			}
			return items;
		}()
	},
	windowPixelFormat
	{
		"Display Color Format", attach,
		MenuId{pack(app().windowDrawableConfig())},
		windowPixelFormatItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(autoWindowPixelFormatStr(appContext()));
					return true;
				}
				else
					return false;
			}
		},
	},
	secondDisplay
	{
		"2nd Window (for testing only)", attach,
		false,
		[this](BoolMenuItem &item)
		{
			app().setEmuViewOnExtraWindow(item.flipBoolValue(*this), appContext().mainScreen());
		}
	},
	showOnSecondScreen
	{
		"External Screen", attach,
		app().showOnSecondScreen,
		"OS Managed", "Emu Content",
		[this](BoolMenuItem &item)
		{
			app().showOnSecondScreen = item.flipBoolValue(*this);
			if(appContext().screens().size() > 1)
				app().setEmuViewOnExtraWindow(app().showOnSecondScreen, *appContext().screens()[1]);
		}
	},
	frameClockItems
	{
		{"Auto",                                  attach, MenuItem::Config{.id = FrameTimeSource::Unset}},
		{"Screen (Less latency & power use)",     attach, MenuItem::Config{.id = FrameTimeSource::Screen}},
		{"Renderer (May buffer multiple frames)", attach, MenuItem::Config{.id = FrameTimeSource::Renderer}},
	},
	frameClock
	{
		"Frame Clock", attach,
		MenuId{FrameTimeSource(app().frameTimeSource)},
		frameClockItems,
		MultiChoiceMenuItem::Config
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(wise_enum::to_string(app().effectiveFrameTimeSource()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().frameTimeSource = FrameTimeSource(item.id.val);
				app().video.resetImage(); // texture can switch between single/double buffered
			}
		},
	},
	presentModeItems
	{
		{"Auto",                                                 attach, MenuItem::Config{.id = Gfx::PresentMode::Auto}},
		{"Immediate (Less compositor latency, may drop frames)", attach, MenuItem::Config{.id = Gfx::PresentMode::Immediate}},
		{"Queued (Better frame rate stability)",                 attach, MenuItem::Config{.id = Gfx::PresentMode::FIFO}},
	},
	presentMode
	{
		"Present Mode", attach,
		MenuId{Gfx::PresentMode(app().presentMode)},
		presentModeItems,
		MultiChoiceMenuItem::Config
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(renderer().evalPresentMode(app().emuWindow(), app().presentMode) == Gfx::PresentMode::FIFO ? "Queued" : "Immediate");
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().presentMode = Gfx::PresentMode(item.id.val);
			}
		},
	},
	renderPixelFormatItem
	{
		{"Auto (Match display format)", attach, {.id = PIXEL_NONE}},
		{"RGBA8888",                    attach, {.id = PIXEL_RGBA8888}},
		{"RGB565",                      attach, {.id = PIXEL_RGB565}},
	},
	renderPixelFormat
	{
		"Render Color Format", attach,
		MenuId{app().renderPixelFormat().id()},
		renderPixelFormatItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(emuVideo().internalRenderPixelFormat().name());
					return true;
				}
				return false;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setRenderPixelFormat(PixelFormatID(item.id.val)); }
		},
	},
	screenFrameRateItems
	{
		[&]
		{
			std::vector<TextMenuItem> items;
			auto setRateDel = [this](TextMenuItem &item) { app().overrideScreenFrameRate = std::bit_cast<FrameRate>(item.id); };
			items.emplace_back("Off", attach, setRateDel, MenuItem::Config{.id = 0});
			for(auto rate : app().emuScreen().supportedFrameRates())
				items.emplace_back(std::format("{:g}Hz", rate), attach, setRateDel, MenuItem::Config{.id = std::bit_cast<MenuId>(rate)});
			return items;
		}()
	},
	screenFrameRate
	{
		"Override Screen Frame Rate", attach,
		std::bit_cast<MenuId>(FrameRate(app().overrideScreenFrameRate)),
		screenFrameRateItems
	},
	presentationTimeItems
	{
		{"Full (Apply to all frame rate targets)",         attach, MenuItem::Config{.id = PresentationTimeMode::full}},
		{"Basic (Only apply to lower frame rate targets)", attach, MenuItem::Config{.id = PresentationTimeMode::basic}},
		{"Off",                                            attach, MenuItem::Config{.id = PresentationTimeMode::off}},
	},
	presentationTime
	{
		"Precise Frame Pacing", attach,
		MenuId{PresentationTimeMode(app().presentationTimeMode)},
		presentationTimeItems,
		MultiChoiceMenuItem::Config
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(app().presentationTimeMode == PresentationTimeMode::off)
					return false;
				t.resetString(app().presentationTimeMode == PresentationTimeMode::full ? "Full" : "Basic");
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().presentationTimeMode = PresentationTimeMode(item.id.val);
			}
		},
	},
	blankFrameInsertion
	{
		"Allow Blank Frame Insertion", attach,
		app().allowBlankFrameInsertion,
		[this](BoolMenuItem &item) { app().allowBlankFrameInsertion = item.flipBoolValue(*this); }
	},
	brightnessItem
	{
		{
			"Default", attach, [this](View &v)
			{
				app().setVideoBrightness(1.f, ImageChannel::All);
				setAllColorLevelsSelected(MenuId{100});
				v.dismiss();
			}
		},
		{"Custom Value", attach, setVideoBrightnessCustomDel(ImageChannel::All)},
	},
	redItem
	{
		{"Default", attach, [this](){ app().setVideoBrightness(1.f, ImageChannel::Red); }, {.id = 100}},
		{"Custom Value", attach, setVideoBrightnessCustomDel(ImageChannel::Red), {.id = defaultMenuId}},
	},
	greenItem
	{
		{"Default", attach, [this](){ app().setVideoBrightness(1.f, ImageChannel::Green); }, {.id = 100}},
		{"Custom Value", attach, setVideoBrightnessCustomDel(ImageChannel::Green), {.id = defaultMenuId}},
	},
	blueItem
	{
		{"Default", attach, [this](){ app().setVideoBrightness(1.f, ImageChannel::Blue); }, {.id = 100}},
		{"Custom Value", attach, setVideoBrightnessCustomDel(ImageChannel::Blue), {.id = defaultMenuId}},
	},
	brightness
	{
		"Set All Levels", attach,
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<TableView>("All Levels", brightnessItem), e);
		}
	},
	red
	{
		"Red", attach,
		MenuId{app().videoBrightnessAsInt(ImageChannel::Red)},
		redItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{}%", app().videoBrightnessAsInt(ImageChannel::Red)));
				return true;
			}
		},
	},
	green
	{
		"Green", attach,
		MenuId{app().videoBrightnessAsInt(ImageChannel::Green)},
		greenItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{}%", app().videoBrightnessAsInt(ImageChannel::Green)));
				return true;
			}
		},
	},
	blue
	{
		"Blue", attach,
		MenuId{app().videoBrightnessAsInt(ImageChannel::Blue)},
		blueItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{}%", app().videoBrightnessAsInt(ImageChannel::Blue)));
				return true;
			}
		},
	},
	visualsHeading{"Visuals", attach},
	screenShapeHeading{"Screen Shape", attach},
	colorLevelsHeading{"Color Levels", attach},
	advancedHeading{"Advanced", attach},
	systemSpecificHeading{"System-specific", attach}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void VideoOptionView::place()
{
	aspectRatio.setSelected(std::bit_cast<MenuId>(app().videoAspectRatio()), *this);
	TableView::place();
}


void VideoOptionView::loadStockItems()
{
	item.emplace_back(&frameInterval);
	item.emplace_back(&frameRate);
	if(EmuSystem::hasPALVideoSystem)
	{
		item.emplace_back(&frameRatePAL);
	}
	if(used(frameTimeStats))
		item.emplace_back(&frameTimeStats);
	item.emplace_back(&visualsHeading);
	item.emplace_back(&imgFilter);
	item.emplace_back(&imgEffect);
	item.emplace_back(&overlayEffect);
	item.emplace_back(&overlayEffectLevel);
	item.emplace_back(&screenShapeHeading);
	item.emplace_back(&zoom);
	item.emplace_back(&viewportZoom);
	item.emplace_back(&aspectRatio);
	item.emplace_back(&contentRotation);
	placeVideo.setActive(system().hasContent());
	item.emplace_back(&placeVideo);
	item.emplace_back(&colorLevelsHeading);
	item.emplace_back(&brightness);
	item.emplace_back(&red);
	item.emplace_back(&green);
	item.emplace_back(&blue);
	item.emplace_back(&advancedHeading);
	item.emplace_back(&textureBufferMode);
	if(windowPixelFormatItem.size() > 2)
	{
		item.emplace_back(&windowPixelFormat);
	}
	if(EmuSystem::canRenderRGBA8888)
		item.emplace_back(&renderPixelFormat);
	item.emplace_back(&imgEffectPixelFormat);
	item.emplace_back(&frameClock);
	if(used(presentMode))
		item.emplace_back(&presentMode);
	if(used(presentationTime) && renderer().supportsPresentationTime())
		item.emplace_back(&presentationTime);
	item.emplace_back(&blankFrameInsertion);
	if(used(screenFrameRate) && app().emuScreen().supportedFrameRates().size() > 1)
		item.emplace_back(&screenFrameRate);
	if(used(secondDisplay))
		item.emplace_back(&secondDisplay);
	if(used(showOnSecondScreen) && app().supportsShowOnSecondScreen(appContext()))
		item.emplace_back(&showOnSecondScreen);
}

void VideoOptionView::setEmuVideoLayer(EmuVideoLayer &videoLayer_)
{
	videoLayer = &videoLayer_;
}

bool VideoOptionView::onFrameTimeChange(VideoSystem vidSys, SteadyClockTime time)
{
	if(!app().outputTimingManager.setFrameTimeOption(vidSys, time))
	{
		app().postMessage(4, true, std::format("{:g}Hz not in valid range", toHz(time)));
		return false;
	}
	return true;
}

TextMenuItem::SelectDelegate VideoOptionView::setVideoBrightnessCustomDel(ImageChannel ch)
{
	return [=, this](const Input::Event &e)
	{
		app().pushAndShowNewCollectValueRangeInputView<int, 0, 200>(attachParams(), e, "Input 0 to 200", "",
			[=, this](EmuApp &app, auto val)
			{
				app.setVideoBrightness(val / 100.f, ch);
				if(ch == ImageChannel::All)
					setAllColorLevelsSelected(MenuId{val});
				else
					[&]() -> MultiChoiceMenuItem&
					{
						switch(ch)
						{
							case ImageChannel::All: break;
							case ImageChannel::Red: return red;
							case ImageChannel::Green: return green;
							case ImageChannel::Blue: return blue;
						}
						bug_unreachable("invalid ImageChannel");
					}().setSelected(MenuId{val}, *this);
				dismissPrevious();
				return true;
			});
		return false;
	};
}

void VideoOptionView::setAllColorLevelsSelected(MenuId val)
{
	red.setSelected(val, *this);
	green.setSelected(val, *this);
	blue.setSelected(val, *this);
}

EmuVideo &VideoOptionView::emuVideo() const
{
	return videoLayer->emuVideo();
}

}

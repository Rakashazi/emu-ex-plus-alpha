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
#include "EmuOptions.hh"
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

class DetectFrameRateView final: public View, public EmuAppHelper<DetectFrameRateView>
{
public:
	using DetectFrameRateDelegate = DelegateFunc<void (IG::FloatSeconds frameTime)>;
	DetectFrameRateDelegate onDetectFrameTime;
	IG::OnFrameDelegate detectFrameRate;
	IG::FrameTime totalFrameTime{};
	IG::FrameTime lastFrameTimestamp{};
	Gfx::Text fpsText;
	int allTotalFrames{};
	int callbacks{};
	std::vector<IG::FrameTime> frameTimeSample{};
	bool useRenderTaskTime = false;

	DetectFrameRateView(ViewAttachParams attach): View(attach),
		fpsText{&defaultFace()}
	{
		defaultFace().precacheAlphaNum(attach.renderer());
		defaultFace().precache(attach.renderer(), ".");
		fpsText.resetString("Preparing to detect frame rate...");
		useRenderTaskTime = !screen()->supportsTimestamps();
		frameTimeSample.reserve(std::round(screen()->frameRate() * 2.));
	}

	~DetectFrameRateView() final
	{
		window().setIntendedFrameRate(0.);
		app().setCPUNeedsLowLatency(appContext(), false);
		window().removeOnFrame(detectFrameRate);
	}

	void place() final
	{
		fpsText.compile(renderer(), projP);
	}

	bool inputEvent(const Input::Event &e) final
	{
		if(e.keyEvent() && e.keyEvent()->pushed(Input::DefaultKey::CANCEL))
		{
			logMsg("aborted detection");
			dismiss();
			return true;
		}
		return false;
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		using namespace IG::Gfx;
		cmds.setColor(1., 1., 1., 1.);
		cmds.basicEffect().enableAlphaTexture(cmds);
		fpsText.draw(cmds, projP.alignToPixel(projP.bounds().center()), C2DO, projP);
	}

	bool runFrameTimeDetection(IG::FrameTime timestampDiff, double slack)
	{
		const int framesToTime = frameTimeSample.capacity() * 10;
		allTotalFrames++;
		frameTimeSample.emplace_back(timestampDiff);
		if(frameTimeSample.size() == frameTimeSample.capacity())
		{
			bool stableFrameTime = true;
			IG::FrameTime frameTimeTotal{};
			{
				IG::FrameTime lastFrameTime{};
				for(auto frameTime : frameTimeSample)
				{
					frameTimeTotal += frameTime;
					if(!stableFrameTime)
						continue;
					double frameTimeDiffSecs =
						std::abs(IG::FloatSeconds(lastFrameTime - frameTime).count());
					if(lastFrameTime.count() && frameTimeDiffSecs > slack)
					{
						logMsg("frame times differed by:%f", frameTimeDiffSecs);
						stableFrameTime = false;
					}
					lastFrameTime = frameTime;
				}
			}
			IG::FloatSeconds frameTimeTotalSecs = IG::FloatSeconds(frameTimeTotal);
			IG::FloatSeconds detectedFrameTime = frameTimeTotalSecs / (double)frameTimeSample.size();
			{
				waitForDrawFinished();
				if(detectedFrameTime.count())
					fpsText.resetString(fmt::format("{:.2f}fps", 1. / detectedFrameTime.count()));
				else
					fpsText.resetString("0fps");
				fpsText.compile(renderer(), projP);
			}
			if(stableFrameTime)
			{
				logMsg("found frame time:%f", detectedFrameTime.count());
				onDetectFrameTime(detectedFrameTime);
				dismiss();
				return false;
			}
			frameTimeSample.erase(frameTimeSample.cbegin());
			postDraw();
		}
		else
		{
			//logMsg("waiting for capacity:%zd/%zd", frameTimeSample.size(), frameTimeSample.capacity());
		}
		if(allTotalFrames >= framesToTime)
		{
			onDetectFrameTime(IG::FloatSeconds{});
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
		lastFrameTimestamp = std::chrono::duration_cast<IG::FrameTime>(IG::steadyClockTimestamp());
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
				return runFrameTimeDetection(params.timestamp() - std::exchange(lastFrameTimestamp, params.timestamp()), 0.00175);
			};
		window().addOnFrame(detectFrameRate);
		app().setCPUNeedsLowLatency(appContext(), true);
	}
};

static auto makeFrameRateStr(EmuSystem &sys)
{
	return fmt::format("Frame Rate: {:.2f}Hz",
		sys.frameRate(VideoSystem::NATIVE_NTSC));
}

static auto makeFrameRatePALStr(EmuSystem &sys)
{
	return fmt::format("Frame Rate (PAL): {:.2f}Hz",
		sys.frameRate(VideoSystem::PAL));
}

TextMenuItem::SelectDelegate VideoOptionView::setFrameIntervalDel()
{
	return [this](TextMenuItem &item)
	{
		app().setFrameInterval(item.id());
		logMsg("set frame interval:%d", item.id());
	};
}

TextMenuItem::SelectDelegate VideoOptionView::setImgEffectDel()
{
	return [this](TextMenuItem &item)
	{
		app().videoEffectOption() = item.id();
		if(emuVideo().image())
		{
			videoLayer->setEffect(system(), (ImageEffectId)item.id(), app().videoEffectPixelFormat());
			app().viewController().postDrawToEmuWindows();
		}
	};
}

TextMenuItem::SelectDelegate VideoOptionView::setOverlayEffectDel()
{
	return [this](TextMenuItem &item)
	{
		app().overlayEffectOption() = item.id();
		videoLayer->setOverlay((ImageOverlayId)item.id());
		app().viewController().postDrawToEmuWindows();
	};
}

TextMenuItem::SelectDelegate VideoOptionView::setImgEffectPixelFormatDel()
{
	return [this](TextMenuItem &item)
	{
		app().videoEffectPixelFormatOption() = item.id();
		videoLayer->setEffectFormat(app().videoEffectPixelFormat());
		app().viewController().postDrawToEmuWindows();
	};
}

TextMenuItem::SelectDelegate VideoOptionView::setRenderPixelFormatDel()
{
	return [this](TextMenuItem &item) { app().setRenderPixelFormat((PixelFormatID)item.id()); };
}

static const char *autoWindowPixelFormatStr(IG::ApplicationContext ctx)
{
	return ctx.defaultWindowPixelFormat() == PIXEL_RGB565 ? "RGB565" : "RGBA8888";
}

TextMenuItem::SelectDelegate VideoOptionView::setWindowDrawableConfigDel(Gfx::DrawableConfig conf)
{
	return [this, conf]()
	{
		if(!app().setWindowDrawableConfig(conf))
		{
			app().postMessage("Restart app for option to take effect");
			return;
		}
		renderPixelFormat.updateDisplayString();
		imgEffectPixelFormat.updateDisplayString();
	};
}

TextMenuItem::SelectDelegate VideoOptionView::setImageBuffersDel()
{
	return [this](TextMenuItem &item)
	{
		app().videoImageBuffersOption() = item.id();
		emuVideo().setImageBuffers(item.id());
	};
}

static int aspectRatioValueIndex(double val)
{
	for(auto i : iotaCount(EmuSystem::aspectRatioInfos().size()))
	{
		if(val == (double)EmuSystem::aspectRatioInfos()[i])
		{
			return i;
		}
	}
	return -1;
}

VideoOptionView::VideoOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Video Options", attach, item},
	textureBufferMode
	{
		"GPU Copy Mode", &defaultFace(),
		0,
		textureBufferModeItem
	},
	frameIntervalItem
	{
		{"Full", &defaultFace(), setFrameIntervalDel(), 1},
		{"1/2",  &defaultFace(), setFrameIntervalDel(), 2},
		{"1/3",  &defaultFace(), setFrameIntervalDel(), 3},
		{"1/4",  &defaultFace(), setFrameIntervalDel(), 4},
	},
	frameInterval
	{
		"Target Frame Rate", &defaultFace(),
		(MenuItem::Id)app().frameInterval(),
		frameIntervalItem
	},
	dropLateFrames
	{
		"Skip Late Frames", &defaultFace(),
		(bool)app().shouldSkipLateFrames(),
		[this](BoolMenuItem &item)
		{
			app().setShouldSkipLateFrames(item.flipBoolValue(*this));
		}
	},
	frameRate
	{
		u"", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowFrameRateSelectMenu(VideoSystem::NATIVE_NTSC, e);
			postDraw();
		}
	},
	frameRatePAL
	{
		u"", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowFrameRateSelectMenu(VideoSystem::PAL, e);
			postDraw();
		}
	},
	aspectRatio
	{
		"Aspect Ratio", &defaultFace(),
		[this](auto idx, Gfx::Text &t)
		{
			if(idx == EmuSystem::aspectRatioInfos().size())
			{
				t.resetString(fmt::format("{:.2f}", app().videoAspectRatio()));
				return true;
			}
			return false;
		},
		(int)EmuSystem::aspectRatioInfos().size(),
		aspectRatioItem
	},
	zoomItem
	{
		{"100%",                  &defaultFace(), setZoomDel(), 100},
		{"90%",                   &defaultFace(), setZoomDel(), 90},
		{"80%",                   &defaultFace(), setZoomDel(), 80},
		{"Integer-only",          &defaultFace(), setZoomDel(), optionImageZoomIntegerOnly},
		{"Integer-only (Height)", &defaultFace(), setZoomDel(), optionImageZoomIntegerOnlyY},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 10, 100>(attachParams(), e, "Input 10 to 100", "",
					[this](EmuApp &app, auto val)
					{
						app.setVideoZoom(val);
						zoom.setSelected((MenuItem::Id)val, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	zoom
	{
		"Content Zoom", &defaultFace(),
		[this](auto idx, Gfx::Text &t)
		{
			if(app().videoZoom() <= 100)
			{
				t.resetString(fmt::format("{}%", app().videoZoom()));
				return true;
			}
			return false;
		},
		(MenuItem::Id)app().videoZoom(),
		zoomItem
	},
	viewportZoomItem
	{
		{"100%", &defaultFace(), setViewportZoomDel(), 100},
		{"95%", &defaultFace(),  setViewportZoomDel(), 95},
		{"90%", &defaultFace(),  setViewportZoomDel(), 90},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 50, 100>(attachParams(), e, "Input 50 to 100", "",
					[this](EmuApp &app, auto val)
					{
						app.setViewportZoom(val);
						viewportZoom.setSelected((MenuItem::Id)val, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	viewportZoom
	{
		"App Zoom", &defaultFace(),
		[this](auto idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{}%", app().viewportZoom()));
			return true;
		},
		(MenuItem::Id)app().viewportZoom(),
		viewportZoomItem
	},
	contentRotationItem
	{
		{"Auto",        &defaultFace(), setContentRotationDel(), std::to_underlying(Rotation::ANY)},
		{"Standard",    &defaultFace(), setContentRotationDel(), std::to_underlying(Rotation::UP)},
		{"90° Right",   &defaultFace(), setContentRotationDel(), std::to_underlying(Rotation::RIGHT)},
		{"Upside Down", &defaultFace(), setContentRotationDel(), std::to_underlying(Rotation::DOWN)},
		{"90° Left",    &defaultFace(), setContentRotationDel(), std::to_underlying(Rotation::LEFT)},
	},
	contentRotation
	{
		"Content Rotation", &defaultFace(),
		(MenuItem::Id)app().contentRotation(),
		contentRotationItem
	},
	imgFilter
	{
		"Image Interpolation", &defaultFace(),
		(bool)app().videoFilterOption(),
		"None", "Linear",
		[this](BoolMenuItem &item)
		{
			app().videoFilterOption().val = item.flipBoolValue(*this);
			videoLayer->setLinearFilter(app().videoFilterOption());
			app().viewController().postDrawToEmuWindows();
		}
	},
	imgEffectItem
	{
		{"Off",         &defaultFace(), setImgEffectDel(), std::to_underlying(ImageEffectId::DIRECT)},
		{"hq2x",        &defaultFace(), setImgEffectDel(), std::to_underlying(ImageEffectId::HQ2X)},
		{"Scale2x",     &defaultFace(), setImgEffectDel(), std::to_underlying(ImageEffectId::SCALE2X)},
		{"Prescale 2x", &defaultFace(), setImgEffectDel(), std::to_underlying(ImageEffectId::PRESCALE2X)},
		{"Prescale 3x", &defaultFace(), setImgEffectDel(), std::to_underlying(ImageEffectId::PRESCALE3X)},
		{"Prescale 4x", &defaultFace(), setImgEffectDel(), std::to_underlying(ImageEffectId::PRESCALE4X)},
	},
	imgEffect
	{
		"Image Effect", &defaultFace(),
		(MenuItem::Id)app().videoEffectOption().val,
		imgEffectItem
	},
	overlayEffectItem
	{
		{"Off",            &defaultFace(), setOverlayEffectDel(), 0},
		{"Scanlines",      &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::SCANLINES)},
		{"Scanlines 2x",   &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::SCANLINES_2)},
		{"LCD Grid",       &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::LCD)},
		{"CRT Mask",       &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::CRT_MASK)},
		{"CRT Mask .5x",   &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::CRT_MASK_2)},
		{"CRT Grille",     &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::CRT_GRILLE)},
		{"CRT Grille .5x", &defaultFace(), setOverlayEffectDel(), std::to_underlying(ImageOverlayId::CRT_GRILLE_2)}
	},
	overlayEffect
	{
		"Overlay Effect", &defaultFace(),
		(MenuItem::Id)app().overlayEffectOption().val,
		overlayEffectItem
	},
	overlayEffectLevelItem
	{
		{"100%", &defaultFace(), setOverlayEffectLevelDel(), 100},
		{"75%",  &defaultFace(), setOverlayEffectLevelDel(), 75},
		{"50%",  &defaultFace(), setOverlayEffectLevelDel(), 50},
		{"25%",  &defaultFace(), setOverlayEffectLevelDel(), 25},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 0, 100>(attachParams(), e, "Input 0 to 100", "",
					[this](EmuApp &app, auto val)
					{
						app.setOverlayEffectLevel(*videoLayer, val);
						overlayEffectLevel.setSelected((MenuItem::Id)val, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	overlayEffectLevel
	{
		"Overlay Effect Level", &defaultFace(),
		[this](auto idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{}%", app().overlayEffectLevel()));
			return true;
		},
		(MenuItem::Id)app().overlayEffectLevel(),
		overlayEffectLevelItem
	},
	imgEffectPixelFormatItem
	{
		{"Auto (Match display format)", &defaultFace(), setImgEffectPixelFormatDel(), PIXEL_NONE},
		{"RGBA8888",                    &defaultFace(), setImgEffectPixelFormatDel(), PIXEL_RGBA8888},
		{"RGB565",                      &defaultFace(), setImgEffectPixelFormatDel(), PIXEL_RGB565},
	},
	imgEffectPixelFormat
	{
		"Effect Color Format", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.resetString(app().videoEffectPixelFormat().name());
				return true;
			}
			else
				return false;
		},
		(MenuItem::Id)app().videoEffectPixelFormatOption().val,
		imgEffectPixelFormatItem
	},
	windowPixelFormat
	{
		"Display Color Format", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.resetString(autoWindowPixelFormatStr(appContext()));
				return true;
			}
			else
				return false;
		},
		0,
		windowPixelFormatItem
	},
	secondDisplay
	{
		"2nd Window (for testing only)", &defaultFace(),
		false,
		[this](BoolMenuItem &item)
		{
			app().setEmuViewOnExtraWindow(item.flipBoolValue(*this), appContext().mainScreen());
		}
	},
	showOnSecondScreen
	{
		"External Screen", &defaultFace(),
		(bool)app().showOnSecondScreenOption(),
		"OS Managed", "Emu Content",
		[this](BoolMenuItem &item)
		{
			app().showOnSecondScreenOption() = item.flipBoolValue(*this);
			if(appContext().screens().size() > 1)
				app().setEmuViewOnExtraWindow(app().showOnSecondScreenOption(), *appContext().screens()[1]);
		}
	},
	imageBuffersItem
	{
		{"Auto",                                     &defaultFace(), setImageBuffersDel(), 0},
		{"1 (Syncs GPU each frame, less input lag)", &defaultFace(), setImageBuffersDel(), 1},
		{"2 (More stable, may add 1 frame of lag)",  &defaultFace(), setImageBuffersDel(), 2},
	},
	imageBuffers
	{
		"Image Buffers", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			t.resetString(emuVideo().imageBuffers() == 1 ? "1" : "2");
			return true;
		},
		(MenuItem::Id)app().videoImageBuffersOption().val,
		imageBuffersItem
	},
	renderPixelFormatItem
	{
		{"Auto (Match display format)", &defaultFace(), setRenderPixelFormatDel(), IG::PIXEL_NONE},
		{"RGBA8888",                    &defaultFace(), setRenderPixelFormatDel(), IG::PIXEL_RGBA8888},
		{"RGB565",                      &defaultFace(), setRenderPixelFormatDel(), IG::PIXEL_RGB565},
	},
	renderPixelFormat
	{
		"Render Color Format", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.resetString(emuVideo().internalRenderPixelFormat().name());
				return true;
			}
			return false;
		},
		(MenuItem::Id)app().renderPixelFormat().id(),
		renderPixelFormatItem
	},
	presentationTime
	{
		"Reduce Compositor Lag", &defaultFace(),
		app().usePresentationTime(),
		[this](BoolMenuItem &item)
		{
			app().setUsePresentationTime(item.flipBoolValue(*this));
		}
	},
	forceMaxScreenFrameRate
	{
		"Force Max Screen Frame Rate", &defaultFace(),
		app().shouldForceMaxScreenFrameRate(),
		[this](BoolMenuItem &item)
		{
			app().setForceMaxScreenFrameRate(item.flipBoolValue(*this));
		}
	},
	brightnessItem
	{
		{
			"Default", &defaultFace(), [this](View &v)
			{
				app().setVideoBrightness(1.f, ImageChannel::All);
				setAllColorLevelsSelected(MenuItem::Id{100});
				v.dismiss();
			}
		},
		{"Custom Value", &defaultFace(), setVideoBrightnessCustomDel(ImageChannel::All)},
	},
	redItem
	{
		{"Default", &defaultFace(), [this](){ app().setVideoBrightness(1.f, ImageChannel::Red); }, 100},
		{"Custom Value", &defaultFace(), setVideoBrightnessCustomDel(ImageChannel::Red), MenuItem::DEFAULT_ID},
	},
	greenItem
	{
		{"Default", &defaultFace(), [this](){ app().setVideoBrightness(1.f, ImageChannel::Green); }, 100},
		{"Custom Value", &defaultFace(), setVideoBrightnessCustomDel(ImageChannel::Green), MenuItem::DEFAULT_ID},
	},
	blueItem
	{
		{"Default", &defaultFace(), [this](){ app().setVideoBrightness(1.f, ImageChannel::Blue); }, 100},
		{"Custom Value", &defaultFace(), setVideoBrightnessCustomDel(ImageChannel::Blue), MenuItem::DEFAULT_ID},
	},
	brightness
	{
		"Set All Levels", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<TableView>("All Levels", brightnessItem), e);
		}
	},
	red
	{
		"Red", &defaultFace(),
		[this](size_t idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{}%", app().videoBrightnessAsInt(ImageChannel::Red)));
			return true;
		},
		MenuItem::Id{app().videoBrightnessAsInt(ImageChannel::Red)},
		redItem
	},
	green
	{
		"Green", &defaultFace(),
		[this](size_t idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{}%", app().videoBrightnessAsInt(ImageChannel::Green)));
			return true;
		},
		MenuItem::Id{app().videoBrightnessAsInt(ImageChannel::Green)},
		greenItem
	},
	blue
	{
		"Blue", &defaultFace(),
		[this](size_t idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{}%", app().videoBrightnessAsInt(ImageChannel::Blue)));
			return true;
		},
		MenuItem::Id{app().videoBrightnessAsInt(ImageChannel::Blue)},
		blueItem
	},
	visualsHeading{"Visuals", &defaultBoldFace()},
	screenShapeHeading{"Screen Shape", &defaultBoldFace()},
	colorLevelsHeading{"Color Levels", &defaultBoldFace()},
	advancedHeading{"Advanced", &defaultBoldFace()},
	systemSpecificHeading{"System-specific", &defaultBoldFace()}
{
	windowPixelFormatItem.emplace_back("Auto", &defaultFace(), setWindowDrawableConfigDel({}));
	{
		auto descs = renderer().supportedDrawableConfigs();
		for(auto desc: descs)
		{
			windowPixelFormatItem.emplace_back(desc.name, &defaultFace(), setWindowDrawableConfigDel(desc.config));
		}
		windowPixelFormat.setSelected(IG::findIndex(descs, app().windowDrawableConfig()) + 1);
	}
	for(const auto &i : EmuSystem::aspectRatioInfos())
	{
		aspectRatioItem.emplace_back(i.name, &defaultFace(),
			[this, aspect = i.aspect]()
			{
				app().setVideoAspectRatio(aspect.ratio<double>());
			});
	}
	aspectRatioItem.emplace_back("Custom Value", &defaultFace(),
		[this](const Input::Event &e)
		{
			app().pushAndShowNewCollectValueInputView<std::pair<double, double>>(attachParams(), e,
				"Input decimal or fraction", "",
				[this](EmuApp &app, auto val)
				{
					double ratio = val.first / val.second;
					if(app.setVideoAspectRatio(ratio))
					{
						if(auto idx = aspectRatioValueIndex(ratio);
							idx != -1)
						{
							aspectRatio.setSelected(idx, *this);
						}
						else
						{
							aspectRatio.setSelected(std::size(aspectRatioItem) - 1, *this);
						}
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
		});
	if(auto idx = aspectRatioValueIndex(app().videoAspectRatio());
		idx != -1)
	{
		aspectRatio.setSelected(idx, *this);
	}
	textureBufferModeItem.emplace_back("Auto (Set optimal mode)", &defaultFace(),
		[this](View &view)
		{
			app().textureBufferModeOption() = 0;
			auto defaultMode = renderer().makeValidTextureBufferMode();
			emuVideo().setTextureBufferMode(system(), defaultMode);
			textureBufferMode.setSelected(IG::findIndex(renderer().textureBufferModes(), defaultMode) + 1);
			view.dismiss();
			return false;
		});
	{
		auto descs = renderer().textureBufferModes();
		for(auto desc: descs)
		{
			textureBufferModeItem.emplace_back(desc.name, &defaultFace(),
				[this, mode = desc.mode]()
				{
					app().textureBufferModeOption() = (uint8_t)mode;
					emuVideo().setTextureBufferMode(system(), mode);
				});
		}
		textureBufferMode.setSelected(IG::findIndex(descs, renderer().makeValidTextureBufferMode((Gfx::TextureBufferMode)app().textureBufferModeOption().val)) + 1);
	}
	if(!customMenu)
	{
		loadStockItems();
	}
}

void VideoOptionView::loadStockItems()
{
	if(used(frameInterval))
		item.emplace_back(&frameInterval);
	item.emplace_back(&dropLateFrames);
	frameRate.setName(makeFrameRateStr(system()));
	item.emplace_back(&frameRate);
	if(EmuSystem::hasPALVideoSystem)
	{
		frameRatePAL.setName(makeFrameRatePALStr(system()));
		item.emplace_back(&frameRatePAL);
	}
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
	if(!app().videoImageBuffersOption().isConst)
		item.emplace_back(&imageBuffers);
	if(IG::used(presentationTime) && renderer().supportsPresentationTime())
		item.emplace_back(&presentationTime);
	if(IG::used(forceMaxScreenFrameRate) && appContext().androidSDK() >= 30)
		item.emplace_back(&forceMaxScreenFrameRate);
	if(IG::used(secondDisplay))
		item.emplace_back(&secondDisplay);
	if(IG::used(showOnSecondScreen) && !app().showOnSecondScreenOption().isConst)
		item.emplace_back(&showOnSecondScreen);
}

void VideoOptionView::setEmuVideoLayer(EmuVideoLayer &videoLayer_)
{
	videoLayer = &videoLayer_;
}

bool VideoOptionView::onFrameTimeChange(VideoSystem vidSys, IG::FloatSeconds time)
{
	if(auto [effectiveFrameTime, isValid] = app().setFrameTime(vidSys, time);
		!isValid)
	{
		app().postMessage(4, true, fmt::format("{:.2f}Hz not in valid range", 1. / effectiveFrameTime.count()));
		return false;
	}
	if(vidSys == VideoSystem::NATIVE_NTSC)
	{
		frameRate.compile(makeFrameRateStr(system()), renderer(), projP);
	}
	else
	{
		frameRatePAL.compile(makeFrameRatePALStr(system()), renderer(), projP);
	}
	return true;
}

void VideoOptionView::pushAndShowFrameRateSelectMenu(VideoSystem vidSys, const Input::Event &e)
{
	const bool includeFrameRateDetection = !Config::envIsIOS;
	auto multiChoiceView = makeViewWithName<TextTableView>("Frame Rate", includeFrameRateDetection ? 4 : 3);
	multiChoiceView->appendItem("Set screen's reported rate",
		[this, vidSys](View &view)
		{
			if(!app().viewController().emuWindowScreen()->frameRateIsReliable())
			{
				if(Config::envIsAndroid && appContext().androidSDK() <= 10)
				{
					app().postErrorMessage("Many Android 2.3 devices mis-report their refresh rate, "
						"using the detected or default rate may give better results");
				}
				else
				{
					app().postErrorMessage("Reported rate potentially unreliable, "
						"using the detected or default rate may give better results");
				}
			}
			if(onFrameTimeChange(vidSys, {}))
				view.dismiss();
		});
	multiChoiceView->appendItem("Set emulated system's rate",
		[this, vidSys](View &view)
		{
			onFrameTimeChange(vidSys, EmuSystem::defaultFrameTime(vidSys));
			view.dismiss();
		});
	multiChoiceView->appendItem("Set custom rate",
		[this, vidSys](const Input::Event &e)
		{
			app().pushAndShowNewCollectValueInputView<std::pair<double, double>>(attachParams(), e,
				"Input decimal or fraction", "",
				[this, vidSys](EmuApp &, auto val)
				{
					if(onFrameTimeChange(vidSys, IG::FloatSeconds{val.second / val.first}))
					{
						dismissPrevious();
						return true;
					}
					else
						return false;
				});
		});
	if(includeFrameRateDetection)
	{
		multiChoiceView->appendItem("Detect screen's rate and set",
			[this, vidSys](const Input::Event &e)
			{
				window().setIntendedFrameRate(1. / EmuSystem::defaultFrameTime(vidSys).count());
				auto frView = makeView<DetectFrameRateView>();
				frView->onDetectFrameTime =
					[this, vidSys](IG::FloatSeconds frameTime)
					{
						if(frameTime.count())
						{
							if(onFrameTimeChange(vidSys, frameTime))
								dismissPrevious();
						}
						else
						{
							app().postErrorMessage("Detected rate too unstable to use");
						}
					};
				pushAndShowModal(std::move(frView), e);
			});
	}
	pushAndShow(std::move(multiChoiceView), e);
}

TextMenuItem::SelectDelegate VideoOptionView::setZoomDel()
{
	return [this](TextMenuItem &item) { app().setVideoZoom(item.id()); };
}

TextMenuItem::SelectDelegate VideoOptionView::setViewportZoomDel()
{
	return [this](TextMenuItem &item) { app().setViewportZoom(item.id()); };
}

TextMenuItem::SelectDelegate VideoOptionView::setContentRotationDel()
{
	return [this](TextMenuItem &item) { app().setContentRotation((IG::Rotation)item.id()); };
}

TextMenuItem::SelectDelegate VideoOptionView::setOverlayEffectLevelDel()
{
	return [this](TextMenuItem &item) { app().setOverlayEffectLevel(*videoLayer, item.id()); };
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
					setAllColorLevelsSelected(MenuItem::Id{val});
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
					}().setSelected(MenuItem::Id{val}, *this);
				dismissPrevious();
				return true;
			});
		return false;
	};
}

void VideoOptionView::setAllColorLevelsSelected(MenuItem::Id val)
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

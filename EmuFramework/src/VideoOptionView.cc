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

#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/VideoImageEffect.hh>
#include "EmuOptions.hh"
#include "private.hh"
#include "EmuViewController.hh"
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>

using namespace IG;

class DetectFrameRateView final: public View
{
public:
	using DetectFrameRateDelegate = DelegateFunc<void (IG::FloatSeconds frameTime)>;
	DetectFrameRateDelegate onDetectFrameTime;
	Base::OnFrameDelegate detectFrameRate;
	Base::FrameTime totalFrameTime{};
	Base::FrameTime lastFrameTimestamp{};
	Gfx::Text fpsText;
	uint allTotalFrames = 0;
	uint callbacks = 0;
	std::vector<Base::FrameTime> frameTimeSample{};
	bool useRenderTaskTime = false;

	DetectFrameRateView(ViewAttachParams attach): View(attach),
		fpsText{nullptr, &View::defaultFace}
	{
		View::defaultFace.precacheAlphaNum(attach.renderer());
		View::defaultFace.precache(attach.renderer(), ".");
		fpsText.setString("Preparing to detect frame rate...");
		useRenderTaskTime = !screen()->supportsTimestamps(appContext());
		frameTimeSample.reserve(std::round(screen()->frameRate() * 2.));
	}

	~DetectFrameRateView() final
	{
		window().setIntendedFrameRate(0.);
		setCPUNeedsLowLatency(appContext(), false);
		window().removeOnFrame(detectFrameRate);
	}

	void place() final
	{
		fpsText.compile(renderer(), projP);
	}

	bool inputEvent(Input::Event e) final
	{
		if(e.pushed() && e.isDefaultCancelButton())
		{
			logMsg("aborted detection");
			dismiss();
			return true;
		}
		return false;
	}

	void draw(Gfx::RendererCommands &cmds) final
	{
		using namespace Gfx;
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA, projP.makeTranslate());
		fpsText.draw(cmds, projP.alignXToPixel(projP.bounds().xCenter()),
			projP.alignYToPixel(projP.bounds().yCenter()), C2DO, projP);
	}

	bool runFrameTimeDetection(Base::FrameTime timestampDiff, double slack)
	{
		const uint framesToTime = frameTimeSample.capacity() * 10;
		allTotalFrames++;
		frameTimeSample.emplace_back(timestampDiff);
		if(frameTimeSample.size() == frameTimeSample.capacity())
		{
			bool stableFrameTime = true;
			Base::FrameTime frameTimeTotal{};
			{
				Base::FrameTime lastFrameTime{};
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
				std::array<char, 32> fpsStr{};
				if(detectedFrameTime.count())
					string_printf(fpsStr, "%.2ffps", 1. / detectedFrameTime.count());
				else
					string_printf(fpsStr, "0fps");
				fpsText.setString(fpsStr.data());
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
			onDetectFrameTime({});
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

	void onAddedToController(ViewController *, Input::Event e) final
	{
		lastFrameTimestamp = std::chrono::duration_cast<IG::FrameTime>(IG::steadyClockTimestamp());
		detectFrameRate =
			[this](IG::FrameParams params)
			{
				const uint callbacksToSkip = 10;
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
		setCPUNeedsLowLatency(appContext(), true);
	}
};

static std::array<char, 64> makeFrameRateStr()
{
	return string_makePrintf<64>("Frame Rate: %.2fHz",
		EmuSystem::frameRate(EmuSystem::VIDSYS_NATIVE_NTSC));
}

static std::array<char, 64> makeFrameRatePALStr()
{
	return string_makePrintf<64>("Frame Rate (PAL): %.2fHz",
		EmuSystem::frameRate(EmuSystem::VIDSYS_PAL));
}

#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
static void setFrameInterval(int interval)
{
	optionFrameInterval = interval;
	logMsg("set frame interval: %d", int(optionFrameInterval));
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffect(uint val, EmuVideoLayer &layer)
{
	optionImgEffect = val;
	if(layer.emuVideo().image())
	{
		layer.setEffect(val, optionImageEffectPixelFormatValue());
		emuViewController().postDrawToEmuWindows();
	}
}
#endif

static void setOverlayEffect(uint val, EmuVideoLayer &layer)
{
	optionOverlayEffect = val;
	layer.setOverlay(val);
	emuViewController().postDrawToEmuWindows();
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffectPixelFormat(PixelFormatID format, EmuVideoLayer &layer)
{
	optionImageEffectPixelFormat = format;
	layer.setEffectFormat(format);
	emuViewController().postDrawToEmuWindows();
}
#endif

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
static const char *autoWindowPixelFormatStr(Base::ApplicationContext app)
{
	return Base::Window::defaultPixelFormat(app) == PIXEL_RGB565 ? "RGB565" : "RGBA8888";
}

static void setWindowPixelFormat(PixelFormatID format)
{
	EmuApp::postMessage("Restart app for option to take effect");
	optionWindowPixelFormat = format;
}
#endif

static void setImageBuffers(unsigned buffers, EmuVideoLayer &layer)
{
	optionVideoImageBuffers = buffers;
	layer.setImageBuffers(buffers);
}

static int aspectRatioValueIndex(double val)
{
	iterateTimes(EmuSystem::aspectRatioInfos, i)
	{
		if(val == (double)EmuSystem::aspectRatioInfo[i])
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
		"GPU Copy Mode",
		0,
		textureBufferModeItem
	},
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	frameIntervalItem
	{
		{"Full", [this]() { setFrameInterval(1); }},
		{"1/2", [this]() { setFrameInterval(2); }},
		{"1/3", [this]() { setFrameInterval(3); }},
		{"1/4", [this]() { setFrameInterval(4); }},
	},
	frameInterval
	{
		"Target Frame Rate",
		optionFrameInterval - 1,
		frameIntervalItem
	},
	#endif
	dropLateFrames
	{
		"Skip Late Frames",
		(bool)optionSkipLateFrames,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSkipLateFrames.val = item.flipBoolValue(*this);
		}
	},
	frameRate
	{
		nullptr,
		[this](Input::Event e)
		{
			pushAndShowFrameRateSelectMenu(EmuSystem::VIDSYS_NATIVE_NTSC, e);
			postDraw();
		}
	},
	frameRatePAL
	{
		nullptr,
		[this](Input::Event e)
		{
			pushAndShowFrameRateSelectMenu(EmuSystem::VIDSYS_PAL, e);
			postDraw();
		}
	},
	aspectRatio
	{
		"Aspect Ratio",
		[this](uint32_t idx, Gfx::Text &t)
		{
			if(idx == EmuSystem::aspectRatioInfos)
			{
				t.setString(string_makePrintf<6>("%.2f", optionAspectRatio.val).data());
				return true;
			}
			return false;
		},
		(int)EmuSystem::aspectRatioInfos,
		aspectRatioItem
	},
	zoomItem
	{
		{"100%", [this]() { setZoom(100); }},
		{"90%", [this]() { setZoom(90); }},
		{"80%", [this]() { setZoom(80); }},
		{"Integer-only", [this]() { setZoom(optionImageZoomIntegerOnly); }},
		{"Integer-only (Height)", [this]() { setZoom(optionImageZoomIntegerOnlyY); }},
		{"Custom Value",
			[this](Input::Event e)
			{
				EmuApp::pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 10 to 100", "",
					[this](auto val)
					{
						if(optionImageZoom.isValidVal(val))
						{
							setZoom(val);
							zoom.setSelected(std::size(zoomItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							EmuApp::postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	},
	zoom
	{
		"Content Zoom",
		[this](uint32_t idx, Gfx::Text &t)
		{
			if(optionImageZoom <= 100)
			{
				t.setString(string_makePrintf<5>("%u%%", optionImageZoom.val).data());
				return true;
			}
			return false;
		},
		[]()
		{
			switch(optionImageZoom.val)
			{
				case 100: return 0;
				case 90: return 1;
				case 80: return 2;
				case optionImageZoomIntegerOnly: return 3;
				case optionImageZoomIntegerOnlyY: return 4;
				default: return 5;
			}
		}(),
		zoomItem
	},
	viewportZoomItem
	{
		{"100%", [this]() { setViewportZoom(100); }},
		{"95%", [this]() { setViewportZoom(95); }},
		{"90%", [this]() { setViewportZoom(90); }},
		{"Custom Value",
			[this](Input::Event e)
			{
				EmuApp::pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 50 to 100", "",
					[this](auto val)
					{
						if(optionViewportZoom.isValidVal(val))
						{
							setViewportZoom(val);
							viewportZoom.setSelected(std::size(viewportZoomItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							EmuApp::postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	},
	viewportZoom
	{
		"App Zoom",
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<5>("%u%%", optionViewportZoom.val).data());
			return true;
		},
		[]()
		{
			switch(optionViewportZoom.val)
			{
				case 100: return 0;
				case 95: return 1;
				case 90: return 2;
				default: return 3;
			}
		}(),
		viewportZoomItem
	},
	imgFilter
	{
		"Image Interpolation",
		(bool)optionImgFilter,
		"None", "Linear",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionImgFilter.val = item.flipBoolValue(*this);
			videoLayer->setLinearFilter(optionImgFilter);
			emuViewController().postDrawToEmuWindows();
		}
	},
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffectItem
	{
		{"Off", [this]() { setImgEffect(0, *videoLayer); }},
		{"hq2x", [this]() { setImgEffect(VideoImageEffect::HQ2X, *videoLayer); }},
		{"Scale2x", [this]() { setImgEffect(VideoImageEffect::SCALE2X, *videoLayer); }},
		{"Prescale 2x", [this]() { setImgEffect(VideoImageEffect::PRESCALE2X, *videoLayer); }}
	},
	imgEffect
	{
		"Image Effect",
		[]()
		{
			switch(optionImgEffect)
			{
				default: return 0;
				case VideoImageEffect::HQ2X: return 1;
				case VideoImageEffect::SCALE2X: return 2;
				case VideoImageEffect::PRESCALE2X: return 3;
			}
		}(),
		imgEffectItem
	},
	#endif
	overlayEffectItem
	{
		{"Off", [this]() { setOverlayEffect(0, *videoLayer); }},
		{"Scanlines", [this]() { setOverlayEffect(VideoImageOverlay::SCANLINES, *videoLayer); }},
		{"Scanlines 2x", [this]() { setOverlayEffect(VideoImageOverlay::SCANLINES_2, *videoLayer); }},
		{"CRT Mask", [this]() { setOverlayEffect(VideoImageOverlay::CRT, *videoLayer); }},
		{"CRT", [this]() { setOverlayEffect(VideoImageOverlay::CRT_RGB, *videoLayer); }},
		{"CRT 2x", [this]() { setOverlayEffect(VideoImageOverlay::CRT_RGB_2, *videoLayer); }}
	},
	overlayEffect
	{
		"Overlay Effect",
		[]()
		{
			switch(optionOverlayEffect)
			{
				default: return 0;
				case VideoImageOverlay::SCANLINES: return 1;
				case VideoImageOverlay::SCANLINES_2: return 2;
				case VideoImageOverlay::CRT: return 3;
				case VideoImageOverlay::CRT_RGB: return 4;
				case VideoImageOverlay::CRT_RGB_2: return 5;
			}
		}(),
		overlayEffectItem
	},
	overlayEffectLevelItem
	{
		{"100%", [this]() { setOverlayEffectLevel(100); }},
		{"75%", [this]() { setOverlayEffectLevel(75); }},
		{"50%", [this]() { setOverlayEffectLevel(50); }},
		{"25%", [this]() { setOverlayEffectLevel(25); }},
		{"Custom Value",
			[this](Input::Event e)
			{
				EmuApp::pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 10 to 100", "",
					[this](auto val)
					{
						if(optionOverlayEffectLevel.isValidVal(val))
						{
							setOverlayEffectLevel(val);
							overlayEffectLevel.setSelected(std::size(overlayEffectLevelItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							EmuApp::postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	},
	overlayEffectLevel
	{
		"Overlay Effect Level",
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<5>("%u%%", optionOverlayEffectLevel.val).data());
			return true;
		},
		[]()
		{
			switch(optionOverlayEffectLevel)
			{
				case 100: return 0;
				case 75: return 1;
				case 50: return 2;
				case 25: return 3;
				default: return 4;
			}
		}(),
		overlayEffectLevelItem
	},
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffectPixelFormatItem
	{
		{"Auto (Match render format as needed)", [this]() { setImgEffectPixelFormat(PIXEL_NONE, *videoLayer); }},
		{"RGB565", [this]() { setImgEffectPixelFormat(PIXEL_RGB565, *videoLayer); }},
		{"RGBA8888", [this]() { setImgEffectPixelFormat(PIXEL_RGBA8888, *videoLayer);}},
	},
	imgEffectPixelFormat
	{
		"Effect Color Format",
		[](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.setString("Auto");
				return true;
			}
			else
				return false;
		},
		[]()
		{
			switch(optionImageEffectPixelFormat.val)
			{
				default: return 0;
				case PIXEL_RGB565: return 1;
				case PIXEL_RGBA8888: return 2;
			}
		}(),
		imgEffectPixelFormatItem
	},
	#endif
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	windowPixelFormatItem
	{
		{"Auto", [this]() { setWindowPixelFormat(PIXEL_NONE); }},
		{"RGB565", [this]() { setWindowPixelFormat(PIXEL_RGB565); }},
		{"RGB888", [this]() { setWindowPixelFormat(PIXEL_RGB888); }},
		{"RGBX8888", [this]() { setWindowPixelFormat(PIXEL_RGBX8888); }},
		{"RGBA8888", [this]() { setWindowPixelFormat(PIXEL_RGBA8888); }},
	},
	windowPixelFormat
	{
		"Display Color Format",
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.setString(autoWindowPixelFormatStr(appContext()));
				return true;
			}
			else
				return false;
		},
		[]()
		{
			switch(optionWindowPixelFormat.val)
			{
				default: return 0;
				case PIXEL_RGB565: return 1;
				case PIXEL_RGB888: return 2;
				case PIXEL_RGBX8888: return 3;
				case PIXEL_RGBA8888: return 4;
			}
		}(),
		windowPixelFormatItem
	},
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	secondDisplay
	{
		"2nd Window (for testing only)",
		false,
		[this](BoolMenuItem &item, Input::Event e)
		{
			emuViewController().setEmuViewOnExtraWindow(item.flipBoolValue(*this), *appContext().screen(0));
		}
	},
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	showOnSecondScreen
	{
		"External Screen",
		(bool)optionShowOnSecondScreen,
		"OS Managed", "Game Content",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowOnSecondScreen = item.flipBoolValue(*this);
			if(appContext().screens() > 1)
				emuViewController().setEmuViewOnExtraWindow(optionShowOnSecondScreen, *appContext().screen(1));
		}
	},
	#endif
	imageBuffersItem
	{
		{"Auto", [this]() { setImageBuffers(0, *videoLayer); }},
		{"1 (Syncs GPU each frame, less input lag)", [this]() { setImageBuffers(1, *videoLayer); }},
		{"2 (More stable, may add 1 frame of lag)", [this]() { setImageBuffers(2, *videoLayer); }},
	},
	imageBuffers
	{
		"Image Buffers",
		[this](int idx, Gfx::Text &t)
		{
			t.setString(videoLayer->imageBuffers() == 1 ? "1" : "2");
			return true;
		},
		[]()
		{
			switch(optionVideoImageBuffers.val)
			{
				default: return 0;
				case 1: return 1;
				case 2: return 2;
			}
		}(),
		imageBuffersItem
	},
	visualsHeading{"Visuals"},
	screenShapeHeading{"Screen Shape"},
	advancedHeading{"Advanced"},
	systemSpecificHeading{"System-specific"}
{
	iterateTimes(EmuSystem::aspectRatioInfos, i)
	{
		aspectRatioItem.emplace_back(EmuSystem::aspectRatioInfo[i].name,
			[this, i]()
			{
				setAspectRatio((double)EmuSystem::aspectRatioInfo[i]);
			});
	}
	aspectRatioItem.emplace_back("Custom Value",
		[this](Input::Event e)
		{
			EmuApp::pushAndShowNewCollectValueInputView<std::pair<double, double>>(attachParams(), e,
				"Input decimal or fraction", "",
				[this](auto val)
				{
					double ratio = val.first / val.second;
					if(optionAspectRatio.isValidVal(ratio))
					{
						setAspectRatio(ratio);
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
						EmuApp::postErrorMessage("Value not in range");
						return false;
					}
				});
			return false;
		});
	if(auto idx = aspectRatioValueIndex(optionAspectRatio);
		idx != -1)
	{
		aspectRatio.setSelected(idx, *this);
	}
	textureBufferModeItem.emplace_back("Auto (Set optimal mode)",
		[this](View &view)
		{
			optionTextureBufferMode = 0;
			auto defaultMode = renderer().makeValidTextureBufferMode();
			videoLayer->setTextureBufferMode(defaultMode);
			textureBufferMode.setSelected(idxOfBufferMode(defaultMode));
			view.dismiss();
			return false;
		});
	for(auto desc: renderer().textureBufferModes())
	{
		textureBufferModeItem.emplace_back(desc.name,
			[this, mode = desc.mode]()
			{
				optionTextureBufferMode = (uint8_t)mode;
				videoLayer->setTextureBufferMode(mode);
			});
	}
	if(!customMenu)
	{
		loadStockItems();
	}
}

void VideoOptionView::loadStockItems()
{
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	item.emplace_back(&frameInterval);
	#endif
	item.emplace_back(&dropLateFrames);
	if(!optionFrameRate.isConst)
	{
		frameRate.setName(makeFrameRateStr().data());
		item.emplace_back(&frameRate);
	}
	if(!optionFrameRatePAL.isConst)
	{
		frameRatePAL.setName(makeFrameRatePALStr().data());
		item.emplace_back(&frameRatePAL);
	}
	item.emplace_back(&visualsHeading);
	item.emplace_back(&imgFilter);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	item.emplace_back(&imgEffect);
	#endif
	item.emplace_back(&overlayEffect);
	item.emplace_back(&overlayEffectLevel);
	item.emplace_back(&screenShapeHeading);
	item.emplace_back(&zoom);
	item.emplace_back(&viewportZoom);
	item.emplace_back(&aspectRatio);
	item.emplace_back(&advancedHeading);
	item.emplace_back(&textureBufferMode);
	textureBufferMode.setSelected(
		idxOfBufferMode(renderer().makeValidTextureBufferMode((Gfx::TextureBufferMode)optionTextureBufferMode.val)));
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	item.emplace_back(&imgEffectPixelFormat);
	#endif
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	item.emplace_back(&windowPixelFormat);
	#endif
	if(!optionVideoImageBuffers.isConst)
		item.emplace_back(&imageBuffers);
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	item.emplace_back(&secondDisplay);
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	if(!optionShowOnSecondScreen.isConst)
	{
		item.emplace_back(&showOnSecondScreen);
	}
	#endif
}

void VideoOptionView::setEmuVideoLayer(EmuVideoLayer &videoLayer_)
{
	videoLayer = &videoLayer_;
}

bool VideoOptionView::onFrameTimeChange(EmuSystem::VideoSystem vidSys, IG::FloatSeconds time)
{
	auto wantedTime = time;
	if(!time.count())
	{
		wantedTime = emuViewController().emuWindowScreen()->frameTime();
	}
	if(!EmuSystem::setFrameTime(vidSys, wantedTime))
	{
		EmuApp::printfMessage(4, true, "%.2fHz not in valid range", 1. / wantedTime.count());
		return false;
	}
	EmuSystem::configFrameTime(optionSoundRate);
	if(vidSys == EmuSystem::VIDSYS_NATIVE_NTSC)
	{
		optionFrameRate = time.count();
		frameRate.setName(makeFrameRateStr().data());
		frameRate.compile(renderer(), projP);
	}
	else
	{
		optionFrameRatePAL = time.count();
		frameRatePAL.compile(makeFrameRatePALStr().data(), renderer(), projP);
	}
	return true;
}

void VideoOptionView::pushAndShowFrameRateSelectMenu(EmuSystem::VideoSystem vidSys, Input::Event e)
{
	const bool includeFrameRateDetection = !Config::envIsIOS;
	auto multiChoiceView = makeViewWithName<TextTableView>("Frame Rate", includeFrameRateDetection ? 4 : 3);
	multiChoiceView->appendItem("Set with screen's reported rate",
		[this, vidSys](View &view, Input::Event e)
		{
			if(!emuViewController().emuWindowScreen()->frameRateIsReliable())
			{
				#ifdef __ANDROID__
				if(appContext().androidSDK() <= 10)
				{
					EmuApp::postErrorMessage("Many Android 2.3 devices mis-report their refresh rate, "
						"using the detected or default rate may give better results");
				}
				else
				#endif
				{
					EmuApp::postErrorMessage("Reported rate potentially unreliable, "
						"using the detected or default rate may give better results");
				}
			}
			if(onFrameTimeChange(vidSys, {}))
				view.dismiss();
		});
	multiChoiceView->appendItem("Set default rate",
		[this, vidSys](View &view, Input::Event e)
		{
			onFrameTimeChange(vidSys, EmuSystem::defaultFrameTime(vidSys));
			view.dismiss();
		});
	multiChoiceView->appendItem("Set custom rate",
		[this, vidSys](Input::Event e)
		{
			EmuApp::pushAndShowNewCollectValueInputView<std::pair<double, double>>(attachParams(), e,
				"Input decimal or fraction", "",
				[this, vidSys](auto val)
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
			[this, vidSys](Input::Event e)
			{
				window().setIntendedFrameRate(vidSys == EmuSystem::VIDSYS_NATIVE_NTSC ? 60. : 50.);
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
							EmuApp::postErrorMessage("Detected rate too unstable to use");
						}
					};
				pushAndShowModal(std::move(frView), e);
			});
	}
	pushAndShow(std::move(multiChoiceView), e);
}

void VideoOptionView::setZoom(uint8_t val)
{
	optionImageZoom = val;
	logMsg("set image zoom: %d", int(optionImageZoom));
	emuViewController().placeEmuViews();
	emuViewController().postDrawToEmuWindows();
}

void VideoOptionView::setViewportZoom(uint8_t val)
{
	optionViewportZoom = val;
	logMsg("set viewport zoom: %d", int(optionViewportZoom));
	emuViewController().startMainViewportAnimation();
}

void VideoOptionView::setOverlayEffectLevel(uint8_t val)
{
	optionOverlayEffectLevel = val;
	videoLayer->setOverlayIntensity(val/100.);
	emuViewController().postDrawToEmuWindows();
}

void VideoOptionView::setAspectRatio(double val)
{
	optionAspectRatio = val;
	logMsg("set aspect ratio: %.2f", val);
	emuViewController().placeEmuViews();
	emuViewController().postDrawToEmuWindows();
}

unsigned VideoOptionView::idxOfBufferMode(Gfx::TextureBufferMode mode)
{
	for(unsigned idx = 0; auto desc: renderer().textureBufferModes())
	{
		if(desc.mode == mode)
		{
			assert(idx + 1 < std::size(textureBufferModeItem));
			return idx + 1;
		}
		idx++;
	}
	return 0;
}

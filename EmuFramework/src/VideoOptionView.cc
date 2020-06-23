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
#include <emuframework/VideoImageEffect.hh>
#include "EmuOptions.hh"
#include "private.hh"
#include <imagine/base/Screen.hh>
#include <imagine/base/Base.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>

using namespace IG;

class DetectFrameRateView : public View
{
public:
	using DetectFrameRateDelegate = DelegateFunc<void (IG::FloatSeconds frameTime)>;
	DetectFrameRateDelegate onDetectFrameTime;
	Base::OnFrameDelegate detectFrameRate;
	Base::FrameTime totalFrameTime{};
	Gfx::Text fpsText;
	uint allTotalFrames = 0;
	uint callbacks = 0;
	std::array<char, 32> fpsStr{};
	std::vector<Base::FrameTime> frameTimeSample{};
	bool useRenderTaskTime = false;

	DetectFrameRateView(ViewAttachParams attach): View(attach),
		fpsText{nullptr, &View::defaultFace}
	{
		View::defaultFace.precacheAlphaNum(attach.renderer());
		View::defaultFace.precache(attach.renderer(), ".");
		fpsText.setString("Preparing to detect frame rate...");
		useRenderTaskTime = !screen()->supportsTimestamps();
		frameTimeSample.reserve(std::round(screen()->frameRate() * 2.));
	}

	~DetectFrameRateView() final
	{
		setCPUNeedsLowLatency(false);
		if(!useRenderTaskTime)
			screen()->removeOnFrame(detectFrameRate);
		else
			appWindowData(window()).drawableHolder.removeOnFrame(detectFrameRate);
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
			popAndShow();
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
				popAndShow();
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
			popAndShow();
			return false;
		}
		else
		{
			if(useRenderTaskTime)
				postDraw();
			return true;
		}
	}

	void onAddedToController(Input::Event e) final
	{
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
				return runFrameTimeDetection(params.timestampDiff(), 0.00175);
			};
		if(!useRenderTaskTime)
		{
			screen()->addOnFrame(detectFrameRate);
		}
		else
		{
			appWindowData(window()).drawableHolder.addOnFrame(detectFrameRate);
			postDraw();
		}
		setCPUNeedsLowLatency(true);
	}
};

template <size_t S>
static void printFrameRateStr(char (&str)[S])
{
	string_printf(str, "Frame Rate: %.2fHz",
		EmuSystem::frameRate(EmuSystem::VIDSYS_NATIVE_NTSC));
}

template <size_t S>
static void printFrameRatePALStr(char (&str)[S])
{
	string_printf(str, "Frame Rate (PAL): %.2fHz",
		EmuSystem::frameRate(EmuSystem::VIDSYS_PAL));
}

#ifdef __ANDROID__
static bool setAndroidTextureStorage(uint8_t mode)
{
	static auto resetVideo =
		[]()
		{
			if(emuVideo.image())
			{
				// texture may switch to external format so
				// force effect shaders to re-compile
				emuVideoLayer.reset(optionImgEffect, optionImageEffectPixelFormatValue());
			}
		};
	if(!Gfx::Texture::setAndroidStorageImpl(emuVideo.renderer(), makeAndroidStorageImpl(mode)))
	{
		return false;
	}
	resetVideo();
	optionAndroidTextureStorage = mode;
	return true;
}
#endif

#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
static void setFrameInterval(int interval)
{
	optionFrameInterval = interval;
	logMsg("set frame interval: %d", int(optionFrameInterval));
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffect(uint val)
{
	optionImgEffect = val;
	if(emuVideo.image())
	{
		emuVideoLayer.setEffect(val, optionImageEffectPixelFormatValue());
		emuViewController.postDrawToEmuWindows();
	}
}
#endif

static void setOverlayEffect(uint val)
{
	optionOverlayEffect = val;
	emuVideoLayer.setOverlay(val);
	emuViewController.postDrawToEmuWindows();
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffectPixelFormat(PixelFormatID format)
{
	optionImageEffectPixelFormat = format;
	emuVideoLayer.setEffectFormat(format);
	emuViewController.postDrawToEmuWindows();
}
#endif

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
static const char *autoWindowPixelFormatStr()
{
	return Base::Window::defaultPixelFormat() == PIXEL_RGB565 ? "RGB565" : "RGBA8888";
}

static void setWindowPixelFormat(PixelFormatID format)
{
	EmuApp::postMessage("Restart app for option to take effect");
	optionWindowPixelFormat = format;
}
#endif

static void setGPUMultiThreading(Gfx::Renderer::ThreadMode mode)
{
	EmuApp::postMessage("Restart app for option to take effect");
	optionGPUMultiThreading = (int)mode;
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
	#ifdef __ANDROID__
	androidTextureStorageItem
	{
		{
			"Auto",
			[this]()
			{
				setAndroidTextureStorage(OPTION_ANDROID_TEXTURE_STORAGE_AUTO);
			}
		},
		{
			"Standard",
			[this]()
			{
				setAndroidTextureStorage(OPTION_ANDROID_TEXTURE_STORAGE_NONE);
			}
		},
		{
			"Graphic Buffer",
			[this](Input::Event e)
			{
				static auto setAndroidTextureStorageGraphicBuffer =
					[]()
					{
						if(!setAndroidTextureStorage(OPTION_ANDROID_TEXTURE_STORAGE_GRAPHIC_BUFFER))
						{
							EmuApp::postErrorMessage("Not supported on this GPU");
							return false;
						}
						return true;
					};
				if(!Gfx::Texture::isAndroidGraphicBufferStorageWhitelisted(renderer()))
				{
					auto ynAlertView = makeView<YesNoAlertView>(
						"Setting Graphic Buffer improves performance but may hang or crash "
						"the app depending on your device or GPU",
						"OK", "Cancel");
					ynAlertView->setOnYes(
						[this](TextMenuItem &, View &view, Input::Event e)
						{
							if(setAndroidTextureStorageGraphicBuffer())
							{
								androidTextureStorage.setSelected(2);
								view.dismiss();
								popAndShow();
							}
							else
							{
								view.dismiss();
							}
						});
					pushAndShowModal(std::move(ynAlertView), e);
					return false;
				}
				else
				{
					return setAndroidTextureStorageGraphicBuffer();
				}
			}
		},
		{
			"Surface Texture",
			[this]()
			{
				if(!setAndroidTextureStorage(OPTION_ANDROID_TEXTURE_STORAGE_SURFACE_TEXTURE))
				{
					EmuApp::postErrorMessage("Not supported on this GPU");
					return false;
				}
				return true;
			}
		}
	},
	androidTextureStorage
	{
		"GPU Copy Mode",
		[this](int idx) -> const char*
		{
			if(idx == 0)
			{
				return Gfx::Texture::androidStorageImplStr(renderer());
			}
			else
				return nullptr;
		},
		optionAndroidTextureStorage,
		[items = Base::androidSDK() >= 14 ? 4u : 3u](const MultiChoiceMenuItem &) -> int
		{
			return items;
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return androidTextureStorageItem[idx];
		}
	},
	#endif
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
		frameRateStr,
		[this](Input::Event e)
		{
			pushAndShowFrameRateSelectMenu(EmuSystem::VIDSYS_NATIVE_NTSC, e);
			postDraw();
		}
	},
	frameRatePAL
	{
		frameRatePALStr,
		[this](Input::Event e)
		{
			pushAndShowFrameRateSelectMenu(EmuSystem::VIDSYS_PAL, e);
			postDraw();
		}
	},
	aspectRatio
	{
		"Aspect Ratio",
		[this](uint32_t idx) -> const char*
		{
			if(idx == EmuSystem::aspectRatioInfos)
			{
				return aspectRatioStr;
			}
			return nullptr;
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
							popAndShow();
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
		[this](uint32_t idx) -> const char*
		{
			if(optionImageZoom <= 100)
				return zoomStr;
			else
				return nullptr;
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
							popAndShow();
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
		[this](uint32_t idx)
		{
			return viewportZoomStr;
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
			emuVideoLayer.setLinearFilter(optionImgFilter);
			emuViewController.postDrawToEmuWindows();
		}
	},
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffectItem
	{
		{"Off", [this]() { setImgEffect(0); }},
		{"hq2x", [this]() { setImgEffect(VideoImageEffect::HQ2X); }},
		{"Scale2x", [this]() { setImgEffect(VideoImageEffect::SCALE2X); }},
		{"Prescale 2x", [this]() { setImgEffect(VideoImageEffect::PRESCALE2X); }}
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
		{"Off", [this]() { setOverlayEffect(0); }},
		{"Scanlines", [this]() { setOverlayEffect(VideoImageOverlay::SCANLINES); }},
		{"Scanlines 2x", [this]() { setOverlayEffect(VideoImageOverlay::SCANLINES_2); }},
		{"CRT Mask", [this]() { setOverlayEffect(VideoImageOverlay::CRT); }},
		{"CRT", [this]() { setOverlayEffect(VideoImageOverlay::CRT_RGB); }},
		{"CRT 2x", [this]() { setOverlayEffect(VideoImageOverlay::CRT_RGB_2); }}
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
							popAndShow();
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
		[this](uint32_t idx)
		{
			return overlayEffectLevelStr;
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
		{"Auto (Match render format as needed)", [this]() { setImgEffectPixelFormat(PIXEL_NONE); }},
		{"RGB565", [this]() { setImgEffectPixelFormat(PIXEL_RGB565); }},
		{"RGBA8888", [this]() { setImgEffectPixelFormat(PIXEL_RGBA8888);}},
	},
	imgEffectPixelFormat
	{
		"Effect Color Format",
		[](int idx) -> const char*
		{
			if(idx == 0)
				return "Auto";
			else
				return nullptr;
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
		[](int idx) -> const char*
		{
			if(idx == 0)
			{
				return autoWindowPixelFormatStr();
			}
			else
				return nullptr;
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
			emuViewController.setEmuViewOnExtraWindow(item.flipBoolValue(*this), *Base::Screen::screen(0));
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
			if(Base::Screen::screens() > 1)
				emuViewController.setEmuViewOnExtraWindow(optionShowOnSecondScreen, *Base::Screen::screen(1));
		}
	},
	#endif
	gpuMultithreadingItem
	{
		{"Auto", [this]() { setGPUMultiThreading(Gfx::Renderer::ThreadMode::AUTO); }},
		{"Off", [this]() { setGPUMultiThreading(Gfx::Renderer::ThreadMode::SINGLE); }},
		{"On", [this]() { setGPUMultiThreading(Gfx::Renderer::ThreadMode::MULTI); }},
	},
	gpuMultithreading
	{
		"Render Multithreading",
		[this](int idx) -> const char*
		{
			if(idx == 0)
			{
				return renderer().threadMode() == Gfx::Renderer::ThreadMode::MULTI ? "On" : "Off";
			}
			else
				return nullptr;
		},
		[]()
		{
			switch(optionGPUMultiThreading.val)
			{
				default: return 0;
				case (int)Gfx::Renderer::ThreadMode::SINGLE: return 1;
				case (int)Gfx::Renderer::ThreadMode::MULTI: return 2;
			}
		}(),
		gpuMultithreadingItem
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
						popAndShow();
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
		printFrameRateStr(frameRateStr);
		item.emplace_back(&frameRate);
	}
	if(!optionFrameRatePAL.isConst)
	{
		printFrameRatePALStr(frameRatePALStr);
		item.emplace_back(&frameRatePAL);
	}
	item.emplace_back(&visualsHeading);
	item.emplace_back(&imgFilter);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	item.emplace_back(&imgEffect);
	#endif
	item.emplace_back(&overlayEffect);
	string_printf(overlayEffectLevelStr, "%u%%", optionOverlayEffectLevel.val);
	item.emplace_back(&overlayEffectLevel);
	item.emplace_back(&screenShapeHeading);
	string_printf(zoomStr, "%u%%", optionImageZoom.val);
	item.emplace_back(&zoom);
	string_printf(viewportZoomStr, "%u%%", optionViewportZoom.val);
	item.emplace_back(&viewportZoom);
	string_printf(aspectRatioStr, "%.2f", optionAspectRatio.val);
	item.emplace_back(&aspectRatio);
	item.emplace_back(&advancedHeading);
	#ifdef CONFIG_BASE_ANDROID
	item.emplace_back(&androidTextureStorage);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	item.emplace_back(&imgEffectPixelFormat);
	#endif
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	item.emplace_back(&windowPixelFormat);
	#endif
	if(renderer().supportsThreadMode())
		item.emplace_back(&gpuMultithreading);
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

bool VideoOptionView::onFrameTimeChange(EmuSystem::VideoSystem vidSys, IG::FloatSeconds time)
{
	auto wantedTime = time;
	if(!time.count())
	{
		wantedTime = emuViewController.emuWindowScreen()->frameTime();
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
		printFrameRateStr(frameRateStr);
		frameRate.compile(renderer(), projP);
	}
	else
	{
		optionFrameRatePAL = time.count();
		printFrameRatePALStr(frameRatePALStr);
		frameRatePAL.compile(renderer(), projP);
	}
	return true;
}

void VideoOptionView::pushAndShowFrameRateSelectMenu(EmuSystem::VideoSystem vidSys, Input::Event e)
{
	const bool includeFrameRateDetection = !Config::envIsIOS;
	auto multiChoiceView = makeViewWithName<TextTableView>("Frame Rate", includeFrameRateDetection ? 4 : 3);
	multiChoiceView->appendItem("Set with screen's reported rate",
		[this, vidSys](Input::Event e)
		{
			if(!emuViewController.emuWindowScreen()->frameRateIsReliable())
			{
				#ifdef __ANDROID__
				if(Base::androidSDK() <= 10)
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
				popAndShow();
		});
	multiChoiceView->appendItem("Set default rate",
		[this, vidSys](Input::Event e)
		{
			onFrameTimeChange(vidSys, EmuSystem::defaultFrameTime(vidSys));
			popAndShow();
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
						popAndShow();
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
				auto frView = makeView<DetectFrameRateView>();
				frView->onDetectFrameTime =
					[this, vidSys](IG::FloatSeconds frameTime)
					{
						if(frameTime.count())
						{
							onFrameTimeChange(vidSys, frameTime);
						}
						else
						{
							EmuApp::postErrorMessage("Detected rate too unstable to use");
						}
					};
				popAndShow();
				pushAndShowModal(std::move(frView), e);
			});
	}
	pushAndShow(std::move(multiChoiceView), e);
}

void VideoOptionView::setZoom(uint8_t val)
{
	string_printf(zoomStr, "%u%%", val);
	optionImageZoom = val;
	logMsg("set image zoom: %d", int(optionImageZoom));
	emuViewController.placeEmuViews();
	emuViewController.postDrawToEmuWindows();
}

void VideoOptionView::setViewportZoom(uint8_t val)
{
	string_printf(viewportZoomStr, "%u%%", val);
	optionViewportZoom = val;
	logMsg("set viewport zoom: %d", int(optionViewportZoom));
	emuViewController.startMainViewportAnimation();
}

void VideoOptionView::setOverlayEffectLevel(uint8_t val)
{
	string_printf(overlayEffectLevelStr, "%u%%", val);
	optionOverlayEffectLevel = val;
	emuVideoLayer.setOverlayIntensity(val/100.);
	emuViewController.postDrawToEmuWindows();
}

void VideoOptionView::setAspectRatio(double val)
{
	string_printf(aspectRatioStr, "%.2f", val);
	optionAspectRatio = val;
	logMsg("set aspect ratio: %.2f", val);
	emuViewController.placeEmuViews();
	emuViewController.postDrawToEmuWindows();
}

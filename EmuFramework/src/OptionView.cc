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
#include <emuframework/FilePicker.hh>
#include <imagine/base/Base.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>
#include <algorithm>
#include "private.hh"

using namespace IG;

static constexpr bool USE_MOBILE_ORIENTATION_NAMES = Config::envIsAndroid || Config::envIsIOS;
static const char *landscapeName = USE_MOBILE_ORIENTATION_NAMES ? "Landscape" : "90 Left";
static const char *landscape2Name = USE_MOBILE_ORIENTATION_NAMES ? "Landscape 2" : "90 Right";
static const char *portraitName = USE_MOBILE_ORIENTATION_NAMES ? "Portrait" : "Standard";
static const char *portrait2Name = USE_MOBILE_ORIENTATION_NAMES ? "Portrait 2" : "Upside Down";

static FS::PathString savePathStrToDescStr(char *savePathStr)
{
	FS::PathString desc{};
	if(strlen(savePathStr))
	{
		if(string_equal(savePathStr, optionSavePathDefaultToken))
			string_copy(desc, "Default");
		else
		{
			string_copy(desc, FS::basename(optionSavePath).data());
		}
	}
	else
	{
		string_copy(desc, "Same as Game");
	}
	return desc;
}

BiosSelectMenu::BiosSelectMenu(const char *name, ViewAttachParams attach, FS::PathString *biosPathStr_, BiosChangeDelegate onBiosChange_,
	EmuSystem::NameFilterFunc fsFilter_):
	TableView
	{
		name,
		attach,
		[this](const TableView &)
		{
			return 2;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return selectFile;
				default: return unset;
			}
		}
	},
	selectFile
	{
		"Select File",
		[this](Input::Event e)
		{
			auto startPath = strlen(biosPathStr->data()) ? FS::dirname(*biosPathStr) : lastLoadPath;
			auto fPicker = makeView<EmuFilePicker>(startPath.data(), false, fsFilter, FS::RootPathInfo{}, e);
			fPicker->setOnSelectFile(
				[this](FSPicker &picker, const char* name, Input::Event e)
				{
					*biosPathStr = picker.makePathString(name);
					picker.dismiss();
					onBiosChangeD.callSafe();
					popAndShow();
				});
			emuViewController.pushAndShowModal(std::move(fPicker), e, false);
		}
	},
	unset
	{
		"Unset",
		[this]()
		{
			strcpy(biosPathStr->data(), "");
			auto onBiosChange = onBiosChangeD;
			popAndShow();
			onBiosChange.callSafe();
		}
	},
	onBiosChangeD{onBiosChange_},
	biosPathStr{biosPathStr_},
	fsFilter{fsFilter_}
{
	assert(biosPathStr);
}

static void setAutoSaveState(uint val)
{
	optionAutoSaveState = val;
	logMsg("set auto-savestate %d", optionAutoSaveState.val);
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
				emuVideoLayer.reset();
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

static void setAudioRate(uint32_t rate)
{
	if(rate > (uint32_t)IG::AudioManager::nativeFormat().rate)
		return;
	optionSoundRate = rate;
	EmuSystem::configAudioPlayback(rate);
}

static void setMenuOrientation(uint val, Base::Window &win)
{
	optionMenuOrientation = val;
	emuVideo.renderer().setWindowValidOrientations(win, optionMenuOrientation);
	logMsg("set menu orientation: %s", Base::orientationToStr(int(optionMenuOrientation)));
}

static void setGameOrientation(uint val)
{
	optionGameOrientation = val;
	logMsg("set game orientation: %s", Base::orientationToStr(int(optionGameOrientation)));
}

static void setSoundBuffers(int val)
{
	emuAudio.close();
	optionSoundBuffers = val;
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

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffect(uint val)
{
	optionImgEffect = val;
	if(emuVideo.image())
	{
		emuVideoLayer.setEffect(val);
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

void VideoOptionView::setOverlayEffectLevel(uint8_t val)
{
	string_printf(overlayEffectLevelStr, "%u%%", val);
	optionOverlayEffectLevel = val;
	emuVideoLayer.setOverlayIntensity(val/100.);
	emuViewController.postDrawToEmuWindows();
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static const char *autoImgEffectPixelFormatStr()
{
	return "RGB565";
}

static void setImgEffectPixelFormat(PixelFormatID format)
{
	optionImageEffectPixelFormat = format;
	emuVideoLayer.setEffectBitDepth(format == PIXEL_RGBA8888 ? 32 : 16);
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

void GUIOptionView::setFontSize(uint16_t val)
{
	string_printf(fontSizeStr, "%.2f", val / 1000.);
	optionFontSize = val;
	setupFont(renderer(), window());
	emuViewController.placeElements();
}

template <size_t S>
static void printPathMenuEntryStr(PathOption optionSavePath, char (&str)[S])
{
	string_printf(str, "Save Path: %s", savePathStrToDescStr(optionSavePath).data());
}

class DetectFrameRateView : public View
{
public:
	using DetectFrameRateDelegate = DelegateFunc<void (IG::FloatSeconds frameTime)>;
	DetectFrameRateDelegate onDetectFrameTime;
	Base::Screen::OnFrameDelegate detectFrameRate;
	Gfx::DrawFinishedDelegate detectFrameDrawRate;
	Base::FrameTime totalFrameTime{};
	Gfx::Text fpsText;
	uint allTotalFrames = 0;
	uint callbacks = 0;
	std::array<char, 32> fpsStr{};
	std::vector<Base::FrameTime> frameTimeSample{};
	Gfx::RendererTask &rendererTask;

	DetectFrameRateView(ViewAttachParams attach, Gfx::RendererTask &rendererTask): View(attach),
		fpsText{nullptr, &View::defaultFace}, rendererTask{rendererTask}
	{
		View::defaultFace.precacheAlphaNum(attach.renderer());
		View::defaultFace.precache(attach.renderer(), ".");
		fpsText.setString("Preparing to detect frame rate...");
	}

	~DetectFrameRateView() final
	{
		setCPUNeedsLowLatency(false);
		emuViewController.emuWindowScreen()->removeOnFrame(detectFrameRate);
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
		postDraw();
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
			return true;
		}
	}

	void onAddedToController(Input::Event e) final
	{
		auto screen = emuViewController.emuWindowScreen();
		assumeExpr(screen);
		frameTimeSample.reserve(std::round(screen->frameRate() * 2.));
		if(screen->supportsTimestamps())
		{
			logMsg("detecting via frame timestamps");
			detectFrameRate =
				[this](Base::Screen::FrameParams params)
				{
					postDraw();
					const uint callbacksToSkip = 8;
					callbacks++;
					if(callbacks >= callbacksToSkip)
					{
						detectFrameRate =
							[this](Base::Screen::FrameParams params)
							{
								return runFrameTimeDetection(params.timestampDiff(), 0.00001);
							};
						params.screen().addOnFrame(detectFrameRate);
						return false;
					}
					else
					{
						return true;
					}
				};
			screen->addOnFrame(detectFrameRate);
		}
		else
		{
			logMsg("detecting via draw finished timestamps");
			detectFrameDrawRate =
				[this](Gfx::DrawFinishedParams params)
				{
					postDraw();
					const uint callbacksToSkip = 10;
					callbacks++;
					if(callbacks >= callbacksToSkip)
					{
						detectFrameDrawRate =
							[this](Gfx::DrawFinishedParams params)
							{
								return runFrameTimeDetection(params.timestampDiff(), 0.001);
							};
						params.rendererTask().addOnDrawFinished(detectFrameDrawRate);
						postDraw();
						return false;
					}
					else
					{
						return true;
					}
				};
			rendererTask.addOnDrawFinished(detectFrameDrawRate);
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
	item.emplace_back(&aspectRatio);
	item.emplace_back(&advancedHeading);
	#ifdef CONFIG_BASE_ANDROID
	if(!Config::MACHINE_IS_OUYA)
	{
		item.emplace_back(&androidTextureStorage);
	}
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

void AudioOptionView::loadStockItems()
{
	item.emplace_back(&snd);
	if(!optionSoundRate.isConst)
	{
		audioRateItem.clear();
		audioRateItem.emplace_back("Device Native",
			[this](TextMenuItem &, View &parent, Input::Event)
			{
				setAudioRate(IG::AudioManager::nativeFormat().rate);
				updateAudioRateItem();
				parent.dismiss();
				return false;
			});
		audioRateItem.emplace_back("22KHz", [this]() { setAudioRate(22050); });
		audioRateItem.emplace_back("32KHz", [this]() { setAudioRate(32000); });
		audioRateItem.emplace_back("44KHz", [this]() { setAudioRate(44100); });
		if(IG::AudioManager::nativeFormat().rate >= 48000)
			audioRateItem.emplace_back("48KHz", [this]() { setAudioRate(48000); });
		item.emplace_back(&audioRate);
		updateAudioRateItem();
	}
	item.emplace_back(&soundBuffers);
	item.emplace_back(&addSoundBuffersOnUnderrun);
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	item.emplace_back(&audioSoloMix);
	#endif
}

void AudioOptionView::updateAudioRateItem()
{
	switch(optionSoundRate)
	{
		bcase 22050: audioRate.setSelected(1);
		bcase 32000: audioRate.setSelected(2);
		bdefault: audioRate.setSelected(3); // 44100
		bcase 48000: audioRate.setSelected(4);
	}
}

void SystemOptionView::loadStockItems()
{
	item.emplace_back(&autoSaveState);
	item.emplace_back(&confirmAutoLoadState);
	item.emplace_back(&confirmOverwriteState);
	printPathMenuEntryStr(optionSavePath, savePathStr);
	item.emplace_back(&savePath);
	item.emplace_back(&checkSavePathWriteAccess);
	item.emplace_back(&fastForwardSpeed);
	#ifdef __ANDROID__
	item.emplace_back(&processPriority);
	if(!optionSustainedPerformanceMode.isConst)
		item.emplace_back(&performanceMode);
	#endif
}

void GUIOptionView::loadStockItems()
{
	if(!optionPauseUnfocused.isConst)
	{
		item.emplace_back(&pauseUnfocused);
	}
	if(!optionNotificationIcon.isConst)
	{
		item.emplace_back(&notificationIcon);
	}
	if(!optionTitleBar.isConst)
	{
		item.emplace_back(&navView);
	}
	if(!View::needsBackControlIsConst)
	{
		item.emplace_back(&backNav);
	}
	item.emplace_back(&systemActionsIsDefaultMenu);
	if(!optionFontSize.isConst)
	{
		string_printf(fontSizeStr, "%.2f", optionFontSize / 1000.);
		item.emplace_back(&fontSize);
	}
	if(!optionIdleDisplayPowerSave.isConst)
	{
		item.emplace_back(&idleDisplayPowerSave);
	}
	if(!optionLowProfileOSNav.isConst)
	{
		item.emplace_back(&lowProfileOSNav);
	}
	if(!optionHideOSNav.isConst)
	{
		item.emplace_back(&hideOSNav);
	}
	if(!optionHideStatusBar.isConst)
	{
		item.emplace_back(&statusBar);
	}
	if(EmuSystem::hasBundledGames)
	{
		item.emplace_back(&showBundledGames);
	}
	#ifdef CONFIG_BLUETOOTH
	item.emplace_back(&showBluetoothScan);
	#endif
	if(!optionGameOrientation.isConst)
	{
		item.emplace_back(&orientationHeading);
		item.emplace_back(&gameOrientation);
		item.emplace_back(&menuOrientation);
	}
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
					emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
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
		0,
		[](const MultiChoiceMenuItem &) -> int
		{
			return EmuSystem::aspectRatioInfos;
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return aspectRatioItem[idx];
		}
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
		{"Auto", [this]() { setImgEffectPixelFormat(PIXEL_NONE); }},
		{"RGB565", [this]() { setImgEffectPixelFormat(PIXEL_RGB565); }},
		{"RGBA8888", [this]() { setImgEffectPixelFormat(PIXEL_RGBA8888);}},
	},
	imgEffectPixelFormat
	{
		"Effect Color Format",
		[](int idx) -> const char*
		{
			if(idx == 0)
			{
				return autoImgEffectPixelFormatStr();
			}
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
		aspectRatioItem[i] = {EmuSystem::aspectRatioInfo[i].name,
			[this, i]()
			{
				optionAspectRatio.val = EmuSystem::aspectRatioInfo[i].aspect;
				logMsg("set aspect ratio: %u:%u", optionAspectRatio.val.x, optionAspectRatio.val.y);
				emuViewController.placeEmuViews();
				emuViewController.postDrawToEmuWindows();
			}};
		if(optionAspectRatio == EmuSystem::aspectRatioInfo[i].aspect)
		{
			aspectRatio.setSelected(i, *this);
		}
	}
	if(!customMenu)
	{
		loadStockItems();
	}
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
				auto frView = makeView<DetectFrameRateView>(emuViewController.rendererTask());
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
				emuViewController.pushAndShowModal(std::move(frView), e, false);
			});
	}
	pushAndShow(std::move(multiChoiceView), e);
}

AudioOptionView::AudioOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Audio Options", attach, item},
	snd
	{
		"Sound",
		(bool)optionSound,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSound = item.flipBoolValue(*this);
			if(!optionSound)
				emuAudio.close();
		}
	},
	soundBuffersItem
	{
		{"2", [this]() { setSoundBuffers(2); }},
		{"3", [this]() { setSoundBuffers(3); }},
		{"4", [this]() { setSoundBuffers(4); }},
		{"5", [this]() { setSoundBuffers(5); }},
		{"6", [this]() { setSoundBuffers(6); }},
		{"7", [this]() { setSoundBuffers(7); }},
		{"8", [this]() { setSoundBuffers(8); }},
	},
	soundBuffers
	{
		"Buffer Size In Frames",
		(int)optionSoundBuffers - 2,
		[this](const MultiChoiceMenuItem &) -> int
		{
			return std::size(soundBuffersItem);
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return soundBuffersItem[idx];
		}
	},
	addSoundBuffersOnUnderrun
	{
		"Auto-increase Buffer Size",
		(bool)optionAddSoundBuffersOnUnderrun,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionAddSoundBuffersOnUnderrun = item.flipBoolValue(*this);
			emuAudio.setAddSoundBuffersOnUnderrun(optionAddSoundBuffersOnUnderrun);
		}
	},
	audioRate
	{
		"Sound Rate",
		0,
		audioRateItem
	}
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	,audioSoloMix
	{
		"Mix With Other Apps",
		!optionAudioSoloMix,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionAudioSoloMix = !item.flipBoolValue(*this);
		}
	}
	#endif
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

SystemOptionView::SystemOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"System Options", attach, item},
	autoSaveStateItem
	{
		{"Off", [this]() { setAutoSaveState(0); }},
		{"Game Exit", [this]() { setAutoSaveState(1); }},
		{"15mins", [this]() { setAutoSaveState(15); }},
		{"30mins", [this]() { setAutoSaveState(30); }}
	},
	autoSaveState
	{
		"Auto-save State",
		[]()
		{
			switch(optionAutoSaveState.val)
			{
				default: return 0;
				case 1: return 1;
				case 15: return 2;
				case 30: return 3;
			}
		}(),
		autoSaveStateItem
	},
	confirmAutoLoadState
	{
		"Confirm Auto-load State",
		(bool)optionConfirmAutoLoadState,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConfirmAutoLoadState = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State",
		(bool)optionConfirmOverwriteState,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConfirmOverwriteState = item.flipBoolValue(*this);
		}
	},
	savePath
	{
		savePathStr,
		[this](TextMenuItem &, View &view, Input::Event e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Save Path", 3);
			multiChoiceView->appendItem("Set Custom Path",
				[this](Input::Event e)
				{
					auto startPath = strlen(optionSavePath) ? optionSavePath : optionLastLoadPath;
					auto fPicker = makeView<EmuFilePicker>(startPath, true,
						EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
					fPicker->setOnClose(
						[this](FSPicker &picker, Input::Event e)
						{
							EmuSystem::savePath_ = picker.path();
							logMsg("set save path %s", (char*)optionSavePath);
							onSavePathChange(optionSavePath);
							picker.dismiss();
							popAndShow();
						});
					emuViewController.pushAndShowModal(std::move(fPicker), e, false);
				});
			multiChoiceView->appendItem("Same as Game",
				[this]()
				{
					popAndShow();
					strcpy(optionSavePath, "");
					onSavePathChange("");
				});
			multiChoiceView->appendItem("Default",
				[this]()
				{
					popAndShow();
					strcpy(optionSavePath, optionSavePathDefaultToken);
					onSavePathChange(optionSavePathDefaultToken);
				});
			pushAndShow(std::move(multiChoiceView), e);
			postDraw();
		}
	},
	checkSavePathWriteAccess
	{
		"Check Save Path Write Access",
		(bool)optionCheckSavePathWriteAccess,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionCheckSavePathWriteAccess = item.flipBoolValue(*this);
		}
	},
	fastForwardSpeedItem
	{
		{"2x", [this]() { optionFastForwardSpeed = 2; }},
		{"3x", [this]() { optionFastForwardSpeed = 3; }},
		{"4x", [this]() { optionFastForwardSpeed = 4; }},
		{"5x", [this]() { optionFastForwardSpeed = 5; }},
		{"6x", [this]() { optionFastForwardSpeed = 6; }},
		{"7x", [this]() { optionFastForwardSpeed = 7; }},
	},
	fastForwardSpeed
	{
		"Fast Forward Speed",
		[]() -> int
		{
			if(optionFastForwardSpeed >= MIN_FAST_FORWARD_SPEED && optionFastForwardSpeed <= 7)
			{
				return optionFastForwardSpeed - MIN_FAST_FORWARD_SPEED;
			}
			return 0;
		}(),
		fastForwardSpeedItem
	}
	#if defined __ANDROID__
	,processPriorityItem
	{
		{
			"Normal",
			[this]()
			{
				optionProcessPriority = 0;
				Base::setProcessPriority(optionProcessPriority);
			}
		},
		{
			"High",
			[this]()
			{
				optionProcessPriority = -6;
				Base::setProcessPriority(optionProcessPriority);
			}
		},
		{
			"Very High",
			[this]()
			{
				optionProcessPriority = -14;
				Base::setProcessPriority(optionProcessPriority);
			}
		}
	},
	processPriority
	{
		"Process Priority",
		[]()
		{
			switch(optionProcessPriority.val)
			{
				default: return 0;
				case -6: return 1;
				case -14: return 2;
			}
		}(),
		processPriorityItem
	},
	performanceMode
	{
		"Performance Mode",
		(bool)optionSustainedPerformanceMode,
		"Normal", "Sustained",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSustainedPerformanceMode = item.flipBoolValue(*this);
		}
	}
	#endif
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void SystemOptionView::onSavePathChange(const char *path)
{
	if(string_equal(path, optionSavePathDefaultToken))
	{
		auto defaultPath = EmuSystem::baseDefaultGameSavePath();
		EmuApp::printfMessage(4, false, "Default Save Path:\n%s", defaultPath.data());
	}
	printPathMenuEntryStr(optionSavePath, savePathStr);
	savePath.compile(renderer(), projP);
	EmuSystem::setupGameSavePath();
	EmuSystem::savePathChanged();
}

void SystemOptionView::onFirmwarePathChange(const char *path, Input::Event e) {}

void SystemOptionView::pushAndShowFirmwarePathMenu(const char *name, Input::Event e)
{
	auto multiChoiceView = std::make_unique<TextTableView>(name, attachParams(), 2);
	multiChoiceView->appendItem("Set Custom Path",
		[this](Input::Event e)
		{
			auto startPath =  EmuApp::firmwareSearchPath();
			auto fPicker = makeView<EmuFilePicker>(startPath.data(), true,
				EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
			fPicker->setOnClose(
				[this](FSPicker &picker, Input::Event e)
				{
					auto path = picker.path();
					EmuApp::setFirmwareSearchPath(path.data());
					logMsg("set firmware path %s", path.data());
					onFirmwarePathChange(path.data(), e);
					picker.dismiss();
				});
			popAndShow();
			EmuApp::pushAndShowModalView(std::move(fPicker), e);
		});
	multiChoiceView->appendItem("Default",
		[this](Input::Event e)
		{
			popAndShow();
			EmuApp::setFirmwareSearchPath("");
			onFirmwarePathChange("", e);
		});
	pushAndShow(std::move(multiChoiceView), e);
}

GUIOptionView::GUIOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"GUI Options", attach, item},
	pauseUnfocused
	{
		Config::envIsPS3 ? "Pause in XMB" : "Pause if unfocused",
		(bool)optionPauseUnfocused,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionPauseUnfocused = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2", [this](){ setFontSize(2000); }},
		{"3", [this]() { setFontSize(3000); }},
		{"4", [this]() { setFontSize(4000); }},
		{"5", [this]() { setFontSize(5000); }},
		{"6", [this]() { setFontSize(6000); }},
		{"7", [this]() { setFontSize(7000); }},
		{"8", [this]() { setFontSize(8000); }},
		{"9", [this]() { setFontSize(9000); }},
		{"10", [this]() { setFontSize(10000); }},
		{"Custom Value",
			[this](Input::Event e)
			{
				EmuApp::pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](auto val)
					{
						int scaledIntVal = val * 1000.0;
						if(optionFontSize.isValidVal(scaledIntVal))
						{
							setFontSize(scaledIntVal);
							fontSize.setSelected(std::size(fontSizeItem) - 1, *this);
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
	fontSize
	{
		"Font Size",
		[this](uint32_t idx)
		{
			return fontSizeStr;
		},
		[]()
		{
			switch(optionFontSize)
			{
				case 2000: return 0;
				case 3000: return 1;
				case 4000: return 2;
				case 5000: return 3;
				case 6000: return 4;
				case 7000: return 5;
				case 8000: return 6;
				case 9000: return 7;
				case 10000: return 8;
				default: return 9;
			}
		}(),
		fontSizeItem
	},
	notificationIcon
	{
		"Suspended App Icon",
		(bool)optionNotificationIcon,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionNotificationIcon = item.flipBoolValue(*this);
		}
	},
	statusBarItem
	{
		{
			"Off",
			[this]()
			{
				optionHideStatusBar = 0;
				applyOSNavStyle(false);
			}
		},
		{
			"In Game",
			[this]()
			{
				optionHideStatusBar = 1;
				applyOSNavStyle(false);
			}
		},
		{
			"On",
			[this]()
			{
				optionHideStatusBar = 2;
				applyOSNavStyle(false);
			}
		}
	},
	statusBar
	{
		"Hide Status Bar",
		optionHideStatusBar,
		statusBarItem
	},
	lowProfileOSNavItem
	{
		{
			"Off",
			[this]()
			{
				optionLowProfileOSNav = 0;
				applyOSNavStyle(false);
			}
		},
		{
			"In Game",
			[this]()
			{
				optionLowProfileOSNav = 1;
				applyOSNavStyle(false);
			}
		},
		{
			"On",
			[this]()
			{
				optionLowProfileOSNav = 2;
				applyOSNavStyle(false);
			}
		}
	},
	lowProfileOSNav
	{
		"Dim OS UI",
		optionLowProfileOSNav,
		lowProfileOSNavItem
	},
	hideOSNavItem
	{
		{
			"Off",
			[this]()
			{
				optionHideOSNav = 0;
				applyOSNavStyle(false);
			}
		},
		{
			"In Game",
			[this]()
			{
				optionHideOSNav = 1;
				applyOSNavStyle(false);
			}
		},
		{
			"On",
			[this]()
			{
				optionHideOSNav = 2;
				applyOSNavStyle(false);
			}
		}
	},
	hideOSNav
	{
		"Hide OS Navigation",
		optionHideOSNav,
		hideOSNavItem
	},
	idleDisplayPowerSave
	{
		"Dim Screen If Idle",
		(bool)optionIdleDisplayPowerSave,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionIdleDisplayPowerSave = item.flipBoolValue(*this);
			Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
		}
	},
	navView
	{
		"Title Bar",
		(bool)optionTitleBar,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionTitleBar = item.flipBoolValue(*this);
			emuViewController.showNavView(optionTitleBar);
			emuViewController.placeElements();
		}
	},
	backNav
	{
		"Title Back Navigation",
		View::needsBackControl,
		[this](BoolMenuItem &item, Input::Event e)
		{
			View::setNeedsBackControl(item.flipBoolValue(*this));
			emuViewController.setShowNavViewBackButton(View::needsBackControl);
			emuViewController.placeElements();
		}
	},
	systemActionsIsDefaultMenu
	{
		"Default Menu",
		(bool)optionSystemActionsIsDefaultMenu,
		"Last Used", "System Actions",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSystemActionsIsDefaultMenu = item.flipBoolValue(*this);
		}
	},
	showBundledGames
	{
		"Show Bundled Games",
		(bool)optionShowBundledGames,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowBundledGames = item.flipBoolValue(*this);
			onMainMenuItemOptionChanged();
		}
	},
	showBluetoothScan
	{
		"Show Bluetooth Menu Items",
		(bool)optionShowBluetoothScan,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowBluetoothScan = item.flipBoolValue(*this);
			onMainMenuItemOptionChanged();
		}
	},
	orientationHeading
	{
		"Orientation",
	},
	menuOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", [this](){ setMenuOrientation(Base::VIEW_ROTATE_AUTO, window()); }},
		#endif
		{landscapeName, [this](){ setMenuOrientation(Base::VIEW_ROTATE_90, window()); }},
		{landscape2Name, [this](){ setMenuOrientation(Base::VIEW_ROTATE_270, window()); }},
		{portraitName, [this](){ setMenuOrientation(Base::VIEW_ROTATE_0, window()); }},
		{portrait2Name, [this](){ setMenuOrientation(Base::VIEW_ROTATE_180, window()); }},
	},
	menuOrientation
	{
		"In Menu",
		[]()
		{
			int itemOffset = Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 0 : 1;
			switch(optionMenuOrientation)
			{
				default: return 0;
				case Base::VIEW_ROTATE_90: return 1 - itemOffset;
				case Base::VIEW_ROTATE_270: return 2 - itemOffset;
				case Base::VIEW_ROTATE_0: return 3 - itemOffset;
				case Base::VIEW_ROTATE_180: return 4 - itemOffset;
			}
		}(),
		menuOrientationItem
	},
	gameOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", [](){ setGameOrientation(Base::VIEW_ROTATE_AUTO); }},
		#endif
		{landscapeName, [](){ setGameOrientation(Base::VIEW_ROTATE_90); }},
		{landscape2Name, [](){ setGameOrientation(Base::VIEW_ROTATE_270); }},
		{portraitName, [](){ setGameOrientation(Base::VIEW_ROTATE_0); }},
		{portrait2Name, [](){ setGameOrientation(Base::VIEW_ROTATE_180); }},
	},
	gameOrientation
	{
		"In Game",
		[]()
		{
			int itemOffset = Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 0 : 1;
			switch(optionGameOrientation)
			{
				default: return 0;
				case Base::VIEW_ROTATE_90: return 1 - itemOffset;
				case Base::VIEW_ROTATE_270: return 2 - itemOffset;
				case Base::VIEW_ROTATE_0: return 3 - itemOffset;
				case Base::VIEW_ROTATE_180: return 4 - itemOffset;
			}
		}(),
		gameOrientationItem
	}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

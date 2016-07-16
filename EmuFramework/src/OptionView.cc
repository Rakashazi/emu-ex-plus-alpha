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
#include <emuframework/FilePicker.hh>
#include <imagine/gui/TextEntry.hh>
#include <algorithm>

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

BiosSelectMenu::BiosSelectMenu(const char *name, FS::PathString *biosPathStr_, BiosChangeDelegate onBiosChange_,
	EmuSystem::NameFilterFunc fsFilter_, Base::Window &win):
	TableView
	{
		name,
		win,
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
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto startPath = strlen(biosPathStr->data()) ? FS::dirname(*biosPathStr) : lastLoadPath;
			auto &fPicker = *new EmuFilePicker{window(), startPath.data(), false, fsFilter};
			fPicker.setOnSelectFile(
				[this](FSPicker &picker, const char* name, Input::Event e)
				{
					onSelectFile(picker.makePathString(name), e);
					picker.dismiss();
				});
			modalViewController.pushAndShow(fPicker, e);
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

void BiosSelectMenu::onSelectFile(FS::PathString path, Input::Event e)
{
	*biosPathStr = path;
	onBiosChangeD.callSafe();
	viewStack.popAndShow();
}

static void setAutoSaveState(uint val)
{
	optionAutoSaveState = val;
	logMsg("set auto-savestate %d", optionAutoSaveState.val);
}

#ifdef __ANDROID__
static bool setAndroidTextureStorage(uint8 mode)
{
	using namespace Gfx;
	static auto resetVideo =
		[]()
		{
			if(emuVideo.vidImg)
			{
				// texture may switch to external format so
				// force effect shaders to re-compile
				emuVideoLayer.setEffect(0);
				emuVideo.reinitImage();
				emuVideo.clearImage();
				#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
				emuVideoLayer.setEffect(optionImgEffect);
				#endif
			}
		};
	if(!Gfx::Texture::setAndroidStorageImpl(makeAndroidStorageImpl(mode)))
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

static void setAudioRate(uint rate)
{
	optionSoundRate = rate;
	EmuSystem::configAudioPlayback();
}

static void setMenuOrientation(uint val)
{
	optionMenuOrientation = val;
	Gfx::setWindowValidOrientations(mainWin.win, optionMenuOrientation);
	logMsg("set menu orientation: %s", Base::orientationToStr(int(optionMenuOrientation)));
}

static void setGameOrientation(uint val)
{
	optionGameOrientation = val;
	logMsg("set game orientation: %s", Base::orientationToStr(int(optionGameOrientation)));
}

#ifdef CONFIG_AUDIO_LATENCY_HINT
static void setSoundBuffers(int val)
{
	if(Audio::isOpen())
		Audio::closePcm();
	optionSoundBuffers = val;
}
#endif

static void setZoom(int val)
{
	optionImageZoom = val;
	logMsg("set image zoom: %d", int(optionImageZoom));
	placeEmuViews();
	emuWin->win.postDraw();
}

static void setViewportZoom(int val)
{
	optionViewportZoom = val;
	logMsg("set viewport zoom: %d", int(optionViewportZoom));
	startViewportAnimation(mainWin);
	mainWin.win.postDraw();
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffect(uint val)
{
	optionImgEffect = val;
	if(emuVideo.vidImg)
	{
		emuVideoLayer.setEffect(val);
		emuWin->win.postDraw();
	}
}
#endif

static void setOverlayEffect(uint val)
{
	optionOverlayEffect = val;
	emuVideoLayer.vidImgOverlay.setEffect(val);
	emuVideoLayer.placeOverlay();
	emuWin->win.postDraw();
}

static void setOverlayEffectLevel(uint val)
{
	optionOverlayEffectLevel = val;
	emuVideoLayer.vidImgOverlay.intensity = val/100.;
	emuWin->win.postDraw();
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void setImgEffectPixelFormat(PixelFormatID format)
{
	if(format == PIXEL_NONE)
		popup.printf(3, false, "Set RGB565 mode via Auto");
	optionImageEffectPixelFormat = format;
	emuVideoLayer.vidImgEffect.setBitDepth(format == PIXEL_RGBA8888 ? 32 : 16);
	emuWin->win.postDraw();
}
#endif

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
static void setWindowPixelFormat(PixelFormatID format)
{
	if(format == PIXEL_NONE)
	{
		popup.printf(3, false, "Set %s mode via Auto, restart app for option to take effect",
			Base::Window::defaultPixelFormat() == PIXEL_RGB565 ? "RGB565" : "RGBA8888");
	}
	else
	{
		popup.post("Restart app for option to take effect");
	}
	optionWindowPixelFormat = format;
}
#endif

static void setFontSize(uint val)
{
	optionFontSize = val;
	setupFont();
	placeElements();
}

template <size_t S>
static void printPathMenuEntryStr(char (&str)[S])
{
	string_printf(str, "Save Path: %s", savePathStrToDescStr(optionSavePath).data());
}

static class SavePathSelectMenu
{
public:
	PathChangeDelegate onPathChange;

	constexpr SavePathSelectMenu() {}

	void onClose(FSPicker &picker, Input::Event e)
	{
		EmuSystem::savePath_ = picker.path();
		logMsg("set save path %s", (char*)optionSavePath);
		onPathChange.callSafe(optionSavePath);
	}

	void init(Input::Event e)
	{
		auto &multiChoiceView = *new TextTableView{"Save Path", mainWin.win, 3};
		multiChoiceView.appendItem("Set Custom Path",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto startPath = strlen(optionSavePath) ? optionSavePath : optionLastLoadPath;
				auto &fPicker = *new EmuFilePicker{mainWin.win, startPath, true, {}};
				fPicker.setOnClose(
					[this](FSPicker &picker, Input::Event e)
					{
						onClose(picker, e);
						picker.dismiss();
						viewStack.popAndShow();
					});
				modalViewController.pushAndShow(fPicker, e);
			});
		multiChoiceView.appendItem("Same as Game",
			[this]()
			{
				auto onPathChange = this->onPathChange;
				viewStack.popAndShow();
				strcpy(optionSavePath, "");
				onPathChange.callSafe("");
			});
		multiChoiceView.appendItem("Default",
			[this]()
			{
				auto onPathChange = this->onPathChange;
				viewStack.popAndShow();
				strcpy(optionSavePath, optionSavePathDefaultToken);
				onPathChange.callSafe(optionSavePathDefaultToken);
			});
		viewStack.pushAndShow(multiChoiceView, e);
	}
} pathSelectMenu;

class DetectFrameRateView : public View
{
	IG::WindowRect viewFrame;

public:
	using DetectFrameRateDelegate = DelegateFunc<void (double frameRate)>;
	DetectFrameRateDelegate onDetectFrameTime;
	Base::Screen::OnFrameDelegate detectFrameRate;
	Base::FrameTimeBase totalFrameTime{};
	Gfx::Text fpsText;
	double lastAverageFrameTimeSecs = 0;
	uint totalFrames = 0;
	uint callbacks = 0;
	std::array<char, 32> fpsStr{};

	DetectFrameRateView(Base::Window &win): View(win),
		fpsText{nullptr, View::defaultFace}
	{
		View::defaultFace->precacheAlphaNum();
		View::defaultFace->precache(".");
		fpsText.setString("Preparing to detect frame rate...");
	}

	~DetectFrameRateView() override
	{
		setCPUNeedsLowLatency(false);
		emuWin->win.screen()->removeOnFrame(detectFrameRate);
	}

	IG::WindowRect &viewRect() override { return viewFrame; }

	double totalFrameTimeSecs() const
	{
		return Base::frameTimeBaseToSecsDec(totalFrameTime);
	}

	double averageFrameTimeSecs() const
	{
		return totalFrameTimeSecs() / (double)totalFrames;
	}

	void place() override
	{
		fpsText.compile(projP);
	}

	void inputEvent(Input::Event e) override
	{
		if(e.pushed() && e.isDefaultCancelButton())
		{
			logMsg("aborted detection");
			popAndShow();
		}
	}

	void draw() override
	{
		using namespace Gfx;
		setColor(1., 1., 1., 1.);
		texAlphaProgram.use(projP.makeTranslate());
		fpsText.draw(projP.alignXToPixel(projP.bounds().xCenter()),
			projP.alignYToPixel(projP.bounds().yCenter()), C2DO, projP);
	}

	void onAddedToController(Input::Event e) override
	{
		setCPUNeedsLowLatency(true);
		detectFrameRate =
			[this](Base::Screen::FrameParams params)
			{
				postDraw();
				const uint callbacksToSkip = 30;
				callbacks++;
				if(callbacks >= callbacksToSkip)
				{
					detectFrameRate =
						[this](Base::Screen::FrameParams params)
						{
							postDraw();
							const uint framesToTime = 120 * 10;
							totalFrameTime += params.timestampDiff();
							totalFrames++;
							if(totalFrames % 120 == 0)
							{
								if(!lastAverageFrameTimeSecs)
									lastAverageFrameTimeSecs = averageFrameTimeSecs();
								else
								{
									double avgFrameTimeSecs = averageFrameTimeSecs();
									double avgFrameTimeDiff = std::abs(lastAverageFrameTimeSecs - avgFrameTimeSecs);
									if(avgFrameTimeDiff < 0.00001)
									{
										logMsg("finished with diff %.8f, total frame time: %.2f, average %.6f over %u frames",
											avgFrameTimeDiff, totalFrameTimeSecs(), avgFrameTimeSecs, totalFrames);
										onDetectFrameTime(avgFrameTimeSecs);
										popAndShow();
										return;
									}
									else
										lastAverageFrameTimeSecs = averageFrameTimeSecs();
								}
								if(totalFrames % 60 == 0)
								{
									string_printf(fpsStr, "%.2ffps", 1. / averageFrameTimeSecs());
									fpsText.setString(fpsStr.data());
									fpsText.compile(projP);
								}
							}
							if(totalFrames >= framesToTime)
							{
								logErr("unstable frame rate over frame time: %.2f, average %.6f over %u frames",
									totalFrameTimeSecs(), averageFrameTimeSecs(), totalFrames);
								onDetectFrameTime(0);
								popAndShow();
							}
							else
							{
								params.readdOnFrame();
							}
						};
					params.screen().addOnFrame(detectFrameRate);
				}
				else
				{
					params.readdOnFrame();
				}
			};
		emuWin->win.screen()->addOnFrame(detectFrameRate);
	}
};

static class FrameRateSelectMenu
{
public:
	using FrameRateChangeDelegate = DelegateFunc<void (double frameRate)>;
	FrameRateChangeDelegate onFrameTimeChange;

	constexpr FrameRateSelectMenu() {}

	void init(EmuSystem::VideoSystem vidSys, Input::Event e)
	{
		const bool includeFrameRateDetection = !Config::envIsIOS;
		auto &multiChoiceView = *new TextTableView{"Frame Rate", mainWin.win, includeFrameRateDetection ? 4 : 3};
		multiChoiceView.appendItem("Set with screen's reported rate",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				if(!emuWin->win.screen()->frameRateIsReliable())
				{
					#ifdef __ANDROID__
					if(Base::androidSDK() <= 10)
					{
						popup.postError("Many Android 2.3 devices mis-report their refresh rate, "
							"using the detected or default rate may give better results");
					}
					else
					#endif
					{
						popup.postError("Reported rate potentially unreliable, "
							"using the detected or default rate may give better results");
					}
				}
				onFrameTimeChange.callSafe(0);
				view.popAndShow();
			});
		multiChoiceView.appendItem("Set default rate",
			[this, vidSys](TextMenuItem &, View &view, Input::Event e)
			{
				onFrameTimeChange.callSafe(EmuSystem::defaultFrameTime(vidSys));
				view.popAndShow();
			});
		multiChoiceView.appendItem("Set custom rate",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				auto &textInputView = *new CollectTextInputView{view.window()};
				textInputView.init("Input decimal or fraction", "", getCollectTextCloseAsset());
				textInputView.onText() =
					[this](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							double numer, denom;
							int items = sscanf(str, "%lf /%lf", &numer, &denom);
							if(items == 1 && numer > 0)
							{
								onFrameTimeChange.callSafe(1. / numer);
							}
							else if(items > 1 && (numer > 0 && denom > 0))
							{
								onFrameTimeChange.callSafe(denom / numer);
							}
							else
							{
								popup.postError("Invalid input");
								return 1;
							}
						}
						view.dismiss();
						return 0;
					};
				view.popAndShow();
				modalViewController.pushAndShow(textInputView, e);
			});
		if(includeFrameRateDetection)
		{
			multiChoiceView.appendItem("Detect screen's rate and set",
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					auto &frView = *new DetectFrameRateView{view.window()};
					frView.onDetectFrameTime =
						[this](double frameTime)
						{
							if(frameTime)
							{
								onFrameTimeChange.callSafe(frameTime);
							}
							else
							{
								popup.postError("Detected rate too unstable to use");
							}
						};
					view.popAndShow();
					modalViewController.pushAndShow(frView, e);
				});
		}
		viewStack.pushAndShow(multiChoiceView, e);
	}
} frameRateSelectMenu;

void FirmwarePathSelector::onClose(FSPicker &picker, Input::Event e)
{
	snprintf(optionFirmwarePath, sizeof(FS::PathString), "%s", picker.path().data());
	logMsg("set firmware path %s", (char*)optionFirmwarePath);
	onPathChange.callSafe(optionFirmwarePath);
}

void FirmwarePathSelector::init(const char *name, Input::Event e)
{
	auto &multiChoiceView = *new TextTableView{name, mainWin.win, 2};
	multiChoiceView.appendItem("Set Custom Path",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			viewStack.popAndShow();
			auto startPath = strlen(optionFirmwarePath) ? optionFirmwarePath : optionLastLoadPath;
			auto &fPicker = *new EmuFilePicker{mainWin.win, startPath, true, {}};
			fPicker.setOnClose(
				[this](FSPicker &picker, Input::Event e)
				{
					onClose(picker, e);
					picker.dismiss();
				});
			modalViewController.pushAndShow(fPicker, e);
		});
	multiChoiceView.appendItem("Default",
		[this]()
		{
			viewStack.popAndShow();
			strcpy(optionFirmwarePath, "");
			onPathChange.callSafe("");
		});
	viewStack.pushAndShow(multiChoiceView, e);
}

template <size_t S>
static void printFrameRateStr(char (&str)[S])
{
	string_printf(str, "Frame Rate: %.2fHz",
		1. / EmuSystem::frameTime(EmuSystem::VIDSYS_NATIVE_NTSC));
}

template <size_t S>
static void printFrameRatePALStr(char (&str)[S])
{
	string_printf(str, "Frame Rate (PAL): %.2fHz",
		1. / EmuSystem::frameTime(EmuSystem::VIDSYS_PAL));
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
	item.emplace_back(&imgFilter);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	item.emplace_back(&imgEffect);
	#endif
	item.emplace_back(&overlayEffect);
	item.emplace_back(&overlayEffectLevel);
	item.emplace_back(&zoom);
	item.emplace_back(&viewportZoom);
	item.emplace_back(&aspectRatio);
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
	if(!optionDitherImage.isConst)
	{
		item.emplace_back(&dither);
	}
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
	if(!optionSoundRate.isConst) { item.emplace_back(&audioRate); }
	#ifdef CONFIG_AUDIO_LATENCY_HINT
	item.emplace_back(&soundBuffers);
	#endif
	#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
	item.emplace_back(&sndUnderrunCheck);
	#endif
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	item.emplace_back(&audioSoloMix);
	#endif
}

void SystemOptionView::loadStockItems()
{
	item.emplace_back(&autoSaveState);
	item.emplace_back(&confirmAutoLoadState);
	item.emplace_back(&confirmOverwriteState);
	printPathMenuEntryStr(savePathStr);
	item.emplace_back(&savePath);
	item.emplace_back(&checkSavePathWriteAccess);
	item.emplace_back(&fastForwardSpeed);
	#ifdef __ANDROID__
	item.emplace_back(&processPriority);
	if(!optionFakeUserActivity.isConst)
		item.emplace_back(&fakeUserActivity);
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
	item.emplace_back(&rememberLastMenu);
	if(!optionFontSize.isConst)
	{
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

VideoOptionView::VideoOptionView(Base::Window &win, bool customMenu):
	TableView{"Video Options", win, item},
	#ifdef __ANDROID__
	androidTextureStorageItem
	{
		{
			"Auto",
			[this]()
			{
				setAndroidTextureStorage(OPTION_ANDROID_TEXTURE_STORAGE_AUTO);
				popup.printf(3, false, "Set %s mode via Auto", Gfx::Texture::androidStorageImplStr());
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
			[this](TextMenuItem &, View &view, Input::Event)
			{
				static auto setAndroidTextureStorageGraphicBuffer =
					[]()
					{
						if(!setAndroidTextureStorage(OPTION_ANDROID_TEXTURE_STORAGE_GRAPHIC_BUFFER))
						{
							popup.postError("Not supported on this GPU");
							return false;
						}
						return true;
					};
				if(!Gfx::Texture::isAndroidGraphicBufferStorageWhitelisted())
				{
					auto &ynAlertView = *new YesNoAlertView{view.window(),
						"Setting Graphic Buffer improves performance but may hang or crash "
						"the app depending on your device or GPU",
						"OK", "Cancel"};
					ynAlertView.setOnYes(
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
					modalViewController.pushAndShow(ynAlertView, Input::defaultEvent());
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
					popup.postError("Not supported on this GPU");
					return false;
				}
				return true;
			}
		}
	},
	androidTextureStorage
	{
		"GPU Copy Mode",
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
		(uint)optionFrameInterval - 1u,
		frameIntervalItem
	},
	#endif
	dropLateFrames
	{
		"Skip Late Frames",
		(bool)optionSkipLateFrames,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSkipLateFrames.val = item.flipBoolValue(*this);
		}
	},
	frameRate
	{
		frameRateStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			frameRateSelectMenu.init(EmuSystem::VIDSYS_NATIVE_NTSC, e);
			frameRateSelectMenu.onFrameTimeChange =
				[this](double time)
				{
					double wantedTime = time;
					if(!time)
					{
						wantedTime = emuWin->win.screen()->frameTime();
					}
					if(!EmuSystem::setFrameTime(EmuSystem::VIDSYS_NATIVE_NTSC, wantedTime))
					{
						popup.printf(4, true, "%.2fHz not in valid range", 1. / wantedTime);
						return;
					}
					EmuSystem::configFrameTime();
					optionFrameRate = time;
					printFrameRateStr(frameRateStr);
					frameRate.compile(projP);
				};
			postDraw();
		}
	},
	frameRatePAL
	{
		frameRatePALStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			frameRateSelectMenu.init(EmuSystem::VIDSYS_PAL, e);
			frameRateSelectMenu.onFrameTimeChange =
				[this](double time)
				{
					double wantedTime = time;
					if(!time)
					{
						wantedTime = emuWin->win.screen()->frameTime();
					}
					if(!EmuSystem::setFrameTime(EmuSystem::VIDSYS_PAL, wantedTime))
					{
						popup.printf(4, true, "%.2fHz not in valid range", 1. / wantedTime);
						return;
					}
					EmuSystem::configFrameTime();
					optionFrameRatePAL = time;
					printFrameRatePALStr(frameRatePALStr);
					frameRatePAL.compile(projP);
				};
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
		{"70%", [this]() { setZoom(70); }},
		{"Integer-only", [this]() { setZoom(optionImageZoomIntegerOnly); }},
		{"Integer-only (Height)", [this]() { setZoom(optionImageZoomIntegerOnlyY); }},
	},
	zoom
	{
		"Zoom",
		[]() -> uint
		{
			switch(optionImageZoom.val)
			{
				default:
				case 100: return 0;
				case 90: return 1;
				case 80: return 2;
				case 70: return 3;
				case optionImageZoomIntegerOnly: return 4;
				case optionImageZoomIntegerOnlyY: return 5;
			}
		}(),
		zoomItem
	},
	viewportZoomItem
	{
		{"100%", [this]() { setViewportZoom(100); }},
		{"95%", [this]() { setViewportZoom(95); }},
		{"90%", [this]() { setViewportZoom(90); }},
		{"85%", [this]() { setViewportZoom(85); }}
	},
	viewportZoom
	{
		"Screen Area",
		[]() -> uint
		{
			switch(optionViewportZoom.val)
			{
				default:
				case 100: return 0;
				case 95: return 1;
				case 90: return 2;
				case 85: return 3;
			}
		}(),
		viewportZoomItem
	},
	imgFilter
	{
		"Image Interpolation",
		(bool)optionImgFilter,
		"None", "Linear",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionImgFilter.val = item.flipBoolValue(*this);
			emuVideoLayer.setLinearFilter(optionImgFilter);
			emuWin->win.postDraw();
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
		[]() -> uint
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
		[]() -> uint
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
		{"10%", [this]() { setOverlayEffectLevel(10); }},
		{"25%", [this]() { setOverlayEffectLevel(25); }},
		{"33%", [this]() { setOverlayEffectLevel(33); }},
		{"50%", [this]() { setOverlayEffectLevel(50); }},
		{"66%", [this]() { setOverlayEffectLevel(66); }},
		{"75%", [this]() { setOverlayEffectLevel(75); }},
		{"100%", [this]() { setOverlayEffectLevel(100); }}
	},
	overlayEffectLevel
	{
		"Overlay Effect Level",
		[]() -> uint
		{
			switch(optionOverlayEffectLevel)
			{
				default: return 0;
				case 25: return 1;
				case 33: return 2;
				case 50: return 3;
				case 66: return 4;
				case 75: return 5;
				case 100: return 6;
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
		[]() -> uint
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
		[]() -> uint
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
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			setEmuViewOnExtraWindow(item.flipBoolValue(*this));
		}
	},
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	showOnSecondScreen
	{
		"External Screen",
		(bool)optionShowOnSecondScreen,
		"OS Managed", "Game Content",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionShowOnSecondScreen = item.flipBoolValue(*this);
			if(Base::Screen::screens() > 1)
				setEmuViewOnExtraWindow(optionShowOnSecondScreen);
		}
	},
	#endif
	dither
	{
		"Dither Image",
		(bool)optionDitherImage,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionDitherImage = item.flipBoolValue(*this);
			Gfx::setDither(optionDitherImage);
		}
	}
{
	iterateTimes(EmuSystem::aspectRatioInfos, i)
	{
		aspectRatioItem[i] = {EmuSystem::aspectRatioInfo[i].name,
			[this, i]()
			{
				optionAspectRatio.val = EmuSystem::aspectRatioInfo[i].aspect;
				logMsg("set aspect ratio: %u:%u", optionAspectRatio.val.x, optionAspectRatio.val.y);
				placeEmuViews();
				emuWin->win.postDraw();
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

AudioOptionView::AudioOptionView(Base::Window &win, bool customMenu):
	TableView{"Audio Options", win, item},
	snd
	{
		"Sound",
		(bool)optionSound,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSound = item.flipBoolValue(*this);
			if(!optionSound)
				Audio::closePcm();
		}
	},
	#ifdef CONFIG_AUDIO_LATENCY_HINT
	soundBuffersItem
	{
		{"2", [this]() { setSoundBuffers(2); }},
		{"3", [this]() { setSoundBuffers(3); }},
		{"4", [this]() { setSoundBuffers(4); }},
		{"5", [this]() { setSoundBuffers(5); }},
		{"6", [this]() { setSoundBuffers(6); }},
		{"7", [this]() { setSoundBuffers(7); }},
		{"8", [this]() { setSoundBuffers(8); }},
		{"9", [this]() { setSoundBuffers(9); }},
		{"10", [this]() { setSoundBuffers(10); }},
	},
	soundBuffers
	{
		"Buffer Size In Frames",
		std::min((uint)optionSoundBuffers - (uint)OPTION_SOUND_BUFFERS_MIN, (uint)IG::size(soundBuffersItem) - 1u),
		[this](const MultiChoiceMenuItem &) -> int
		{
			return IG::size(soundBuffersItem) - (OPTION_SOUND_BUFFERS_MIN - 2);
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return soundBuffersItem[idx + (OPTION_SOUND_BUFFERS_MIN - 2)];
		}
	},
	#endif
	audioRateItem
	{
		{"22KHz", [this]() { setAudioRate(22050); }},
		{"32KHz", [this]() { setAudioRate(32000); }},
		{"44KHz", [this]() { setAudioRate(44100); }},
		{"48KHz", [this]() { setAudioRate(48000); }}
	},
	audioRate
	{
		"Sound Rate",
		[]() -> uint
		{
			switch(optionSoundRate)
			{
				case 22050: return 0;
				case 32000: return 1;
				default:
				case 44100: return 2;
				case 48000: return 3;
			}
		}(),
		[this](const MultiChoiceMenuItem &) -> int
		{
			return AudioManager::nativeFormat().rate >= 48000 ? 4u : 3u;
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return audioRateItem[idx];
		}
	}
	#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
	,sndUnderrunCheck
	{
		"Strict Underrun Check",
		(bool)optionSoundUnderrunCheck,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			if(Audio::isOpen())
				Audio::closePcm();
			optionSoundUnderrunCheck = item.flipBoolValue(*this);
		}
	}
	#endif
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	,audioSoloMix
	{
		"Mix With Other Apps",
		!optionAudioSoloMix,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionAudioSoloMix = item.flipBoolValue(*this);
		}
	}
	#endif
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

SystemOptionView::SystemOptionView(Base::Window &win, bool customMenu):
	TableView{"System Options", win, item},
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
		[]() -> uint
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
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionConfirmAutoLoadState = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State",
		(bool)optionConfirmOverwriteState,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionConfirmOverwriteState = item.flipBoolValue(*this);
		}
	},
	savePath
	{
		savePathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pathSelectMenu.init(e);
			pathSelectMenu.onPathChange =
				[this](const char *newPath)
				{
					if(string_equal(newPath, optionSavePathDefaultToken))
					{
						auto path = EmuSystem::baseDefaultGameSavePath();
						popup.printf(4, false, "Default Save Path:\n%s", path.data());
					}
					printPathMenuEntryStr(savePathStr);
					savePath.compile(projP);
					EmuSystem::setupGameSavePath();
					EmuSystem::savePathChanged();
				};
			postDraw();
		}
	},
	checkSavePathWriteAccess
	{
		"Check Save Path Write Access",
		(bool)optionCheckSavePathWriteAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionCheckSavePathWriteAccess = item.flipBoolValue(*this);
		}
	},
	fastForwardSpeedItem
	{
		{"3x", [this]() { optionFastForwardSpeed = 2; }},
		{"4x", [this]() { optionFastForwardSpeed = 3; }},
		{"5x", [this]() { optionFastForwardSpeed = 4; }},
		{"6x", [this]() { optionFastForwardSpeed = 5; }},
		{"7x", [this]() { optionFastForwardSpeed = 6; }},
		{"8x", [this]() { optionFastForwardSpeed = 7; }},
	},
	fastForwardSpeed
	{
		"Fast Forward Speed",
		[]() -> uint
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
		[]() -> uint
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
	fakeUserActivity
	{
		"Prevent CPU Idling",
		(bool)optionFakeUserActivity,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			userActivityFaker.reset();
			if(!item.boolValue())
			{
				userActivityFaker = std::make_unique<Base::UserActivityFaker>();
			}
			optionFakeUserActivity = item.flipBoolValue(*this);
		}
	}
	#endif
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

GUIOptionView::GUIOptionView(Base::Window &win, bool customMenu):
	TableView{"GUI Options", win, item},
	pauseUnfocused
	{
		Config::envIsPS3 ? "Pause in XMB" : "Pause if unfocused",
		(bool)optionPauseUnfocused,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionPauseUnfocused = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2", [this](){ setFontSize(2000); }},
		{"2.5", [this]() { setFontSize(2500); }},
		{"3", [this]() { setFontSize(3000); }},
		{"3.5", [this]() { setFontSize(3500); }},
		{"4", [this]() { setFontSize(4000); }},
		{"4.5", [this]() { setFontSize(4500); }},
		{"5", [this]() { setFontSize(5000); }},
		{"5.5", [this]() { setFontSize(5500); }},
		{"6", [this]() { setFontSize(6000); }},
		{"6.5", [this]() { setFontSize(6500); }},
		{"7", [this]() { setFontSize(7000); }},
		{"7.5", [this]() { setFontSize(7500); }},
		{"8", [this]() { setFontSize(8000); }},
		{"8.5", [this]() { setFontSize(8500); }},
		{"9", [this]() { setFontSize(9000); }},
		{"9.5", [this]() { setFontSize(9500); }},
		{"10", [this]() { setFontSize(10000); }},
		{"10.5", [this]() { setFontSize(10500); }},
	},
	fontSize
	{
		"Font Size",
		[]() -> uint
		{
			switch(optionFontSize)
			{
				default: return 0;
				case 2500: return 1;
				case 3000: return 2;
				case 3500: return 3;
				case 4000: return 4;
				case 4500: return 5;
				case 5000: return 6;
				case 5500: return 7;
				case 6000: return 8;
				case 6500: return 9;
				case 7000: return 10;
				case 7500: return 11;
				case 8000: return 12;
				case 8500: return 13;
				case 9000: return 14;
				case 9500: return 15;
				case 10000: return 16;
				case 10500: return 17;
			}
		}(),
		fontSizeItem
	},
	notificationIcon
	{
		"Suspended App Icon",
		(bool)optionNotificationIcon,
		[this](BoolMenuItem &item, View &, Input::Event e)
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
		"Dim OS Navigation",
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
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionIdleDisplayPowerSave = item.flipBoolValue(*this);
			Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
		}
	},
	navView
	{
		"Title Bar",
		(bool)optionTitleBar,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionTitleBar = item.flipBoolValue(*this);
			viewStack.showNavView(optionTitleBar);
			placeElements();
		}
	},
	backNav
	{
		"Title Back Navigation",
		View::needsBackControl,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			View::setNeedsBackControl(item.flipBoolValue(*this));
			viewStack.setShowNavViewBackButton(View::needsBackControl);
			placeElements();
		}
	},
	rememberLastMenu
	{
		"Remember Last Menu",
		(bool)optionRememberLastMenu,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionRememberLastMenu = item.flipBoolValue(*this);
		}
	},
	showBundledGames
	{
		"Show Bundled Games",
		(bool)optionShowBundledGames,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionShowBundledGames = item.flipBoolValue(*this);
			onMainMenuItemOptionChanged();
		}
	},
	showBluetoothScan
	{
		"Show Bluetooth Menu Items",
		(bool)optionShowBluetoothScan,
		[this](BoolMenuItem &item, View &, Input::Event e)
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
		{"Auto", [](){ setMenuOrientation(Base::VIEW_ROTATE_AUTO); }},
		#endif
		{landscapeName, [](){ setMenuOrientation(Base::VIEW_ROTATE_90); }},
		{landscape2Name, [](){ setMenuOrientation(Base::VIEW_ROTATE_270); }},
		{portraitName, [](){ setMenuOrientation(Base::VIEW_ROTATE_0); }},
		{portrait2Name, [](){ setMenuOrientation(Base::VIEW_ROTATE_180); }},
	},
	menuOrientation
	{
		"In Menu",
		[]() -> uint
		{
			uint itemOffset = Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 0 : 1;
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
		[]() -> uint
		{
			uint itemOffset = Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 0 : 1;
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

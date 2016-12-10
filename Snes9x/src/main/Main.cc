#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"

#include <snes9x.h>
#include <memmap.h>
#include <display.h>
#include <snapshot.h>
#include <cheats.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <controls.h>
#else
#include <apu.h>
#include <soundux.h>
#endif

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 1996-2011 the\nSnes9x Team\nwww.snes9x.com";
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static bool renderToScreen = false;
static const uint heightChangeFrameDelay = 4;
static uint heightChangeFrames = heightChangeFrameDelay;
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
#ifdef SNES9X_VERSION_1_4
static uint audioFramesPerUpdate = 0;
#endif

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](const char *name)
	{
		return string_hasDotExtension(name, "smc") ||
				string_hasDotExtension(name, "sfc") ||
				string_hasDotExtension(name, "fig") ||
				string_hasDotExtension(name, "mgd") ||
				string_hasDotExtension(name, "bs");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{"Bio Worm", "Bio Worm.7z"}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "SFC-SNES";
}

const char *EmuSystem::systemName()
{
	return "Super Famicom (SNES)";
}

#ifndef SNES9X_VERSION_1_4
bool8 S9xDeinitUpdate (int width, int height)
#else
bool8 S9xDeinitUpdate(int width, int height, bool8)
#endif
{
	if(unlikely(height == 239 && emuVideo.vidPix.h() == 224 && heightChangeFrames))
	{
		// ignore rapid 224 -> 239 -> 224 height changes
		//logMsg("skipped height change");
		heightChangeFrames--;
		height = 224;
	}
	else
	{
		heightChangeFrames = heightChangeFrameDelay;
	}
	emuVideo.initImage(false, width, height);
	IG::Pixmap srcPix = {{{width, height}, pixFmt}, GFX.Screen};
	emuVideo.writeFrame(srcPix);
	if(likely(renderToScreen))
	{
		updateAndDrawEmuVideo();
		renderToScreen = false;
	}
	return 1;
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
	{
		S9xReset();
	}
	else
	{
		S9xSoftReset();
	}
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

#ifndef SNES9X_VERSION_1_4
#define FREEZE_EXT "frz"
#else
#define FREEZE_EXT "s96"
#endif

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c." FREEZE_EXT, statePath, gameName, saveSlotChar(slot));
}

#undef FREEZE_EXT

static FS::PathString sprintSRAMFilename()
{
	return FS::makePathStringPrintf("%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static FS::PathString sprintCheatsFilename()
{
	return FS::makePathStringPrintf("%s/%s.cht", EmuSystem::savePath(), EmuSystem::gameName().data());
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(!S9xFreezeGame(saveStr.data()))
		return {EIO, std::system_category()};
	else
		return {};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr.data()))
	{
		logMsg("loading state %s", saveStr.data());
		if(S9xUnfreezeGame(saveStr.data()))
		{
			IPPU.RenderThisFrame = TRUE;
			return {{}};
		}
		else
			return {{EIO, std::system_category()}};
	}
	return {{ENOENT, std::system_category()}};
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		if(Memory.SRAMSize)
		{
			logMsg("saving backup memory");
			auto saveStr = sprintSRAMFilename();
			fixFilePermissions(saveStr);
			Memory.SaveSRAM(saveStr.data());
		}
		auto cheatsStr = sprintCheatsFilename();
		if(!Cheat.num_cheats)
			logMsg("no cheats present, removing .cht file if present");
		else
			logMsg("saving %d cheat(s)", Cheat.num_cheats);
		S9xSaveCheatFile(cheatsStr.data());
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		if(!S9xFreezeGame(saveStr.data()))
			logMsg("error saving state %s", saveStr.data());
	}
}

void S9xAutoSaveSRAM (void)
{
	EmuSystem::saveBackupMem();
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
}

bool EmuSystem::vidSysIsPAL() { return Settings.PAL; }
uint EmuSystem::multiresVideoBaseX() { return 256; }
uint EmuSystem::multiresVideoBaseY() { return 239; }

static int loadGameCommon()
{
	emuVideo.initImage(false, 256, 224);
	setupSNESInput();

	auto saveStr = sprintSRAMFilename();
	Memory.LoadSRAM(saveStr.data());

	IPPU.RenderThisFrame = TRUE;
	EmuSystem::configAudioPlayback();
	logMsg("finished loading game");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	setupGamePaths(path);
	auto size = io.size();
	if(size > CMemory::MAX_ROM_SIZE)
	{
		popup.postError("ROM is too large");
    return 0;
	}
	#ifndef SNES9X_VERSION_1_4
	IG::fillData(Memory.NSRTHeader);
	#endif
	Memory.HeaderCount = 0;
	string_copy(Memory.ROMFilename, path);
	Settings.ForceNTSC = Settings.ForcePAL = 0;
	switch(optionVideoSystem.val)
	{
		bcase 1: Settings.ForceNTSC = 1;
		bcase 2: Settings.ForcePAL = 1;
		bcase 3: Settings.ForceNTSC = Settings.ForcePAL = 1;
	}
	bool success;
	if(io.mmapConst())
	{
		success = Memory.LoadROMMem((const uint8*)io.mmapConst(), size);
	}
	else
	{
		auto data = std::make_unique<uint8[]>(size);
		if(!io.read(data.get(), size))
		{
			popup.postError("IO Error loading game");
			return 0;
		}
		success = Memory.LoadROMMem(data.get(), size);
	}
	if(!success)
	{
		popup.postError("Error loading game");
		return 0;
	}
	return loadGameCommon();
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	#ifndef SNES9X_VERSION_1_4
	const double rateScaler = (32000./32040.5);
	const double ntscFrameRate = rateScaler * (21477272. / 357366.);
	const double palFrameRate = rateScaler * (21281370. / 425568.);
	#else
	const double ntscFrameRate = (21477272. / 357366.);
	const double palFrameRate = (21281370. / 425568.);
	#endif
	double systemFrameRate = vidSysIsPAL() ? palFrameRate : ntscFrameRate;
	Settings.SoundPlaybackRate = std::round(optionSoundRate * (systemFrameRate * frameTime));
	#ifndef SNES9X_VERSION_1_4
	S9xUpdatePlaybackRate();
	#else
	audioFramesPerUpdate = std::round(pcmFormat.rate * frameTime);
	S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	#endif
	logMsg("sound rate:%d from system frame rate:%f", Settings.SoundPlaybackRate, systemFrameRate);
}

static void mixSamples(int frames, bool renderAudio)
{
	if(likely(frames))
	{
		uint samples = frames * 2;
		int16 audioBuff[samples];
		S9xMixSamples((uint8_t*)audioBuff, samples);
		if(renderAudio)
		{
			//logMsg("%d frames", frames);
			EmuSystem::writeSound(audioBuff, frames);
		}
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	if(unlikely(snesActiveInputPort != SNES_JOYPAD))
	{
		if(doubleClickFrames)
			doubleClickFrames--;
		if(rightClickFrames)
			rightClickFrames--;

		#ifndef SNES9X_VERSION_1_4
		switch(snesActiveInputPort)
		{
			bcase SNES_MOUSE_SWAPPED:
			{
				int x,y;
				uint32 buttons;
				S9xReadMousePosition(0, x, y, buttons);
				*S9xGetMouseBits(0) &= ~(0x40 | 0x80);
				if(buttons == 1)
					*S9xGetMouseBits(0) |= 0x40;
				else if(buttons == 2)
					*S9xGetMouseBits(0) |= 0x80;
				S9xGetMousePosBits(0)[0] = x;
				S9xGetMousePosBits(0)[1] = y;
			}
		}
		#endif
	}

	IPPU.RenderThisFrame = processGfx ? TRUE : FALSE;
	if(renderGfx)
		renderToScreen = 1;
	#ifndef SNES9X_VERSION_1_4
	S9xSetSamplesAvailableCallback([](void *renderAudio)
		{
			S9xFinalizeSamples();
			int samples = S9xGetSampleCount();
			mixSamples(samples / 2, renderAudio);
		}, (void*)renderAudio);
	#endif
	S9xMainLoop();
	// video rendered in S9xDeinitUpdate
	#ifdef SNES9X_VERSION_1_4
	mixSamples(audioFramesPerUpdate, renderAudio);
	#endif
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((46./255.) * .4, (50./255.) * .4, (77./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onMainWindowCreated(Base::Window &win)
{
	win.setOnInputEvent(
		[](Base::Window &win, Input::Event e)
		{
			using namespace Input;
			if(unlikely(EmuSystem::isActive() && e.isPointer()))
			{
				switch(snesActiveInputPort)
				{
					bcase SNES_SUPERSCOPE:
					{
						if(e.state == RELEASED)
						{
							snesPointerBtns = 0;
							#ifndef SNES9X_VERSION_1_4
							*S9xGetSuperscopeBits() = 0;
							#endif
						}
						if(emuVideoLayer.gameRect().overlaps({e.x, e.y}))
						{
							int xRel = e.x - emuVideoLayer.gameRect().x, yRel = e.y - emuVideoLayer.gameRect().y;
							snesPointerX = IG::scalePointRange((float)xRel, (float)emuVideoLayer.gameRect().xSize(), (float)256.);
							snesPointerY = IG::scalePointRange((float)yRel, (float)emuVideoLayer.gameRect().ySize(), (float)224.);
							//logMsg("mouse moved to @ %d,%d, on SNES %d,%d", e.x, e.y, snesPointerX, snesPointerY);
							if(e.state == PUSHED)
							{
								snesPointerBtns = 1;
								#ifndef SNES9X_VERSION_1_4
								*S9xGetSuperscopeBits() = 0x80;
								#endif
							}
						}
						else if(e.state == PUSHED)
						{
							snesPointerBtns = 2;
							#ifndef SNES9X_VERSION_1_4
							*S9xGetSuperscopeBits() = 0x40;
							#endif
						}

						#ifndef SNES9X_VERSION_1_4
						S9xGetSuperscopePosBits()[0] = snesPointerX;
						S9xGetSuperscopePosBits()[1] = snesPointerY;
						#endif
					}

					bcase SNES_MOUSE_SWAPPED:
					{
						dragTracker.inputEvent(e,
							[&](Input::DragTrackerState)
							{
								rightClickFrames = 15;
								if(doubleClickFrames) // check if in double-click time window
								{
									dragWithButton = 1;
								}
								else
								{
									dragWithButton = 0;
									doubleClickFrames = 15;
								}
							},
							[&](Input::DragTrackerState state, Input::DragTrackerState prevState)
							{
								if(!state.isDragging())
									return;
								if(!prevState.isDragging())
								{
									if(dragWithButton)
									{
										snesMouseClick = 0;
										if(!rightClickFrames)
										{
											// in right-click time window
											snesPointerBtns = 2;
											logMsg("started drag with right-button");
										}
										else
										{
											snesPointerBtns = 1;
											logMsg("started drag with left-button");
										}
									}
									else
									{
										logMsg("started drag");
									}
								}
								else
								{
									auto relPos = state.pos() - prevState.pos();
									snesPointerX += relPos.x;
									snesPointerY += relPos.y;
								}
							},
							[&](Input::DragTrackerState state)
							{
								if(state.isDragging())
								{
									logMsg("stopped drag");
									snesPointerBtns = 0;
								}
								else
								{
									if(!rightClickFrames)
									{
										logMsg("right clicking mouse");
										snesPointerBtns = 2;
										doubleClickFrames = 15; // allow extra time for a right-click & drag
									}
									else
									{
										logMsg("left clicking mouse");
										snesPointerBtns = 1;
									}
									snesMouseClick = 3;
								}
							});
					}
				}
			}
			handleInputEvent(win, e);
		});
}

CallResult EmuSystem::onInit()
{
	emuVideo.initFormat(pixFmt);
	static uint16 screenBuff[512*478] __attribute__ ((aligned (8)));
	#ifndef SNES9X_VERSION_1_4
	GFX.Screen = screenBuff;
	#else
	GFX.Screen = (uint8*)screenBuff;
	#endif
	Memory.Init();
	S9xGraphicsInit();
	S9xInitAPU();
	assert(Settings.Stereo == TRUE);
	#ifndef SNES9X_VERSION_1_4
	S9xInitSound(16, 0);
	S9xUnmapAllControls();
	#else
	S9xInitSound(Settings.SoundPlaybackRate, Settings.Stereo, 0);
	assert(Settings.H_Max == SNES_CYCLES_PER_SCANLINE);
	assert(Settings.HBlankStart == (256 * Settings.H_Max) / SNES_HCOUNTER_MAX);
	#endif
	return OK;
}

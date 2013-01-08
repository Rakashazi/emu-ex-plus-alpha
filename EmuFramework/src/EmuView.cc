#include <EmuView.hh>
#include <EmuInput.hh>
#include <VController.hh>
#include <MsgPopup.hh>

extern SysVController vController;
extern bool touchControlsAreOn;
bool touchControlsApplicable();
extern bool ffGuiKeyPush, ffGuiTouch;
extern Gfx::Sprite menuIcon;
extern MsgPopup popup;

void EmuView::placeEmu()
{
	if(EmuSystem::gameIsRunning())
	{
		if((uint)optionImageZoom != optionImageZoomIntegerOnly)
		{
			if(optionAspectRatio == 0U)
				gameView.init(EmuSystem::aspectRatioX, EmuSystem::aspectRatioY);
			else if(optionAspectRatio == 1U)
				gameView.init(1, 1);
			else if(optionAspectRatio == 2U)
				gameView.init();
			gameView.setSizeViewSpaceBestFit();
		}
		else
		{
			gameView.init();
			uint scaleFactor;
			// TODO: generalize this?
			uint gameX = vidPix.x, gameY = vidPix.y;
			GC gameAR = GC(gameX) / GC(gameY);
			if(gameAR >= 2) // avoid overly wide images
			{
				logMsg("unscaled image too wide, doubling height to compensate");
				gameY *= 2;
				gameAR = GC(gameX) / GC(gameY);
			}
			if(gameAR > Gfx::proj.aspectRatio)
			{
				scaleFactor = IG::max(1U, Gfx::viewPixelWidth() / gameX);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = IG::max(1U, Gfx::viewPixelHeight() / gameY);
				logMsg("using y scale factor %d", scaleFactor);
			}
			gameView.setXSize(Gfx::iXSize(gameX * scaleFactor));
			gameView.setYSize(Gfx::iYSize(gameY * scaleFactor));
		}
		gameView.setPosOrigin(C2DO, C2DO);
		GC yOffset = 0;
		#ifdef INPUT_SUPPORTS_POINTER
		if(Gfx::proj.aspectRatio < 1. && touchControlsAreOn && touchControlsApplicable())
		{
			if(vController.gp.dp.origin.onBottom() && vController.gp.btnO.onBottom())
			{
				logMsg("moving game view to top");
				gameView.setPosOrigin(CT2DO, CT2DO);
				if(vController.gp.cenBtnO.onTop())
					yOffset = Gfx::iXSize(-vController.gp.centerBtnBound[0].ySize());
			}
			else if(vController.gp.dp.origin.onTop() && vController.gp.btnO.onTop())
			{
				logMsg("moving game view to bottom");
				gameView.setPosOrigin(CB2DO, CB2DO);
				if(vController.gp.cenBtnO.onBottom())
					yOffset = Gfx::iXSize(vController.gp.centerBtnBound[0].ySize());
			}
		}
		#endif

		if(optionImageZoom.val == optionImageZoomIntegerOnly)
		{
			gameView.alignToPixelUnits();
		}
		else if(optionImageZoom.val != 100)
		{
			Area unzoomed = gameView;
			gameView.scaleSize(GC(optionImageZoom.val) / 100.);
			gameView.setPos(&unzoomed, C2DO, C2DO);
		}

		disp.setPos(gameView.xPos(LB2DO), gameView.yPos(LB2DO) + yOffset, gameView.xPos(RT2DO), gameView.yPos(RT2DO) + yOffset);
		#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
		disp.screenX = gameView.xIPos(LB2DO);
		disp.screenY = Gfx::viewPixelHeight() - gameView.yIPos(LB2DO);
		disp.screenX2 = gameView.iXSize;
		disp.screenY2 = gameView.iYSize;
		#endif
	}
	placeOverlay();
}

template <bool active>
void EmuView::drawContent()
{
	using namespace Gfx;
	disp.draw();
	vidImgOverlay.draw();
	#ifndef CONFIG_BASE_PS3
		if(active && ((touchControlsAreOn && touchControlsApplicable())
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
			|| vController.kbMode
		#endif
		))
		{
			vController.draw();
			if(optionShowMenuIcon)
			{
				setBlendMode(BLEND_MODE_INTENSITY);
				menuIcon.draw();
			}
		}
	#endif
	popup.draw();
}

void EmuView::draw()
{
	using namespace Gfx;
	if(likely(EmuSystem::isActive()))
	{
		resetTransforms();
		setBlendMode(0);
		setImgMode(IMG_MODE_REPLACE);
		/*setImgMode(IMG_MODE_MODULATE);
		setColor(COLOR_WHITE);*/
		#ifdef CONFIG_BASE_PS3
		setColor(1., 1., 1., 1.); // hack to work-around non-working GFX_IMG_MODE_REPLACE
		#endif
		Base::displayNeedsUpdate();

		runFrame();
	}
	else if(EmuSystem::isStarted())
	{
		setBlendMode(0);
		setImgMode(IMG_MODE_MODULATE);
		setColor(.33, .33, .33, 1.);
		resetTransforms();
		drawContent<0>();
	}
}

void EmuView::runFrame()
{
	commonUpdateInput();
	bool renderAudio = optionSound;

	if(unlikely(ffGuiKeyPush || ffGuiTouch))
	{
		iterateTimes(4, i)
		{
			EmuSystem::runFrame(0, 0, 0);
		}
	}
	else
	{
		int framesToSkip = EmuSystem::setupFrameSkip(optionFrameSkip);
		if(framesToSkip > 0)
		{
			iterateTimes(framesToSkip, i)
			{
				EmuSystem::runFrame(0, 0, renderAudio);
			}
		}
		else if(framesToSkip == -1)
		{
			drawContent<1>();
			return;
		}
	}

	EmuSystem::runFrame(1, 1, renderAudio);
}

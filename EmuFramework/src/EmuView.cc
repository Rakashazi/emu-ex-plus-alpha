#include <EmuView.hh>
#include <EmuInput.hh>
#include <VController.hh>
#include <MsgPopup.hh>

#ifdef INPUT_SUPPORTS_POINTER
extern SysVController vController;
#endif
extern bool touchControlsAreOn;
bool touchControlsApplicable();
extern bool ffGuiKeyPush, ffGuiTouch;
extern Gfx::Sprite menuIcon;
extern MsgPopup popup;

void EmuView::placeEmu()
{
	if(EmuSystem::gameIsRunning())
	{
		// compute the video rectangle in pixel coordinates
		if((uint)optionImageZoom == optionImageZoomIntegerOnly || (uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			uint scaleFactor;
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

			gameRect.x = 0;
			gameRect.y = 0;
			gameRect.x2 = gameX * scaleFactor;
			gameRect.y2 = gameY * scaleFactor;
			gameRect.setPos(Gfx::viewPixelWidth()/2 - gameRect.x2/2, Gfx::viewPixelHeight()/2 - gameRect.y2/2);
		}

		// compute the video rectangle in world coordinates for sub-pixel placement
		if((uint)optionImageZoom <= 100 || (uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			Rational aR(EmuSystem::aspectRatioX, EmuSystem::aspectRatioY);
			if(optionAspectRatio == 1U)
				aR = {1, 1};
			else if(optionAspectRatio == 2U)
				aR = {0, 1};

			if((uint)optionImageZoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				GC width = Gfx::iYSize(gameRect.ySize()) * (GC)aR;
				if(!aR)
				{
					width = Gfx::proj.w;
				}
				gameRectG.x = -width/2.;
				gameRectG.x2 = width/2.;
			}
			else
			{
				IG::Point2D<GC> size { Gfx::proj.w, Gfx::proj.h };
				if(aR)
				{
					size = IG::sizesWithRatioBestFit((GC)aR, size.x, size.y);
				}
				gameRectG.x = -size.x/2.;
				gameRectG.x2 = size.x/2.;
				gameRectG.y = size.y/2.;
				gameRectG.y2 = -size.y/2.;
			}
		}

		// determine whether to generate the final coordinates from pixels or world units
		bool getXCoordinateFromPixels = 0, getYCoordinateFromPixels = 0;
		if((uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			getYCoordinateFromPixels = 1;
		}
		else if((uint)optionImageZoom == optionImageZoomIntegerOnly)
		{
			getXCoordinateFromPixels = getYCoordinateFromPixels = 1;
		}

		// adjust position
		GC yOffset = 0;
		int yOffsetPixels = 0;
		#ifdef INPUT_SUPPORTS_POINTER
		if(Gfx::proj.aspectRatio < 1. && touchControlsAreOn && touchControlsApplicable())
		{
			if(vController.gp.dp.origin.onBottom() && vController.gp.btnO.onBottom())
			{
				logMsg("moving game rect to top");
				if(vController.gp.cenBtnO.onTop())
				{
					yOffset = Gfx::iXSize(-vController.gp.centerBtnBound[0].ySize());
					yOffsetPixels = -vController.gp.centerBtnBound[0].ySize();
				}
				gameRectG.setYPos(Gfx::proj.rect.y, CT2DO);
				gameRect.setYPos(0, CT2DO);
			}
			else if(vController.gp.dp.origin.onTop() && vController.gp.btnO.onTop())
			{
				logMsg("moving game rect to bottom");
				if(vController.gp.cenBtnO.onBottom())
				{
					yOffset = Gfx::iXSize(vController.gp.centerBtnBound[0].ySize());
					yOffsetPixels = vController.gp.centerBtnBound[0].ySize();
				}
				gameRectG.setYPos(Gfx::proj.rect.y2, CB2DO);
				gameRect.setYPos(Gfx::viewPixelHeight(), CB2DO);
			}
		}
		#endif

		// apply sub-pixel zoom
		if(optionImageZoom.val < 100)
		{
			auto scaler = (GC(optionImageZoom.val) / 100.);
			gameRectG.x *= scaler;
			gameRectG.y *= scaler;
			gameRectG.x2 *= scaler;
			gameRectG.y2 *= scaler;
		}

		// apply y offset after zoom
		gameRectG += IG::Point2D<GC>{0, yOffset};
		gameRect += IG::Point2D<int>{0, yOffsetPixels};

		// assign final coordinates
		auto fromWorldSpaceRect = Gfx::projectRect2(gameRectG);
		auto fromPixelRect = Gfx::unProjectRect2(gameRect);
		if(getXCoordinateFromPixels)
		{
			gameRectG.x = fromPixelRect.x;
			gameRectG.x2 = fromPixelRect.x2;
		}
		else
		{
			gameRect.x = fromWorldSpaceRect.x;
			gameRect.x2 = fromWorldSpaceRect.x2;
		}
		if(getYCoordinateFromPixels)
		{
			gameRectG.y = fromPixelRect.y;
			gameRectG.y2 = fromPixelRect.y2;
		}
		else
		{
			gameRect.y = fromWorldSpaceRect.y;
			gameRect.y2 = fromWorldSpaceRect.y2;
		}

		disp.setPos(gameRectG.x, gameRectG.y2, gameRectG.x2, gameRectG.y);
		#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
		disp.screenX = gameView.xIPos(LB2DO);
		disp.screenY = Gfx::viewPixelHeight() - gameView.yIPos(LB2DO);
		disp.screenX2 = gameView.iXSize;
		disp.screenY2 = gameView.iYSize;
		#endif
		logMsg("placed game rect at pixels %d:%d:%d:%d, world %f:%f:%f:%f",
				gameRect.x, gameRect.y, gameRect.x2, gameRect.y2,
				(double)gameRectG.x, (double)gameRectG.y, (double)gameRectG.x2, (double)gameRectG.y2);
	}
	placeOverlay();
}

template <bool active>
void EmuView::drawContent()
{
	using namespace Gfx;
	disp.draw();
	vidImgOverlay.draw();
	#ifdef INPUT_SUPPORTS_POINTER
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

template void EmuView::drawContent<0>();
template void EmuView::drawContent<1>();

void EmuView::draw(Gfx::FrameTimeBase frameTime)
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

		runFrame(frameTime);
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

void EmuView::runFrame(Gfx::FrameTimeBase frameTime)
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
		int framesToSkip = EmuSystem::setupFrameSkip(optionFrameSkip, frameTime);
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

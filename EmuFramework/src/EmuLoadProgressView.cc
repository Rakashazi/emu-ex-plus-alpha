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

#include <emuframework/EmuLoadProgressView.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/math/space.hh>
#include <imagine/logger/logger.h>

EmuLoadProgressView::EmuLoadProgressView(ViewAttachParams attach, Input::Event e, EmuApp::CreateSystemCompleteDelegate onComplete):
	View{attach},
	onComplete{onComplete},
	text{"Loading...", &defaultFace()},
	originalEvent{e}
{
	msgPort.attach(
		[this](auto msgs)
		{
			for(auto msg : msgs)
			{
				switch(msg.progress)
				{
					bcase EmuSystem::LoadProgress::FAILED:
					{
						assumeExpr(msg.intArg3 > 0);
						unsigned len = msg.intArg3;
						char errorStr[len + 1];
						msgs.getExtraData(errorStr, len);
						errorStr[len] = 0;
						msgPort.detach();
						auto &app = this->app();
						app.popModalViews();
						app.postErrorMessage(4, errorStr);
						return;
					}
					bcase EmuSystem::LoadProgress::OK:
					{
						msgPort.detach();
						auto onComplete = this->onComplete;
						auto originalEvent = this->originalEvent;
						auto &app = this->app();
						app.popModalViews();
						app.viewController().onSystemCreated();
						onComplete(originalEvent);
						return;
					}
					bcase EmuSystem::LoadProgress::UPDATE:
					{
						setPos(msg.intArg);
						setMax(msg.intArg2);
						assumeExpr(msg.intArg3 >= -1);
						switch(msg.intArg3)
						{
							bcase -1: // no string
							{}
							bcase 0: // default string
							{
								setLabel("Loading...");
							}
							bdefault: // custom string
							{
								unsigned len = msg.intArg3;
								char labelStr[len + 1];
								msgs.getExtraData(labelStr, len);
								labelStr[len] = 0;
								setLabel(labelStr);
								logMsg("set custom string:%s", labelStr);
							}
						}
						place();
						postDraw();
					}
					bdefault:
					{
						logWarn("Unknown LoadProgressMessage value:%d", (int)msg.progress);
					}
				}
			}
		});
}

void EmuLoadProgressView::setMax(int val)
{
	if(val)
	{
		max = val;
	}
}

void EmuLoadProgressView::setPos(int val)
{
	pos = val;
}

void EmuLoadProgressView::setLabel(const char *labelStr)
{
	text.setString(labelStr);
}

void EmuLoadProgressView::place()
{
	text.compile(renderer(), projP);
}

bool EmuLoadProgressView::inputEvent(Input::Event e)
{
	return true;
}

void EmuLoadProgressView::draw(Gfx::RendererCommands &cmds)
{
	if(!text.isVisible())
		return;
	using namespace Gfx;
	projP.resetTransforms(cmds);
	cmds.setBlendMode(0);
	if(max)
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setColor(.0, .0, .75);
		Gfx::GC barHeight = text.height()*1.5;
		auto bar = makeGCRectRel(projP.bounds().pos(LC2DO) - GP{0_gc, barHeight/2_gc},
			{IG::scalePointRange((Gfx::GC)pos, 0_gc, (Gfx::GC)max, 0_gc, projP.width()), barHeight});
		GeomRect::draw(cmds, bar);
	}
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	cmds.set(ColorName::WHITE);
	text.draw(cmds, 0, 0, C2DO, projP);
}

EmuLoadProgressView::MessagePortType &EmuLoadProgressView::messagePort()
{
	return msgPort;
}

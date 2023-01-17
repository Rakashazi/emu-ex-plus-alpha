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
#include <imagine/util/math/math.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

EmuLoadProgressView::EmuLoadProgressView(ViewAttachParams attach, const Input::Event &e, EmuApp::CreateSystemCompleteDelegate onComplete):
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
					case EmuSystem::LoadProgress::FAILED:
					{
						assumeExpr(msg.intArg3 > 0);
						size_t len = msg.intArg3;
						char errorStr[len];
						msgs.getExtraData(std::span{errorStr, len});
						msgPort.detach();
						auto &app = this->app();
						app.popModalViews();
						app.postErrorMessage(4, std::string_view{errorStr, len});
						return;
					}
					case EmuSystem::LoadProgress::OK:
					{
						msgPort.detach();
						auto onComplete = this->onComplete;
						auto originalEvent = this->originalEvent;
						auto &app = this->app();
						app.popModalViews();
						app.onSystemCreated();
						onComplete(originalEvent);
						return;
					}
					case EmuSystem::LoadProgress::UPDATE:
					{
						setPos(msg.intArg);
						setMax(msg.intArg2);
						assumeExpr(msg.intArg3 >= -1);
						switch(msg.intArg3)
						{
							case -1: // no string
								break;
							case 0: // default string
								setLabel("Loading...");
								break;
							default: // custom string
							{
								size_t len = msg.intArg3;
								if(!len)
									break;
								char labelStr[len];
								msgs.getExtraData(std::span{labelStr, len});
								setLabel(std::string_view{labelStr, len});
								logMsg("set custom string:%s", labelStr);
							}
						}
						place();
						postDraw();
						break;
					}
					default:
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

void EmuLoadProgressView::place()
{
	text.compile(renderer());
}

bool EmuLoadProgressView::inputEvent(const Input::Event &e)
{
	return true;
}

void EmuLoadProgressView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	if(!text.isVisible())
		return;
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	cmds.set(BlendMode::OFF);
	if(max)
	{
		basicEffect.disableTexture(cmds);
		cmds.setColor(.0, .0, .75);
		int barHeight = text.height() * 1.5f;
		auto bar = WRect::makeRel(displayRect().pos(LC2DO) - WP{0, barHeight/2},
			{int(IG::remap(int64_t(pos), 0, max, 0, displayRect().xSize())), barHeight});
		GeomRect::draw(cmds, bar);
	}
	basicEffect.enableAlphaTexture(cmds);
	cmds.set(ColorName::WHITE);
	text.draw(cmds, displayRect().center(), C2DO);
}

EmuLoadProgressView::MessagePortType &EmuLoadProgressView::messagePort()
{
	return msgPort;
}

}

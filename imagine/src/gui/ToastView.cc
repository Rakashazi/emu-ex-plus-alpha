/*  This file is part of Imagine.

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

#define LOGTAG "ToastView"
#include <imagine/gui/ToastView.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <string>

ToastView::ToastView(ViewAttachParams attach): View{attach}
{
	text.maxLines = 6;
}

void ToastView::setFace(Gfx::GlyphTextureSet &face)
{
	auto lock = makeControllerMutexLock();
	text.setFace(&face);
	text.setString(str.data());
}

void ToastView::clear()
{
	if(strlen(str.data()))
	{
		unpostTimer.deinit();
		auto lock = makeControllerMutexLock();
		str = {};
	}
}

void ToastView::place()
{
	text.maxLineSize = projP.w;
	if(strlen(str.data()))
		text.compile(renderer(), projP);

	int labelYSize = IG::makeEvenRoundedUp(projP.projectYSize(text.fullHeight()));
	IG::WindowRect viewFrame;
	viewFrame.setPosRel(rect.pos(CB2DO),
		{rect.xSize(), labelYSize}, CB2DO);
	msgFrame = projP.unProjectRect(viewFrame);
}

void ToastView::unpost()
{
	logMsg("unposting");
	{
		auto lock = makeControllerMutexLock();
		str = {};
	}
	postDraw();
}

void ToastView::contentUpdated(bool error)
{
	assert(strlen(str.data()));
	logMsg("%s", str.data());
	place();
	this->error = error;
}

void ToastView::postContent(int secs)
{
	postDraw();
	unpostTimer.callbackAfterSec([this](){unpost();}, secs, {});
}

void ToastView::post(const char *msg, int secs, bool error)
{
	{
		auto lock = makeControllerMutexLock();
		string_copy(str, msg);
		contentUpdated(error);
	}
	postContent(secs);
}

void ToastView::postError(const char *msg, int secs)
{
	post(msg, secs, true);
}

void ToastView::post(const char *prefix, const std::system_error &err, int secs)
{
	printf(secs, true, "%s%s", prefix, err.what());
}

void ToastView::post(const char *prefix, std::error_code ec, int secs)
{
	printf(secs, true, "%s%s", prefix, ec.message().c_str());
}

void ToastView::printf(uint secs, bool error, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	vprintf(secs, error, format, args);
}

void ToastView::vprintf(uint secs, bool error, const char *format, va_list args)
{
	{
		auto lock = makeControllerMutexLock();
		auto result = vsnprintf(str.data(), str.size(), format, args);
		contentUpdated(error);
	}
	postContent(secs);
}

void ToastView::prepareDraw()
{
	text.makeGlyphs(renderer());
}

void ToastView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	if(!strlen(str.data()))
		return;
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	cmds.setBlendMode(BLEND_MODE_ALPHA);
	if(error)
		cmds.setColor(1., 0, 0, .7);
	else
		cmds.setColor(0, 0, 1., .7);
	GeomRect::draw(cmds, msgFrame);
	cmds.setColor(1., 1., 1., 1.);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	text.draw(cmds, 0, projP.alignYToPixel(msgFrame.y + (text.ySize * 1.5)/2.), C2DO, projP);
}

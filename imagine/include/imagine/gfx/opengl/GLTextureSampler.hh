#pragma once

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
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureSamplerConfig.hh>

namespace Gfx
{

class RendererTask;

class GLTextureSampler
{
public:
	constexpr GLTextureSampler() {}
	GLTextureSampler(RendererTask &rTask, TextureSamplerConfig config);
	~GLTextureSampler();
	void setTexParams(GLenum target) const;
	void deinit();
	GLuint name() const;
	const char *label() const;

protected:
	RendererTask *rTask{};
	GLuint name_{};
	uint16_t minFilter{};
	uint16_t magFilter{};
	uint16_t xWrapMode_{};
	uint16_t yWrapMode_{};
	IG_enableMemberIf(Config::DEBUG_BUILD, const char *, debugLabel);
};

using TextureSamplerImpl = GLTextureSampler;

}

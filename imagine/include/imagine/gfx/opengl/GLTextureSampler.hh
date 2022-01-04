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

namespace IG::Gfx
{

class RendererTask;

struct SamplerParams
{
	uint16_t minFilter;
	uint16_t magFilter;
	uint16_t xWrapMode;
	uint16_t yWrapMode;
};

class GLTextureSampler
{
public:
	constexpr GLTextureSampler() = default;
	GLTextureSampler(RendererTask &rTask, TextureSamplerConfig config);
	~GLTextureSampler();
	static void setTexParamsInGL(GLenum target, SamplerParams params);
	static void setTexParamsInGL(GLuint texName, GLenum target, SamplerParams params);
	GLuint name() const;
	const char *label() const;
	SamplerParams samplerParams() const;

protected:
	RendererTask *rTask{};
	union
	{
		SamplerParams params{};
		GLuint name_;
	};
	IG_UseMemberIf(Config::DEBUG_BUILD, const char *, debugLabel);

	void deinit();
};

using TextureSamplerImpl = GLTextureSampler;

}

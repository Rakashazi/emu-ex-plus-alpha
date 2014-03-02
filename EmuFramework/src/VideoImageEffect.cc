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

#include <VideoImageEffect.hh>
#include <io/sys.hh>

void VideoImageEffect::setEffect(uint effect)
{
	effect_ = effect;
	iterateTimes(2, i)
	{
		prog[i].deinit();
		if(fShader[i])
		{
			Gfx::deleteShader(fShader[i]);
			fShader[i] = 0;
		}
	}
}

uint VideoImageEffect::effect()
{
	return effect_;
}

void VideoImageEffect::compile(const Gfx::BufferImage &img)
{
	if(hasProgram())
		return; // already compiled
	switch(effect_)
	{
		bcase HQ2X:
		{
			logMsg("compiling effect HQ2X");
			auto vShader = Gfx::makeDefaultVShader();
			{
				auto file = IOFile(openAppAssetIo("hq2x-f.txt"));
				assert(file);
				auto fileSize = file.size();
				char text[fileSize + 1];
				file.read(text, fileSize);
				text[fileSize] = 0;
				file.close();
				//logMsg("read source:\n%s", text);
				fShader[0] = Gfx::makePluginShader(text, Gfx::SHADER_FRAGMENT, Gfx::IMG_MODE_MODULATE, img);
				fShader[1] = Gfx::makePluginShader(text, Gfx::SHADER_FRAGMENT, Gfx::IMG_MODE_REPLACE, img);
			}
			iterateTimes(2, i)
			{
				prog[i].init(vShader, fShader[i], false, true);
				prog[i].link();
				texDeltaU[i] = prog[i].uniformLocation("texDelta");
			}
			Gfx::autoReleaseShaderCompiler();
		}
		bdefault:
			break;
	}
}

void VideoImageEffect::place(const IG::Pixmap &pix)
{
	if(!hasProgram())
		return;
	iterateTimes(2, i)
	{
		setProgram(prog[i]);
		Gfx::uniformF(texDeltaU[i], 0.5f * (1.0f / (float)pix.x), 0.5f * (1.0f / (float)pix.y));
	}
}

Gfx::Program &VideoImageEffect::program(uint imgMode)
{
	return prog[(imgMode == Gfx::IMG_MODE_MODULATE) ? 0 : 1];
}

bool VideoImageEffect::hasProgram() const
{
	return prog[0];
}

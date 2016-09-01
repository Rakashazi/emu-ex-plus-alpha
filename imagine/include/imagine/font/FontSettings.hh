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
#include <imagine/util/operators.hh>

namespace IG
{

class FontSettings : public NotEquals<FontSettings>
{
public:
	constexpr FontSettings() {}
	constexpr FontSettings(int pixelWidth, int pixelHeight) : pixWidth{pixelWidth}, pixHeight{pixelHeight} {}
	constexpr FontSettings(int pixelHeight) : FontSettings{0, pixelHeight} {}
	operator bool() const;
	int pixelWidth() const;
	int pixelHeight() const;
	void setPixelWidth(int w);
	void setPixelHeight(int h);

	bool operator==(const FontSettings& other) const
	{
		return pixHeight == other.pixHeight
			&& pixWidth == other.pixWidth;
	}


private:
	int pixWidth = 0, pixHeight = 0;
};

}

#pragma once

#include <engine-globals.h>

#ifdef CONFIG_RESOURCE_FONT_FREETYPE
	#include <ft2build.h>
	#include FT_FREETYPE_H

	static constexpr uint MAX_FREETYPE_SLOTS = Config::envIsLinux ? 2 : 1;
	struct FontSizeRef
	{
		FT_Size size[MAX_FREETYPE_SLOTS] {nullptr};
	};
#else
	struct FontSizeRef
	{
		void* ptr = nullptr;
	};
#endif

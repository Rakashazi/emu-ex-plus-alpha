#pragma once

#include <engine-globals.h>
#include <data-type/font/FreetypeFontData.hh>
#include <resource2/font/ResourceFont.h>
#include <io/sys.hh>

class ResourceFontFreetype : public ResourceFont
{
public:
	static ResourceFontFreetype * load(Io *io);
	static ResourceFontFreetype * load(const char *name);
	static ResourceFontFreetype * loadAsset(const char *name)
	{
		return load(openAppAssetIo(name));
	}

	void free() override;
	void charBitmap(void *&bitmap, int &x, int &y, int &pitch) override;
	CallResult activeChar(int idx, GlyphMetrics &metrics) override;
	//int currentFaceDescender() const override;
	//int currentFaceAscender() const override;
	CallResult newSize(FontSettings *settings, FontSizeRef &sizeRef) override;
	CallResult applySize(FontSizeRef &sizeRef) override;
	void freeSize(FontSizeRef &sizeRef) override;
	CallResult loadIntoSlot(Io *io, uint slot);
	CallResult loadIntoSlot(const char *name, uint slot);
private:
	FreetypeFontData f[MAX_FREETYPE_SLOTS];
	uint currCharSlot = 0;
	static ResourceFontFreetype *loadWithIoWithName (Io *io, const char *name);
};



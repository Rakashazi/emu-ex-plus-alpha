#pragma once
#include <engine-globals.h>

#include <resource2/font/ResourceFont.h>
#include <resource2/font/common/glyphTable.h>
#define RESOURCE_FACE_SETTINGS_UNCHANGED 128
#include <io/sys.hh>
#include <pixmap/Pixmap.hh>
#include <data-type/image/GfxImageSource.hh>

class ResourceFace
{
public:
	constexpr ResourceFace() {}
	static ResourceFace *create(ResourceFont *font, FontSettings *set = nullptr);
	static ResourceFace *create(ResourceFace *face, FontSettings *set = nullptr);
	static ResourceFace *load(const char *path, FontSettings *set = nullptr);
	static ResourceFace *load(Io* io, FontSettings *set = nullptr);
	static ResourceFace *loadAsset(const char *name, FontSettings *set = nullptr)
	{
		return load(openAppAssetIo(name), set);
	}
	static ResourceFace *loadSystem(FontSettings *set = nullptr);
	void free();
	CallResult applySettings(FontSettings set);
	//int maxDescender();
	//int maxAscender();
	CallResult writeCurrentChar(Pixmap &out);
	CallResult precache(const char *string);
	CallResult precacheAlphaNum()
	{
		return precache("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
	}
	GlyphEntry *glyphEntry(int c);
	uint nominalHeight() const;

	FontSettings settings;
	static constexpr bool supportsUnicode = Config::UNICODE_CHARS;
private:
	ResourceFont *font = nullptr;
	GlyphEntry *glyphTable = nullptr;
	FontSizeRef faceSize;
	uint nominalHeight_ = 0;

	void calcNominalHeight();
	void initGlyphTable ();
	CallResult cacheChar (int c, int tableIdx);
};

class GfxGlyphImage : public GfxImageSource
{
public:
	GfxGlyphImage(ResourceFace *face, GlyphEntry *entry): face(face), entry(entry) {}

	CallResult getImage(Pixmap &dest) override { return face->writeCurrentChar(dest); }
	uint width() override { return entry->metrics.xSize; }
	uint height() override { return entry->metrics.ySize; }
	const PixelFormatDesc *pixelFormat() override { return &PixelFormatI8; }

	ResourceFace *face;
	GlyphEntry *entry;
};

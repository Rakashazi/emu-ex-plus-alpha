#pragma once
#include <engine-globals.h>

#include <resource2/font/ResourceFont.h>
#include <resource2/font/common/glyphTable.h>
#define RESOURCE_FACE_SETTINGS_UNCHANGED 128
#include <io/sys.hh>
#include <pixmap/Pixmap.hh>

class ResourceFace
{
public:
	constexpr ResourceFace() { }
	static ResourceFace * create(ResourceFont *font, FontSettings *set = 0);
	static ResourceFace * create(ResourceFace *face, FontSettings *set = 0);
	static ResourceFace * load(const char* path, FontSettings * set = 0);
	static ResourceFace * load(Io* io, FontSettings * set = 0);
	static ResourceFace * loadAsset(const char * name, FontSettings * set = 0)
	{
		return load(openAppAssetIo(name), set);
	}
	void free();
	CallResult applySettings(FontSettings set);
	int maxDescender() const;
	int maxAscender() const;
	CallResult writeCurrentChar(Pixmap * out);
	CallResult precache(const char * string);
	CallResult precacheAlphaNum()
	{
		return precache("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
	}
	CallResult getGlyph(ResourceImageGlyph * * glyphAddr, int c);
	void lookupCharBounds(int c, int * width, int * height, int * yLineOffset, int * xOffset, int * xAdvance);
	GlyphEntry * glyphEntry(int c);
	uint nominalHeight() const;

	FontSettings settings;
private:
	ResourceFont *font = nullptr;
	GlyphEntry *glyphTable = nullptr;
	FaceSizeData faceSize = nullptr;
	uint nominalHeight_ = 0;

	void calcNominalHeight();
	void initGlyphTable ();
	CallResult cacheChar (int c, int tableIdx);
};

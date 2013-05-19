#pragma once

#include <engine-globals.h>
#include <resource2/font/ResourceFont.h>
#include <io/sys.hh>
#include <jni.h>

class ResourceFontAndroid : public ResourceFont
{
public:
	static ResourceFont * loadSystem();

	void free() override;
	void charBitmap(void *&bitmap, int &x, int &y, int &pitch) override;
	CallResult activeChar(int idx, GlyphMetrics &metrics) override;
	void unlockCharBitmap(void *data) override;
	//int currentFaceDescender() const override;
	//int currentFaceAscender() const override;
	CallResult newSize(const FontSettings &settings, FontSizeRef &sizeRef) override;
	CallResult applySize(FontSizeRef &sizeRef) override;
	void freeSize(FontSizeRef &sizeRef) override;

private:
	jobject renderer = nullptr, lockedBitmap = nullptr;
};

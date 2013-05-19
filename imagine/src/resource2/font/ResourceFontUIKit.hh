#pragma once

#include <engine-globals.h>
#include <resource2/font/ResourceFont.h>
#include <io/sys.hh>

#ifdef __OBJC__
	#import <UIKit/UIFont.h>
#else
	struct UIFont;
#endif

#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <CoreFoundation/CFBase.h>
#undef Fixed
#undef Rect

class ResourceFontUIKit : public ResourceFont
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
	UniChar currChar = 0;
	uchar *pixBuffer = nullptr, *startOfCharInPixBuffer = nullptr;
	UIFont *activeFont = nullptr;
	uint cXFullSize = 0, cYFullSize = 0;
	uint cXSize = 0, cYSize = 0; // size of the just the glyph pixels
};

#pragma once

#include "OSystem.hxx"

OSystem::OSystem()
{
	mySettings = nullptr;
	myFrameBuffer = new FrameBuffer();
	vcsSound = new ImagineSound(this);
	vcsSound->tiaSound().outputFrequency(tiaSoundRate);
	mySound = vcsSound;
	mySerialPort = new SerialPort();
	myEventHandler = new EventHandler(this);
	myPropSet = new PropertiesSet(this);
	Paddles::setDigitalSensitivity(5);
	Paddles::setMouseSensitivity(7);
}

const std::string& OSystem::nvramDir() const
{
	static string dir;
	dir = EmuSystem::savePath();
	return dir;
}

#ifndef NDEBUG
void OSystem::logMessage(const string& message, uInt8 level)
{
	logMsg("%s", message.c_str());
}
#endif

void OSystem::setDefaultJoymap(Event::Type event, EventMode mode) {}

void OSystem::setFramerate(float framerate) {}

uInt64 OSystem::getTicks() const
{
	assert(::console);
	return ::console->tia().frameCounter() * 16666;
}

EventHandler::EventHandler(OSystem*) {}

FrameBuffer::FrameBuffer() {}

FBInitStatus FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
	logMsg("called FrameBuffer::initialize, %d,%d", width, height);
	return kSuccess;
}

void FrameBuffer::refresh() {}

void FrameBuffer::showFrameStats(bool enable) {}

void FrameBuffer::enablePhosphor(bool enable, int blend)
{
	myUsePhosphor = enable;
	myPhosphorBlend = blend;
}

uint8 FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2) const
{
  if(c2 > c1)
    std::swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

void FrameBuffer::setTIAPalette(const uInt32* palette)
{
	logMsg("setTIAPalette");
	iterateTimes(256, i)
	{
		uint8 r = (palette[i] >> 16) & 0xff;
		uint8 g = (palette[i] >> 8) & 0xff;
		uint8 b = palette[i] & 0xff;

		// RGB 565
		tiaColorMap[i] = emuVideo.vidPix.format.build(r >> 3, g >> 2, b >> 3, 0);
	}

	iterateTimes(256, i)
	{
		iterateTimes(256, j)
		{
			uint8 ri = (palette[i] >> 16) & 0xff;
			uint8 gi = (palette[i] >> 8) & 0xff;
			uint8 bi = palette[i] & 0xff;
			uint8 rj = (palette[j] >> 16) & 0xff;
			uint8 gj = (palette[j] >> 8) & 0xff;
			uint8 bj = palette[j] & 0xff;

			uint8 r = getPhosphor(ri, rj);
			uint8 g = getPhosphor(gi, gj);
			uint8 b = getPhosphor(bi, bj);

			// RGB 565
			tiaPhosphorColorMap[i][j] = emuVideo.vidPix.format.build(r >> 3, g >> 2, b >> 3, 0);
		}
	}
}

#pragma once

OSystem::OSystem()
{
	myEEPROMDir = ".";
	mySettings = 0;
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

OSystem::~OSystem()
{

}

#ifndef NDEBUG
void OSystem::logMessage(const string& message, uInt8 level)
{
	logMsg("%s", message.c_str());
}
#endif

void OSystem::setFramerate(float framerate) { }

uInt64 OSystem::getTicks() const
{
  // Gettimeofday natively refers to the UNIX epoch (a set time in the past)
  timeval now;
  gettimeofday(&now, 0);

  return uInt64(now.tv_sec) * 1000000 + now.tv_usec;
}

EventHandler::EventHandler(OSystem*)
{
	myEvent = new Event();
}

EventHandler::~EventHandler()
{
	delete myEvent;
}

FrameBuffer::FrameBuffer() { }

FBInitStatus FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
	logMsg("called FrameBuffer::initialize, %d,%d", width, height);
	return kSuccess;
}

void FrameBuffer::refresh() { }

void FrameBuffer::showFrameStats(bool enable) { }

void FrameBuffer::setTIAPalette(const uInt32* palette)
{
	logMsg("setTIAPalette");
	iterateTimes(256, i)
	{
		uint8 r = (palette[i] >> 16) & 0xff;
		uint8 g = (palette[i] >> 8) & 0xff;
		uint8 b = palette[i] & 0xff;

		// RGB 565
		tiaColorMap[i] = pixFmt->build(r >> 3, g >> 2, b >> 3, 0);
	}
}

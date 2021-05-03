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

#define LOGTAG "CoreAudio"
#include <imagine/audio/coreaudio/CAOutputStream.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <TargetConditionals.h>

namespace IG::Audio
{

CAOutputStream::CAOutputStream()
{
	logMsg("setting up playback audio unit");
	AudioComponentDescription defaultOutputDescription
	{
		.componentType = kAudioUnitType_Output,
		#if TARGET_OS_IPHONE
		.componentSubType = kAudioUnitSubType_RemoteIO,
		#else
		.componentSubType = kAudioUnitSubType_DefaultOutput,
		#endif
		.componentManufacturer = kAudioUnitManufacturer_Apple,
	};
	AudioComponent defaultOutput = AudioComponentFindNext(nullptr, &defaultOutputDescription);
	assert(defaultOutput);
	auto err = AudioComponentInstanceNew(defaultOutput, &outputUnit);
	if(!outputUnit)
	{
		bug_unreachable("error creating output unit: %d", (int)err);
	}
	AURenderCallbackStruct renderCallbackProp
	{
		[](void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) -> OSStatus
		{
			auto thisPtr = static_cast<CAOutputStream*>(inRefCon);
			auto *buff = ioData->mBuffers[0].mData;
			if(!thisPtr->onSamplesNeeded(buff, inNumberFrames))
			{
				*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
			}
			return 0;
		},
		this
	};
	err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
	    0, &renderCallbackProp, sizeof(renderCallbackProp));
	if(err)
	{
		bug_unreachable("error setting callback: %d", (int)err);
	}
}

CAOutputStream::~CAOutputStream()
{
	if(!outputUnit)
		return;
	close();
	AudioComponentInstanceDispose(outputUnit);
}

IG::ErrorCode CAOutputStream::open(OutputStreamConfig config)
{
	if(isOpen())
	{
		logWarn("audio unit already open");
		return {};
	}
	auto format = config.format();
	AudioFormatFlags formatFlags = format.sample.isFloat() ? kAudioFormatFlagIsFloat : kLinearPCMFormatFlagIsSignedInteger;
	streamFormat.mSampleRate = format.rate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = formatFlags | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
	streamFormat.mBytesPerPacket = format.framesToBytes(1);
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mBytesPerFrame = format.framesToBytes(1);
	streamFormat.mChannelsPerFrame = format.channels;
	streamFormat.mBitsPerChannel = format.sample.bits();
	logMsg("creating unit %dHz %d channels", (int)streamFormat.mSampleRate, (int)streamFormat.mChannelsPerFrame);
	if(auto err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
			0, &streamFormat, sizeof(AudioStreamBasicDescription));
		err)
	{
		logErr("error %d setting stream format", (int)err);
		return {EINVAL};
	}
	onSamplesNeeded = config.onSamplesNeeded();
	AudioUnitInitialize(outputUnit);
	isOpen_ = true;
	if(config.startPlaying())
		play();
	return {};
}

void CAOutputStream::play()
{
	if(!isOpen() || isPlaying_) [[unlikely]]
		return;
	if(auto err = AudioOutputUnitStart(outputUnit);
		err)
	{
		logErr("error %d in AudioOutputUnitStart", (int)err);
	}
	else
		isPlaying_ = true;
}

void CAOutputStream::pause()
{
	if(!isOpen()) [[unlikely]]
		return;
	AudioOutputUnitStop(outputUnit);
	isPlaying_ = false;
}

void CAOutputStream::close()
{
	if(!isOpen())
	{
		logWarn("audio unit already closed");
		return;
	}
	AudioOutputUnitStop(outputUnit);
	AudioUnitUninitialize(outputUnit);
	isPlaying_ = false;
	isOpen_ = false;
	logMsg("closed audio unit");
}

void CAOutputStream::flush()
{
	if(!isOpen()) [[unlikely]]
		return;
	// TODO
	pause();
}

bool CAOutputStream::isOpen()
{
	return isOpen_;
}

bool CAOutputStream::isPlaying()
{
	return isPlaying_;
}

CAOutputStream::operator bool() const
{
	return outputUnit;
}

}

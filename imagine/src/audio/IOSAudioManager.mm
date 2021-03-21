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

#import <AVFoundation/AVFoundation.h>
#include <imagine/audio/AudioManager.hh>
#include <imagine/audio/defs.hh>
#include <imagine/logger/logger.h>
#include "../base/iphone/ios.hh"

static void handleEndInterruption()
{
	logMsg("re-activating audio session");
	[[AVAudioSession sharedInstance] setActive:YES error:nil];
}

#if __IPHONE_OS_VERSION_MIN_REQUIRED < 60000
@interface MainApp (AudioManager) <AVAudioSessionDelegate>
{}
@end

@implementation MainApp (AudioManager)

- (void)endInterruptionWithFlags:(NSUInteger)flags
{
	handleEndInterruption();
}

@end
#endif

namespace IG::AudioManager
{

static bool soloMix_ = true;
static bool sessionActive = false;
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000
static id sessionInterruptionObserver = nil;
#endif

Audio::SampleFormat nativeSampleFormat(Base::ApplicationContext)
{
	return Audio::SampleFormats::f32;
}

uint32_t nativeRate(Base::ApplicationContext)
{
	return 44100;
}

Audio::Format nativeFormat(Base::ApplicationContext app)
{
	return {nativeRate(app), nativeSampleFormat(app), 2};
}

void setSoloMix(Base::ApplicationContext, bool newSoloMix)
{
	if(soloMix_ != newSoloMix)
	{
		logMsg("setting solo mix: %d", newSoloMix);
		soloMix_ = newSoloMix;
		auto category = newSoloMix ? AVAudioSessionCategorySoloAmbient : AVAudioSessionCategoryAmbient;
		[[AVAudioSession sharedInstance] setCategory:category error:nil];
	}
}

bool soloMix()
{
	return soloMix_;
}

void setMusicVolumeControlHint(Base::ApplicationContext) {}

void startSession(Base::ApplicationContext)
{
	if(sessionActive)
		return;
	AVAudioSession *session = [AVAudioSession sharedInstance];
	if(![session setActive:YES error:nil])
	{
		logWarn("error in setActive()");
	}
	#if __IPHONE_OS_VERSION_MIN_REQUIRED < 60000
	session.delegate = Base::mainApp;
	#else
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	sessionInterruptionObserver = [center addObserverForName:AVAudioSessionInterruptionNotification object:nil
		queue:nil usingBlock:
		^(NSNotification *note)
		{
			auto type = [note.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
			if(type == AVAudioSessionInterruptionTypeEnded)
			{
				handleEndInterruption();
			}
		}];
	#endif
	sessionActive = true;
}

void endSession(Base::ApplicationContext)
{
	if(!sessionActive)
		return;
	AVAudioSession *session = [AVAudioSession sharedInstance];
	if(![session setActive:NO error:nil])
	{
		logWarn("error in setActive()");
	}
	#if __IPHONE_OS_VERSION_MIN_REQUIRED < 60000
	session.delegate = nil;
	#else
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center removeObserver:sessionInterruptionObserver];
	sessionInterruptionObserver = nil;
	#endif
	sessionActive = false;
}

}

namespace IG::Audio
{

std::vector<ApiDesc> audioAPIs(Base::ApplicationContext)
{
	return {{"Core Audio", Api::COREAUDIO}};
}

Api makeValidAPI(Base::ApplicationContext, Api api)
{
	return Api::COREAUDIO;
}

}

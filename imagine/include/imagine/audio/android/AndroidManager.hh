#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/jni.hh>

namespace IG::Audio
{

class AndroidManager
{
public:
	static constexpr bool HAS_SOLO_MIX = true;
	static constexpr bool SOLO_MIX_DEFAULT = true;

	AndroidManager(ApplicationContext);
	bool hasFloatFormat() const;
	bool hasStreamUsage() const;
	int defaultOutputBuffers() const;
	int nativeOutputFramesPerBuffer() const;

protected:
	ApplicationContext ctx;
	JNI::UniqueGlobalRef audioManager{};
	JNI::InstMethod<jint(jobject, jint, jint)> jRequestAudioFocus{};
	JNI::InstMethod<jint(jobject)> jAbandonAudioFocus{};
	JNI::InstMethod<jstring(jstring)> jGetProperty{};
	bool soloMix_{SOLO_MIX_DEFAULT};
	bool sessionActive{};

	int audioManagerIntProperty(JNIEnv*, const char *propStr) const;
	void requestAudioFocus(JNIEnv* env, jobject baseActivity) const;
};

using ManagerImpl = AndroidManager;

}

#pragma once

#if !defined __ARM_ARCH_6K__
#define CONFIG_BASE_IOS_GLKIT
#define CONFIG_BASE_IOS_RETINA_SCALE
#endif

namespace Config
{
	#ifdef CONFIG_BASE_IOS_GLKIT
	constexpr bool BASE_IOS_GLKIT = true;
	#else
	constexpr bool BASE_IOS_GLKIT = false;
	#endif
}

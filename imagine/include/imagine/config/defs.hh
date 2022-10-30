#pragma once

#ifdef IMAGINE_CONFIG_H
#define IMAGINE_CONFIG_H_INCLUDE <IMAGINE_CONFIG_H>
#include IMAGINE_CONFIG_H_INCLUDE
#undef IMAGINE_CONFIG_H_INCLUDE
#else
	#ifdef NDEBUG
		#if __has_include (<imagine-config.h>)
		#include <imagine-config.h>
		#endif
	#else
		#if __has_include (<imagine-debug-config.h>)
		#include <imagine-debug-config.h>
		#endif
	#endif
#endif

#include <imagine/config/env.hh>

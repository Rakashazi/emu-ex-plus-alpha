#pragma once

#ifdef IMAGINE_CONFIG_H
#define IMAGINE_CONFIG_H_INCLUDE <IMAGINE_CONFIG_H>
#include IMAGINE_CONFIG_H_INCLUDE
#undef IMAGINE_CONFIG_H_INCLUDE
#else
// fallback to default config header name
	#ifdef NDEBUG
	#include <imagine-config.h>
	#else
	#include <imagine-debug-config.h>
	#endif
#endif

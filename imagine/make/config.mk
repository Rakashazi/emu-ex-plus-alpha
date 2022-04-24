ifndef inc_makeConfig
inc_makeConfig := 1

# common build system functions

parentDir = $(patsubst %/,%,$(dir $1))
lastMakefileDir = $(call parentDir,$(lastword $(MAKEFILE_LIST)))
firstMakefileName := $(shell basename $(firstword $(MAKEFILE_LIST)))
firstMakefileDir := $(call parentDir,$(firstword $(MAKEFILE_LIST)))

IMAGINE_SDK_PATH ?= $(HOME)/imagine-sdk
IMAGINE_SDK_PLATFORM ?= $(ENV)-$(SUBARCH)
IMAGINE_SDK_PLATFORM_PATH ?= $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)
SUBARCH = $(ARCH)

ifeq ($(wildcard $(IMAGINE_PATH)/make)),)
 $(error Invalid Imagine path:$(IMAGINE_PATH), please set IMAGINE_PATH to the root path of the Imagine distribtion
endif

buildSysPath := $(IMAGINE_PATH)/make
projectPath ?= $(firstMakefileDir)

LN ?= ln
RANLIB ?= ranlib
CLANG_TIDY ?= clang-tidy

toolchainEnvParams = CC="$(CC)" CXX="$(CXX)" AR="$(AR)" RANLIB="$(RANLIB)"

ifndef V
 PRINT_CMD := @
endif

endif
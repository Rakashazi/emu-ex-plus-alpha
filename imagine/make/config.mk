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

buildSysPath := $(IMAGINE_PATH)/make
projectPath ?= $(firstMakefileDir)

LN ?= ln

ifndef V
 PRINT_CMD := @
endif

endif
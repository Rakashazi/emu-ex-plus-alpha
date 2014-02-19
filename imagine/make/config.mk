ifndef inc_makeConfig
inc_makeConfig := 1

# common build system functions

parentDir = $(patsubst %/,%,$(dir $1))
lastMakefileDir = $(call parentDir,$(lastword $(MAKEFILE_LIST)))
#$(call parentDir,$(firstword $(MAKEFILE_LIST)))
firstMakefileName := $(shell basename $(firstword $(MAKEFILE_LIST)))
firstMakefileDir := $(call parentDir,$(firstword $(MAKEFILE_LIST)))

buildSysPath := $(IMAGINE_PATH)/make
imagineSrcDir := $(IMAGINE_PATH)/src

projectPath ?= $(firstMakefileDir)

ifndef V
 PRINT_CMD := @
endif

endif
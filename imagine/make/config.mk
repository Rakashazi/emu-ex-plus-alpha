ifndef inc_makeConfig
inc_makeConfig := 1

# common build system functions

parentDir = $(patsubst %/,%,$(dir $1))
currPath = $(call parentDir,$(lastword $(MAKEFILE_LIST)))
# path of original makefile
basePath = $(CURDIR)
#$(call parentDir,$(firstword $(MAKEFILE_LIST)))
baseMakefileName = $(shell basename $(firstword $(MAKEFILE_LIST)))

buildSysPath := $(currPath)

ifndef V
 PRINT_CMD := @
endif

endif
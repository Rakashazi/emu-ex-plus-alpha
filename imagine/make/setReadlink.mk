# need GNU version of readlink for -f
# Note: if on MacOSX, install coreutils from MacPorts

osName := $(shell uname -s)

ifeq ($(osName),Darwin)
 READLINK ?= greadlink
else
 READLINK ?= readlink
endif

osName :=

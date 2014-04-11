ENV := win32
binStatic := 1

ifndef target
 target = $(metadata_exec)
endif

ifndef O_RELEASE
 targetExtension := -debug.exe
else
 targetExtension := .exe
endif

compiler_noSanitizeAddress := 1
include $(buildSysPath)/gcc.mk

#COMPILE_FLAGS += -ffunction-sections -fdata-sections
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDFLAGS += -mwindows -Wl,-O1,--gc-sections,--as-needed,--stack,16777216

#-municode
CPPFLAGS += -DWINVER=0x0501 -D_WIN32_WINNT=0x0501

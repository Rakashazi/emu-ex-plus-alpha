ENV := win32

ifndef target
 target = $(metadata_exec)
endif

ifndef O_RELEASE
 targetExtension := -debug.exe
else
 targetExtension := .exe
endif

include $(buildSysPath)/gcc.mk

ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDFLAGS_SYSTEM += -mwindows -Wl,-O1,--gc-sections,--as-needed,--stack,16777216

#-municode
CPPFLAGS += -DWINVER=0x0501 -D_WIN32_WINNT=0x0501

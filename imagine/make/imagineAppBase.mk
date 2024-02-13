CPPFLAGS += -I$(genPath)
VPATH += $(projectPath)/src

.SUFFIXES: 
.PHONY: all main config metadata-header
all : main

genMetaH = $(genPath)/meta.h
include $(projectPath)/metadata/conf.mk
$(genMetaH) :
	@echo "Generating Metadata Header $@"
	@mkdir -p $(@D)
	echo \#pragma once > $@
	echo \#define CONFIG_APP_NAME \"$(metadata_name)\" > $@
	echo \#define CONFIG_APP_ID \"$(metadata_id)\" >> $@
ifeq ($(ENV), android)
 ifdef ANDROID_GOOGLE_PLAY_STORE_BUILD
	echo \#define CONFIG_GOOGLE_PLAY_STORE >> $@
 endif
endif
ifeq ($(ENV), ps3)
	echo \#define CONFIG_PS3_PRODUCT_ID \"$(ps3_productid)\" >> $@
endif
metadata-header : $(genMetaH)

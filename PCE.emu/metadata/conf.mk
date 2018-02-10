include $(EMUFRAMEWORK_PATH)/metadata/conf.mk
metadata_name = PCE.emu
metadata_exec = pceemu
metadata_pkgName = PceEmu
metadata_supportedMIMETypes = application/zip
metadata_supportedFileExtensions = pce cue ccd
android_metadata_id = com.PceEmu
ifeq ($(ENV),android)
 metadata_id = $(android_metadata_id)
else
 metadata_id = com.explusalpha.PceEmu
endif
metadata_vendor = Robert Broglia
ps3_productid = PCEE00000
ps3_contentid = EM0000-$(ps3_productid)_00-EXPLUSALPHATURBO
pnd_metadata_description = TurboGrafx 16/PC-Engine emulator using components from Mednafen
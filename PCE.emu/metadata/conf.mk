include $(EMUFRAMEWORK_PATH)/metadata/conf.mk
metadata_name = PCE.emu
metadata_exec = pceemu
metadata_pkgName = PceEmu
metadata_supportedFileExtensions += pce cue ccd chd
android_metadata_id = com.PceEmu
ifeq ($(ENV),android)
 metadata_id = $(android_metadata_id)
else
 metadata_id = com.explusalpha.PceEmu
endif
metadata_vendor = Robert Broglia
pnd_metadata_description = TurboGrafx 16/PC-Engine emulator using components from Mednafen
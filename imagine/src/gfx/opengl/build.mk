ifndef inc_gfx
inc_gfx := 1

include $(imagineSrcDir)/base/system.mk
include $(imagineSrcDir)/pixmap/build.mk

configDefs += CONFIG_GFX CONFIG_GFX_OPENGL

include $(IMAGINE_PATH)/make/package/opengl.mk

SRC += gfx/opengl/opengl.cc

ifeq ($(ENV), android)
 ifeq ($(android_hasSDK9), 1)
  SRC += gfx/opengl/android/SurfaceTextureBufferImage.cc
 endif
 SRC += gfx/opengl/android/DirectTextureBufferImage.cc
endif

endif

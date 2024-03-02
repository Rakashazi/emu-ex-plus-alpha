ifndef inc_gfx
inc_gfx := 1

include $(imagineSrcDir)/base/system.mk
include $(imagineSrcDir)/pixmap/build.mk
include $(IMAGINE_PATH)/make/package/opengl.mk

SRC += gfx/common/GfxText.cc \
 gfx/common/GlyphTextureSet.cc \
 gfx/common/Mat4.cc \
 gfx/opengl/BasicEffect.cc \
 gfx/opengl/Buffer.cc \
 gfx/opengl/DrawContextSupport.cc \
 gfx/opengl/GLStateCache.cc \
 gfx/opengl/GLTask.cc \
 gfx/opengl/PixmapBufferTexture.cc \
 gfx/opengl/Program.cc \
 gfx/opengl/Renderer.cc \
 gfx/opengl/RendererCommands.cc \
 gfx/opengl/RendererTask.cc \
 gfx/opengl/Texture.cc \
 gfx/opengl/TextureSampler.cc

ifeq ($(ENV), ios)
 SRC +=  gfx/opengl/ios/drawable.mm
endif

ifeq ($(ENV), android)
 SRC += gfx/opengl/android/HardwareBufferStorage.cc \
 gfx/opengl/android/SurfaceTextureStorage.cc
endif

endif

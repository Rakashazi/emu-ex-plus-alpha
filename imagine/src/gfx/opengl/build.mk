ifndef inc_gfx
inc_gfx := 1

include $(imagineSrcDir)/base/system.mk
include $(imagineSrcDir)/pixmap/build.mk

ifndef openGLAPI
 ifneq ($(filter ios android,$(ENV)),)
  openGLAPI := gles
 else ifeq ($(SUBENV), pandora)
  openGLAPI := gles
 else
  openGL := gl
 endif
endif

ifeq ($(openGLAPI), gles)
 ifndef openGLESVersion
  openGLESVersion := 2
  ifneq ($(filter ios android,$(ENV)),)
   ifeq ($(SUBARCH), armv6)
    openGLESVersion := 1
   endif
  endif
 endif
endif

include $(IMAGINE_PATH)/make/package/opengl.mk

SRC += gfx/common/GeomQuadMesh.cc \
 gfx/common/GfxLGradient.cc \
 gfx/common/GfxText.cc \
 gfx/common/GlyphTextureSet.cc \
 gfx/common/Mat4.cc \
 gfx/opengl/BasicEffect.cc \
 gfx/opengl/config.cc \
 gfx/opengl/debug.cc \
 gfx/opengl/GLStateCache.cc \
 gfx/opengl/GLTask.cc \
 gfx/opengl/PixmapBufferTexture.cc \
 gfx/opengl/Renderer.cc \
 gfx/opengl/RendererCommands.cc \
 gfx/opengl/RendererTask.cc \
 gfx/opengl/shader.cc \
 gfx/opengl/sync.cc \
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

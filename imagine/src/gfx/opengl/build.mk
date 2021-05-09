ifndef inc_gfx
inc_gfx := 1

include $(imagineSrcDir)/base/system.mk
include $(imagineSrcDir)/pixmap/build.mk

configDefs += CONFIG_GFX CONFIG_GFX_OPENGL

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

SRC += gfx/common/AnimatedViewport.cc \
 gfx/common/GeomQuad.cc \
 gfx/common/GeomQuadMesh.cc \
 gfx/common/GfxLGradient.cc \
 gfx/common/GfxText.cc \
 gfx/common/GlyphTextureSet.cc \
 gfx/common/ProjectionPlane.cc \
 gfx/common/Sprite.cc \
 gfx/opengl/config.cc \
 gfx/opengl/debug.cc \
 gfx/opengl/geometry.cc \
 gfx/opengl/GLStateCache.cc \
 gfx/opengl/GLTask.cc \
 gfx/opengl/PixmapBufferTexture.cc \
 gfx/opengl/Renderer.cc \
 gfx/opengl/RendererCommands.cc \
 gfx/opengl/RendererTask.cc \
 gfx/opengl/resource.cc \
 gfx/opengl/shader.cc \
 gfx/opengl/sync.cc \
 gfx/opengl/Texture.cc \
 gfx/opengl/TextureSampler.cc \
 gfx/opengl/transforms.cc \
 gfx/opengl/Viewport.cc

ifeq ($(ENV), ios)
 SRC +=  gfx/opengl/ios/drawable.mm
 ifneq ($(SUBARCH), armv6)
  include $(imagineSrcDir)/util/math/GLKit.mk
 else
  include $(imagineSrcDir)/util/math/GLM.mk
 endif
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/util/math/GLKit.mk
else
 include $(imagineSrcDir)/util/math/GLM.mk
endif

ifeq ($(ENV), android)
 SRC += gfx/opengl/android/HardwareBufferStorage.cc \
 gfx/opengl/android/SurfaceTextureStorage.cc
endif

endif

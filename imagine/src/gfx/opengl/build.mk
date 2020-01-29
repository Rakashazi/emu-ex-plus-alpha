ifndef inc_gfx
inc_gfx := 1

include $(imagineSrcDir)/base/system.mk
include $(imagineSrcDir)/pixmap/build.mk

configDefs += CONFIG_GFX CONFIG_GFX_OPENGL

ifndef openGLAPI
 ifneq ($(filter ios android webos,$(ENV)),)
  openGLAPI := gles
 else ifeq ($(SUBENV), pandora)
  openGLAPI := gles
 else
  openGL := gl
 endif
endif

ifeq ($(openGLAPI), gles)
 ifndef openGLESVersion
  ifneq ($(filter ios android webos,$(ENV)),)
   ifeq ($(SUBARCH), armv6)
    openGLESVersion := 1
   else
    openGLESVersion := 2
   endif
  else
   openGLESVersion := 2
  endif
 endif
endif

ifeq ($(openGLESVersion), 2) # ES 2.0 back-end cannot fallback to fixed function
 openGLFixedFunctionPipeline := 0
endif

ifndef openGLFixedFunctionPipeline
 openGLFixedFunctionPipeline := 1
endif

include $(IMAGINE_PATH)/make/package/opengl.mk

SRC += gfx/opengl/opengl.cc \
 gfx/opengl/transforms.cc \
 gfx/opengl/config.cc \
 gfx/opengl/shader.cc \
 gfx/opengl/GLMainTask.cc \
 gfx/opengl/GLStateCache.cc \
 gfx/opengl/RendererTask.cc \
 gfx/opengl/Texture.cc \
 gfx/opengl/TextureSampler.cc \
 gfx/opengl/geometry.cc \
 gfx/opengl/GeomQuadMesh.cc \
 gfx/opengl/Viewport.cc \
 gfx/opengl/resource.cc \
 gfx/opengl/RendererCommands.cc \
 gfx/common/ProjectionPlane.cc \
 gfx/common/GfxText.cc \
 gfx/common/GlyphTextureSet.cc \
 gfx/common/AnimatedViewport.cc \
 gfx/common/GfxLGradient.cc
 
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
 SRC += gfx/opengl/android/SurfaceTextureStorage.cc gfx/opengl/android/GraphicBufferStorage.cc
endif

endif

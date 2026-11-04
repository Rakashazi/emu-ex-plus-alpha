target_sources(s9xp PRIVATE FILE_SET CXX_MODULES
	BASE_DIRS main
	FILES main/system.ccm
)

target_sources(s9xp PRIVATE
	main/Main.cc
	main/AppMeta.cc
	main/input.cc
	main/options.cc
	main/S9XApi.cc
	main/EmuMenuViews.cc
	main/Cheats.cc
	snes9x/bsx.cpp
	snes9x/bml.cpp
	snes9x/c4.cpp
	snes9x/c4emu.cpp
	snes9x/cheats.cpp
	snes9x/cheats2.cpp
	snes9x/clip.cpp
	snes9x/controls.cpp
	snes9x/cpu.cpp
	snes9x/cpuexec.cpp
	snes9x/cpuops.cpp
	snes9x/dma.cpp
	snes9x/dsp.cpp
	snes9x/dsp1.cpp
	snes9x/dsp2.cpp
	snes9x/dsp3.cpp
	snes9x/dsp4.cpp
	snes9x/fxemu.cpp
	snes9x/fxinst.cpp
	snes9x/gfx.cpp
	snes9x/globals.cpp
	snes9x/loadzip.cpp
	snes9x/memmap.cpp
	snes9x/msu1.cpp
	snes9x/movie.cpp
	snes9x/obc1.cpp
	snes9x/ppu.cpp
	snes9x/stream.cpp
	snes9x/sa1.cpp
	snes9x/sa1cpu.cpp
	snes9x/sdd1.cpp
	snes9x/sdd1emu.cpp
	snes9x/seta.cpp
	snes9x/seta010.cpp
	snes9x/seta011.cpp
	snes9x/seta018.cpp
	snes9x/sha256.cpp
	snes9x/snapshot.cpp
	snes9x/spc7110.cpp
	snes9x/srtc.cpp
	snes9x/tile.cpp
	snes9x/tileimpl-h2x1.cpp
	snes9x/tileimpl-n1x1.cpp
	snes9x/tileimpl-n2x1.cpp
	snes9x/apu/apu.cpp
	snes9x/apu/bapu/dsp/sdsp.cpp
	snes9x/apu/bapu/smp/smp.cpp
	snes9x/apu/bapu/smp/smp_state.cpp
)
option(SNES9X_ANDROID_BRIDGE "Enable the experimental local Android memory/controller bridge" OFF)
if(SNES9X_ANDROID_BRIDGE)
 if(NOT ENV STREQUAL android)
  message(FATAL_ERROR "SNES9X_ANDROID_BRIDGE requires Android")
 endif()
 target_sources(s9xp PRIVATE bridge/Bridge.cc)
 target_compile_definitions(s9xp PRIVATE SNES9X_ANDROID_BRIDGE=1)
 # Keep stack protection on our JNI boundary even when upstream's optimized defaults disable it.
 set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/bridge/Bridge.cc" TARGET_DIRECTORY s9xp
  PROPERTIES COMPILE_OPTIONS "-fstack-protector-strong")
 foreach(config debug release)
  file(APPEND "${CMAKE_BINARY_DIR}/s9xp_${config}_retainedSymbols.txt"
   "Java_org_snes9x_bridge_NativeBridge_info\nJava_org_snes9x_bridge_NativeBridge_read\nJava_org_snes9x_bridge_NativeBridge_queue\nJava_org_snes9x_bridge_NativeBridge_command\n")
 endforeach()
endif()

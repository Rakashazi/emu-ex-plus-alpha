ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CPPFLAGS += -I$(projectPath)/src \
-DBSPF_UNIX \
-DTHUMB_SUPPORT \
-I$(projectPath)/src/stella/emucore \
-I$(projectPath)/src/stella/emucore/tia \
-I$(projectPath)/src/stella/common \
-I$(projectPath)/src/stella/common/tv_filters \
-I$(projectPath)/src/stella/gui

stellaSrc := Console.cxx \
Cart.cxx \
Props.cxx \
MD5.cxx \
Settings.cxx \
Serializer.cxx \
System.cxx \
Control.cxx \
Booster.cxx \
Driving.cxx \
SaveKey.cxx \
KidVid.cxx \
AtariVox.cxx \
MT24LC256.cxx \
Switches.cxx \
M6502.cxx \
M6532.cxx \
TIASnd.cxx \
PropsSet.cxx \
Keyboard.cxx \
Paddles.cxx \
PointingDevice.cxx \
Joystick.cxx \
Genesis.cxx \
Cart2K.cxx \
Cart3E.cxx \
Cart3EPlus.cxx \
Cart4A50.cxx \
Cart3F.cxx \
CartAR.cxx \
Cart4K.cxx \
CartDetector.cxx \
CartDPC.cxx \
CartDPCPlus.cxx \
CartE0.cxx \
CartE7.cxx \
CartEF.cxx \
CartEFSC.cxx \
CartE78K.cxx \
CartF4.cxx \
CartF4SC.cxx \
CartF6.cxx \
CartF6SC.cxx \
CartF8.cxx \
CartF8SC.cxx \
CartFA.cxx \
CartFE.cxx \
CartF0.cxx \
CartCV.cxx \
CartCVPlus.cxx \
CartUA.cxx \
Cart0840.cxx \
CartSB.cxx \
CartX07.cxx \
CartFA2.cxx \
CartCM.cxx \
MindLink.cxx \
CartCTY.cxx \
CartCDF.cxx \
Cart4KSC.cxx \
CompuMate.cxx \
Thumbulator.cxx \
CartDASH.cxx \
CartBF.cxx \
CartBFSC.cxx \
CartBUS.cxx \
CartDF.cxx \
CartDFSC.cxx \
CartMDM.cxx \
CartMNetwork.cxx \
CartWD.cxx \
tia/Background.cxx \
tia/Ball.cxx \
tia/DrawCounterDecodes.cxx \
tia/LatchedInput.cxx \
tia/Missile.cxx \
tia/PaddleReader.cxx \
tia/Player.cxx \
tia/Playfield.cxx \
tia/TIA.cxx \
tia/frame-manager/AbstractFrameManager.cxx \
tia/frame-manager/FrameLayoutDetector.cxx \
tia/frame-manager/FrameManager.cxx \
tia/frame-manager/JitterEmulation.cxx \
tia/frame-manager/YStartDetector.cxx

stellaPath := stella/emucore
SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/SoundGeneric.cc \
main/FrameBuffer.cc \
main/OSystem.cc \
stella/common/Base.cxx \
stella/common/RewindManager.cxx \
stella/common/StateManager.cxx \
$(addprefix $(stellaPath)/,$(stellaSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif

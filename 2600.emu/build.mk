ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CPPFLAGS += -I$(projectPath)/src \
-DEMU_EX_PLATFORM \
-DSOUND_SUPPORT \
-DTHUMB_SUPPORT \
-I$(projectPath)/src/stella/emucore \
-I$(projectPath)/src/stella/emucore/tia \
-I$(projectPath)/src/stella/common \
-I$(projectPath)/src/stella/common/tv_filters \
-I$(projectPath)/src/stella/gui

stellaSrc := AtariVox.cxx \
Bankswitch.cxx \
Booster.cxx \
Cart0840.cxx \
Cart2K.cxx \
Cart3E.cxx \
Cart3EPlus.cxx \
Cart3F.cxx \
Cart4A50.cxx \
Cart4K.cxx \
Cart4KSC.cxx \
CartAR.cxx \
CartBF.cxx \
CartBFSC.cxx \
CartBUS.cxx \
CartCDF.cxx \
CartCM.cxx \
CartCTY.cxx \
CartCV.cxx \
CartCVPlus.cxx \
Cart.cxx \
CartDASH.cxx \
CartDetector.cxx \
CartDF.cxx \
CartDFSC.cxx \
CartDPC.cxx \
CartDPCPlus.cxx \
CartE0.cxx \
CartE78K.cxx \
CartE7.cxx \
CartEF.cxx \
CartEFSC.cxx \
CartF0.cxx \
CartF4.cxx \
CartF4SC.cxx \
CartF6.cxx \
CartF6SC.cxx \
CartF8.cxx \
CartF8SC.cxx \
CartFA2.cxx \
CartFA.cxx \
CartFC.cxx \
CartFE.cxx \
CartMDM.cxx \
CartMNetwork.cxx \
CartSB.cxx \
CartUA.cxx \
CartWD.cxx \
CartX07.cxx \
CompuMate.cxx \
Console.cxx \
Control.cxx \
ControllerDetector.cxx \
DispatchResult.cxx \
Driving.cxx \
EmulationTiming.cxx \
FSNode.cxx \
Genesis.cxx \
Joystick.cxx \
Keyboard.cxx \
KidVid.cxx \
Lightgun.cxx \
M6502.cxx \
M6532.cxx \
MD5.cxx \
MindLink.cxx \
MT24LC256.cxx \
Paddles.cxx \
PointingDevice.cxx \
Props.cxx \
PropsSet.cxx \
SaveKey.cxx \
Serializer.cxx \
Settings.cxx \
Switches.cxx \
System.cxx \
Thumbulator.cxx \
tia/Audio.cxx \
tia/AudioChannel.cxx \
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
tia/frame-manager/JitterEmulation.cxx

stellaPath := stella/emucore
SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/SoundEmuEx.cc \
main/FrameBuffer.cc \
main/OSystem.cc \
main/FSNodeEmuEx.cc \
stella/common/AudioQueue.cxx \
stella/common/AudioSettings.cxx \
stella/common/Base.cxx \
stella/common/RewindManager.cxx \
stella/common/StateManager.cxx \
stella/common/TimerManager.cxx \
stella/common/audio/ConvolutionBuffer.cxx \
stella/common/audio/HighPass.cxx \
stella/common/audio/LanczosResampler.cxx \
stella/common/audio/SimpleResampler.cxx \
$(addprefix $(stellaPath)/,$(stellaSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif

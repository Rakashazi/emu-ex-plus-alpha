ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CPPFLAGS += -I$(projectPath)/src -DHAVE_INTTYPES -DBSPF_UNIX -DTHUMB_SUPPORT \
-DSTELLA_MINIMAL_SETTINGS -DSysDDec=float -I$(projectPath)/src/stella/common

stellaSrc := Console.cxx Cart.cxx Props.cxx MD5.cxx Settings.cxx \
Serializer.cxx System.cxx NullDev.cxx Random.cxx TIA.cxx \
TIATables.cxx Control.cxx Booster.cxx Driving.cxx SaveKey.cxx \
KidVid.cxx AtariVox.cxx MT24LC256.cxx Switches.cxx M6502.cxx \
M6532.cxx TIASnd.cxx StateManager.cxx PropsSet.cxx \
Keyboard.cxx Paddles.cxx TrackBall.cxx Joystick.cxx Genesis.cxx \
Cart2K.cxx Cart3E.cxx Cart4A50.cxx Cart3F.cxx CartAR.cxx \
Cart4K.cxx CartDPC.cxx CartDPCPlus.cxx CartE0.cxx CartE7.cxx \
CartEF.cxx CartEFSC.cxx CartF4.cxx CartF4SC.cxx CartF6.cxx \
CartF6SC.cxx CartF8.cxx CartF8SC.cxx CartFA.cxx CartFE.cxx \
CartMC.cxx CartF0.cxx CartCV.cxx CartUA.cxx Cart0840.cxx \
CartSB.cxx CartX07.cxx CartFA2.cxx CartCM.cxx MindLink.cxx \
CartCTY.cxx CompuMate.cxx Thumbulator.cxx

stellaPath := stella/emucore
SRC += main/Main.cc main/EmuControls.cc main/ImagineSound.cc $(addprefix $(stellaPath)/,$(stellaSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif

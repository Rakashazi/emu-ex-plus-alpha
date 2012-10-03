ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/ImagineSound.cc

include ../EmuFramework/common.mk

CPPFLAGS += -DHAVE_GETTIMEOFDAY -DHAVE_INTTYPES -DBSPF_UNIX -DNO_DUAL_FRAME_BUFFER -DTHUMB_SUPPORT -DSysDDec=float -Isrc/stella/common

# Stella sources
STELLA := stella/emucore
SRC += $(STELLA)/Console.cxx $(STELLA)/Cart.cxx $(STELLA)/Props.cxx $(STELLA)/MD5.cxx $(STELLA)/Settings.cxx \
$(STELLA)/Serializer.cxx $(STELLA)/System.cxx $(STELLA)/NullDev.cxx $(STELLA)/Random.cxx $(STELLA)/TIA.cxx \
$(STELLA)/TIATables.cxx $(STELLA)/Control.cxx $(STELLA)/Booster.cxx $(STELLA)/Driving.cxx $(STELLA)/SaveKey.cxx \
$(STELLA)/KidVid.cxx $(STELLA)/AtariVox.cxx $(STELLA)/MT24LC256.cxx $(STELLA)/Switches.cxx $(STELLA)/M6502.cxx \
$(STELLA)/M6532.cxx $(STELLA)/TIASnd.cxx $(STELLA)/StateManager.cxx $(STELLA)/PropsSet.cxx \
$(STELLA)/Keyboard.cxx $(STELLA)/Paddles.cxx $(STELLA)/TrackBall.cxx $(STELLA)/Joystick.cxx $(STELLA)/Genesis.cxx \
$(STELLA)/Cart2K.cxx $(STELLA)/Cart3E.cxx $(STELLA)/Cart4A50.cxx $(STELLA)/Cart3F.cxx $(STELLA)/CartAR.cxx \
$(STELLA)/Cart4K.cxx $(STELLA)/CartDPC.cxx $(STELLA)/CartDPCPlus.cxx $(STELLA)/CartE0.cxx $(STELLA)/CartE7.cxx \
$(STELLA)/CartEF.cxx $(STELLA)/CartEFSC.cxx $(STELLA)/CartF4.cxx $(STELLA)/CartF4SC.cxx $(STELLA)/CartF6.cxx \
$(STELLA)/CartF6SC.cxx $(STELLA)/CartF8.cxx $(STELLA)/CartF8SC.cxx $(STELLA)/CartFA.cxx $(STELLA)/CartFE.cxx \
$(STELLA)/CartMC.cxx $(STELLA)/CartF0.cxx $(STELLA)/CartCV.cxx $(STELLA)/CartUA.cxx $(STELLA)/Cart0840.cxx \
$(STELLA)/CartSB.cxx $(STELLA)/CartX07.cxx $(STELLA)/CartFA2.cxx $(STELLA)/CartCM.cxx $(STELLA)/MindLink.cxx \
$(STELLA)/CartCTY.cxx $(STELLA)/CompuMate.cxx $(STELLA)/Thumbulator.cxx

include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

ifndef target
target := 2600emu
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif

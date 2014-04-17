makefilesToRun='
	src/minizip/linux-armv7-pandora.mk
	
	src/boost/linux-armv7-pandora.mk
'
source runMakefiles.sh

runMakefiles $@


makefilesToRun='
	src/libpng/linux-x86.mk
	src/minizip/linux-x86.mk
'
source runMakefiles.sh

runMakefiles $@


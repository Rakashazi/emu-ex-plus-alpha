makefilesToRun='
	src/libpng/linux-x86_64.mk
	src/minizip/linux-x86_64.mk
'
source runMakefiles.sh

runMakefiles $@


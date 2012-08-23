CPPFLAGS += -DNOUNCRYPT -DNOCRYPT

outputObjs := $(objDir)/ioapi.o $(objDir)/unzip.o $(objDir)/zip.o

all : $(outputObjs)

install : $(outputObjs)
	@echo "Installing unzip to: $(installDir)"
	@mkdir -p $(installDir)/include/
	cp ioapi.h unzip.h zip.h $(installDir)/include/

.PHONY : all install

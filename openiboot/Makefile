ifeq ($(PLATFORM),IPHONE_2G)
	TARGET = iphone_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE_3G)
	TARGET = iphone_3g_openiboot.img3
endif

ifeq ($(PLATFORM),IPOD)
	TARGET = ipod_1g_openiboot.img3
endif

ifeq ($(PLATFORM),IPOD_2G)
	TARGET = ipod_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE_4)
	TARGET = iphone_4_openiboot.bin
	DFUREQ = true
endif

ifeq ($(PLATFORM),IPAD)
	TARGET = ipad_1g_openiboot.bin
	DFUREQ = true
endif

ifeq ($(INSTALLER),YES)
	TARGET +=-Installer
endif

ifeq ($(DEBUG),YES)
	TARGET +=-Debug
endif

all:
	scons $(TARGET)

clean:
	scons -c
	@rm *.o
	@cd ../utils/syringe; make clean
	@cd ../utils/oibc; make clean

install:
	@echo Building tools...
	@make -C ../utils/syringe &> /dev/null
	@if test $(DFUREQ); then \
		echo A4 device detected, injecting pois0n; \
		echo Please enter DFU mode now; \
		sudo ../utils/syringe/utilities/injectpois0n; \
	else \
		read -p "Put your device into recovery mode, then press enter to continue..." tmp; \
	fi
	@echo Uploading OpeniBoot binary
	@sudo ../utils/syringe/utilities/loadibec $(TARGET)
	@echo Done, OpeniBoot should be running

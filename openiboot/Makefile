ifeq ($(PLATFORM),IPHONE_2G)
	TARGET = iphone_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE_3G)
	TARGET = iphone_3g_openiboot.img3
endif

ifeq ($(PLATFORM),IPOD)
	TARGET = ipod_1g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE_4)
	TARGET = iphone_4_openiboot.bin
endif

ifeq ($(PLATFORM),IPAD)
	TARGET = ipad_1g_openiboot.bin
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
	@cd ../utils/syringe; make clean
	@cd ../utils/oibc; make clean

install:
	@echo Building tools...
	@make -C ../utils/syringe &> /dev/null
	@if test $(IMG3TEMPLATE) = "mk8900image/template-4g.img3"; then \
		echo A4 device detected, injecting pois0n; \
		echo Please enter DFU mode now; \
		sudo ../utils/syringe/injectpois0n; \
	else \
		read -p "Put your device into recovery mode, then press enter to continue..." tmp; \
	fi
	@echo Uploading OpeniBoot binary
	@sudo ../utils/syringe/loadibec openiboot.bin
	@echo Done, OpeniBoot should be running

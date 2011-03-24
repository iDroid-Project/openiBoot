ifeq ($(PLATFORM),IPHONE2G)
	TARGET = iphone_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE3G)
	TARGET = iphone_3g_openiboot.img3
endif

ifeq ($(PLATFORM),IPOD)
	TARGET = ipod_1g_openiboot.img3
endif

ifeq ($(PLATFORM),IPOD2G)
	TARGET = ipod_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE4)
	TARGET = iphone_4_openiboot.bin
endif

ifeq ($(PLATFORM),IPAD)
	TARGET = ipad_1g_openiboot.bin
endif

ifeq ($(PLATFORM),IPOD4G)
	TARGET = ipt_4g_openiboot.bin
endif

all:
	scons $(TARGET)

clean:
	scons -c
	@rm *.o
	@cd ../utils/syringe; make clean
	@cd ../utils/oibc; make clean

install:
	make -C ../utils/syringe
	sudo ../utils/syringe/utilities/loadibec $(TARGET)
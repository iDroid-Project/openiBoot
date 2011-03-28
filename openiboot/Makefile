ifeq ($(PLATFORM),IPHONE2G)
	TARGET = iphone_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE3G)
	TARGET = iphone_3g_openiboot.img3
endif

ifeq ($(PLATFORM),IPT1G)
	TARGET = ipt_1g_openiboot.img3
endif

ifeq ($(PLATFORM),IPT2G)
	TARGET = ipt_2g_openiboot.img3
endif

ifeq ($(PLATFORM),IPHONE4)
	TARGET = iphone_4_openiboot.bin
endif

ifeq ($(PLATFORM),IPAD1G)
	TARGET = ipad_1g_openiboot.bin
endif

ifeq ($(PLATFORM),IPT4G)
	TARGET = ipt_4g_openiboot.bin
endif

all:
	scons $(TARGET)

clean:
	scons -c
	@rm *.o
	@cd ../utils/syringe; make clean_all
	@cd ../utils/oibc; make clean

install:
	make -C ../utils/syringe
	sudo ../utils/syringe/utilities/loadibec $(TARGET)

# Standard things
.SUFFIXES:
.SUFFIXES:	.c .s .o

# Sources
SRC_C               = accel.c aes.c arm.c buttons.c chipid.c clock.c commands.c dma.c event.c framebuffer.c ftl.c gpio.c i2c.c images.c interrupt.c lcd.c malloc.c miu.c mmu.c nand.c nor.c nvram.c openiboot.c pmu.c power.c printf.c sdio.c sha1.c spi.c tasks.c timer.c uart.c usb.c util.c wdt.c wlan.c scripting.c syscfg.c actions.c
SRC_S               = entry.s openiboot-asmhelpers.s

HFS_SRC_C           = hfs/btree.c hfs/catalog.c hfs/extents.c hfs/fastunicodecompare.c hfs/rawfile.c hfs/utility.c hfs/volume.c hfs/bdev.c hfs/fs.c

MENU_SRC_C          = menu.c stb_image.c

ifeq ($(PLATFORM),IPHONE)
	SRC_C          += camera.c radio.c als.c multitouch.c wm8958.c vibrator.c
endif

ifeq ($(PLATFORM),3G)
	SRC_C          += camera.c radio.c alsISL29003.c multitouch-z2.c wm8991.c vibrator.c
endif

ifeq ($(PLATFORM),IPOD)
	SRC_C          += multitouch-z2.c wm8958.c piezo.c
endif

ifeq ($(PLATFORM),IPHONE_4)
	SRC_C          += camera.c radio.c alsISL29003.c multitouch-z2.c wm8991.c vibrator.c
endif

ifeq ($(PLATFORM),IPAD)
        SRC_C          += camera.c radio.c alsISL29003.c multitouch-z2.c wm8991.c vibrator.c
endif

# Variables
ifeq ($(SMALL),YES)
	LOAD           ?= 0x22000000
	CFLAGS         += -Os -DOpenIBootLoad=$(LOAD) -DNO_HFS -DNO_STBIMAGE -DSMALL
else
	LOAD           ?= 0x0
	CFLAGS         += -O3
endif

ARCHFLAGS          += -mlittle-endian -mfpu=vfp -mthumb -mthumb-interwork
LIBRARIES          += -lgcc -nostdlib
INC                += -Iincludes
CFLAGS             += -Wall -Werror
LDFLAGS            += -Ttext=$(LOAD) --nostdlib
VERSION             = "0.1.2"
COMMIT              = $(shell git log | head -n1 | cut -b8-14)
BUILD               = "$(COMMIT)"
CFLAGS             += -DOPENIBOOT_VERSION=$(VERSION) -DOPENIBOOT_VERSION_BUILD=$(BUILD)
DF                  = $(*F)
DEPDIR              = .deps
BUILDDIR            = .build
OBJS                = $(patsubst %.s,%.o,$(SRC_S)) $(patsubst %.c,%.o,$(SRC_C))

ifneq ($(SMALL),YES)
OBJS               += $(patsubst %.c,%.o,$(HFS_SRC_C)) $(patsubst %.c,%.o,$(MENU_SRC_C))
endif

ifeq ($(PLATFORM),IPOD)
	CFLAGS         += -DCONFIG_IPOD
	IMG3TEMPLATE    = mk8900image/template-ipod.img3
else
	ifeq ($(PLATFORM),3G)
		CFLAGS     += -DCONFIG_3G
		IMG3TEMPLATE    = mk8900image/template-3g.img3
	else
		ifeq ($(PLATFORM),IPHONE_4)
			CFLAGS	+= -DCONFIG_IPHONE_4
			IMG3TEMPLATE	= mk8900image/template-4g.img3
		else
	                ifeq ($(PLATFORM),IPAD)
	                        CFLAGS  += -DCONFIG_IPAD -DCONFIG_IPHONE_4
        	                IMG3TEMPLATE    = mk8900image/template-4g.img3
			else
				CFLAGS     += -DCONFIG_IPHONE
				IMG3TEMPLATE    = mk8900image/template.img3
			endif
		endif
	endif
endif

ifeq ($(DEBUG),YES)
	CFLAGS        += -DDEBUG
endif

# Tools
CROSS              ?= arm-elf-
CC                  = $(CROSS)gcc
OBJCOPY             = $(CROSS)objcopy
COMPILE             = $(CC) $(INC) $(CFLAGS) $(ARCHFLAGS) -c $< -o $@
MAKEDEPEND          = $(CC) -M $(INC) $(CFLAGS) $(ARCHFLAGS) -o "$(DEPDIR)/$*.d" $<

# Rules
.PHONY: clean all install

all:	openiboot.img2 openiboot-wtf.img2 openiboot.img3 openiboot.bin

%.o: %.S
	@echo "Compiling $<"
	@mkdir -p $(DEPDIR)/$(dir $@)
	@touch $(DEPDIR)/$(dir $@)/$(*F).d
	@$(COMPILE)

%.o: %.c
	@echo "Compiling $<"
	@mkdir -p $(DEPDIR)/$(dir $@)
	@$(MAKEDEPEND)
#	@cp $(DEPDIR)/$(dir $@)/$(*F).d $(DEPDIR)/$(dir $@)/$(*F).P
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $(DEPDIR)/$(dir $@)/$(*F).d >> $(DEPDIR)/$(dir $@)/$(*F).d
#	@rm -f $(DEPDIR)/$(dir $@)/$(*F).d
	@$(COMPILE)

-include $(addprefix $(DEPDIR)/,$(OBJS:.o=.d))

all-payload: openiboot-payload.img2 openiboot-payload.bin

openiboot: $(OBJS) $(SUBDIRS)
	@echo "Building $@"
	@$(CC) $(ARCHFLAGS) $(LDFLAGS) $(OBJS) $(LIBRARIES) -o $@

payload.o:	payload.bin
	$(OBJCOPY) -I binary -O elf32-little -B arm $< $@

openiboot-payload: $(OBJS) payload.o
	$(CC) --no-warn-mismatch $(LDFLAGS) $(OBJS) payload.o $(LIBRARIES) -o $@

mk8900image/mk8900image:
	cd mk8900image/; make

openiboot.bin: openiboot mk8900image/mk8900image
	@echo "Creating image $@"
	@mk8900image/mk8900image openiboot openiboot.bin

openiboot.img2:	openiboot mk8900image/mk8900image
	@echo "Creating image $@"
	@mk8900image/mk8900image openiboot openiboot.img2 mk8900image/template.img2 mk8900image/iphonelinux.der

openiboot-wtf.img2:	openiboot mk8900image/mk8900image
	@echo "Creating image $@"
	@mk8900image/mk8900image openiboot openiboot-wtf.img2 mk8900image/template-wtf.img2

openiboot.img3:	openiboot mk8900image/mk8900image
	@echo "Creating image $@"
	@mk8900image/mk8900image openiboot openiboot.img3 $(IMG3TEMPLATE)

openiboot-payload.bin: openiboot-payload mk8900image/mk8900image
	@echo "Creating image $@"
	@mk8900image/mk8900image openiboot-payload openiboot-payload.bin

openiboot-payload.img2: openiboot-payload mk8900image/mk8900image
	@echo "Creating image $@"
	@mk8900image/mk8900image openiboot-payload openiboot-payload.img2 mk8900image/template.img2 mk8900image/iphonelinux.der

#.PHONY: clean
clean:
	@rm -rf *.o $(DEPDIR) hfs/*.o openiboot openiboot.img2 openiboot-wtf.img2 openiboot.img3 openiboot-payload.bin openiboot-payload.img2
	@cd mk8900image/; $(MAKE) clean
	@cd ../tools/syringe; make clean
	@cd ../tools/client/linux; make clean
	@cd ../tools/libirecovery; make clean

install:
	@echo Building tools...
	@make -C ../tools/client/linux > /dev/null
	@make -C ../tools/libirecovery linux > /dev/null
	@if test $(IMG3TEMPLATE) = "mk8900image/template-4g.img3"; then \
		echo A4 device detected, building Syringe; \
		make -C ../tools/syringe > /dev/null; \
		echo Invoking syringe, enter DFU mode now; \
		sudo ../tools/syringe/injectpois0n; \
		sleep 5; \
	else \
		read -p "Put your device into recovery mode, then press enter to continue..." tmp; \
	fi
	@echo Uploading OpeniBoot binary
	@sudo ../tools/client/linux/loadibec openiboot.bin
	@sleep 3
	@echo Executing OpeniBoot binary 
	@sudo ../tools/libirecovery/irecovery -c "go jump 0x41000000"
	@echo Done, OpeniBoot should be running

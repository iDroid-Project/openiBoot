#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "commands.h"
#include "util.h"
#include "nor.h"
#include "lcd.h"
#include "images.h"
#include "timer.h"
#include "mmu.h"
#include "arm.h"
#include "gpio.h"
#include "framebuffer.h"
#include "actions.h"
#include "nvram.h"
#include "pmu.h"
#include "dma.h"
#include "nand.h"
#include "ftl.h"
#include "i2c.h"
#include "hfs/fs.h"
#include "aes.h"
#include "accel.h"
#include "sdio.h"
#include "wdt.h"
#include "wmcodec.h"
#include "multitouch.h"
#include "wlan.h"
#include "radio.h"
#include "als.h"
#include "piezo.h"
#include "vibrator.h"
#include "uart.h"
#include "hardware/radio.h"

int received_file_size; // Makes commands automatically use output from last command.

OPIBCommand *command_get_next(OIBCommandIterator *_it)
{
	if(*_it == NULL)
		*_it = &command_list_init;

	(*_it)++;
	return **_it;
}

int command_run(int argc, char **argv)
{
	OIBCommandIterator cmdIt = NULL;
	OPIBCommand *cmd;
	while((cmd = command_get_next(&cmdIt)))
	{
		if(strcmp(argv[0], cmd->name) == 0) {
			cmd->routine(argc, argv);
			return 0;
		}
	}

	return -1;
}

void cmd_help(int argc, char** argv)
{
	OIBCommandIterator it = NULL;
	OPIBCommand *cmd;
	while((cmd = command_get_next(&it)))
        bufferPrintf("%-20s%s\r\n", cmd->name, cmd->description);
}
COMMAND("help", "list the available commands", cmd_help);

void cmd_reboot(int argc, char** argv) {
	Reboot();
}
COMMAND("reboot", "reboot the device", cmd_reboot);

#ifndef CONFIG_A4 // TODO: Fix A4 stuff. :P

#ifdef CONFIG_IPHONE_2G
void cmd_multitouch_fw_install(int argc, char** argv)
{
    if(argc < 5)
    {
        bufferPrintf("%s <a-speed fw> <a-speed fw len> <main fw> <main fw len>\r\n", argv[0]);
        return;
    }
    
    uint8_t* aspeedFW = (uint8_t*) parseNumber(argv[1]);
    uint32_t aspeedFWLen = parseNumber(argv[2]);
    uint8_t* mainFW = (uint8_t*) parseNumber(argv[3]);
    uint32_t mainFWLen = parseNumber(argv[4]);
    
    //get latest apple image
    Image* image = images_get_last_apple_image();
    uint32_t offset = image->offset+image->padded;
    
    //write aspeed first
    if(offset >= 0xfc000 || (offset + aspeedFWLen + mainFWLen) >= 0xfc000) {
        bufferPrintf("**ABORTED** Image of size %d at 0x%x would overflow NOR!\r\n", aspeedFWLen+mainFWLen, offset);
        return;
    }    
    
    bufferPrintf("Writing aspeed 0x%x - 0x%x to 0x%x...\r\n", aspeedFW, aspeedFW + aspeedFWLen, offset);
    nor_write((void*)aspeedFW, offset, aspeedFWLen);
    
    offset += aspeedFWLen;
    
    bufferPrintf("Writing main 0x%x - 0x%x to 0x%x...\r\n", mainFW, mainFW + mainFWLen, offset);
    nor_write((void*)mainFW, offset, mainFWLen);
    
    bufferPrintf("Zephyr firmware installed.\r\n");
}
#else
void cmd_multitouch_fw_install(int argc, char** argv)
{
    if(argc < 3)
    {
        bufferPrintf("%s <constructed fw> <constructed fw len>\r\n", argv[0]);
        return;
    }
    
    uint8_t* fwData = (uint8_t*) parseNumber(argv[1]);
    uint32_t fwLen = parseNumber(argv[2]);
    
    //get latest apple image
    Image* image = images_get_last_apple_image();
    if (image == NULL) {
        bufferPrintf("**ABORTED** Last image position cannot be read\r\n");
        return;
    }
    uint32_t offset = image->offset+image->padded;
    
    if(offset >= 0xfc000 || (offset + fwLen) >= 0xfc000) {
        bufferPrintf("**ABORTED** Image of size %d at %x would overflow NOR!\r\n", fwLen, offset);
        return;
    }    
    
    bufferPrintf("Writing 0x%x - 0x%x to 0x%x...\r\n", fwData, fwData + fwLen, offset);
    nor_write((void*)fwData, offset, fwLen);
    bufferPrintf("Zephyr2 firmware installed.\r\n");
}
#endif
COMMAND("multitouch_fw_install", "install multitouch firmware", cmd_multitouch_fw_install);

void cmd_multitouch_fw_uninstall(int argc, char** argv) {
#ifdef CONFIG_IPHONE_2G	
	images_uninstall(fourcc("mtza"), fourcc("mtza"));
	images_uninstall(fourcc("mtzm"), fourcc("mtzm"));
#else
	images_uninstall(fourcc("mtz2"), fourcc("mtz2"));
#endif	
}
COMMAND("multitouch_fw_uninstall","uninstall multitouch firmware", cmd_multitouch_fw_uninstall);

void cmd_md(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	bufferPrintf("dumping memory 0x%x - 0x%x\r\n", address, address + len);
	buffer_dump_memory(address, len);
}
COMMAND("md", "display a block of memory as 32-bit integers", cmd_md);

void cmd_hexdump(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	bufferPrintf("dumping memory 0x%x - 0x%x\r\n", address, address + len);
	hexdump(address, len);
}
COMMAND("hexdump", "display a block of memory like 'hexdump -C'", cmd_hexdump);

void cmd_cat(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	addToBuffer((char*) address, len);
}
COMMAND("cat", "dumps a block of memory", cmd_cat);

void cmd_go(int argc, char** argv) {
	uint32_t address;

	if(argc < 2) {
		address = 0x09000000;
	} else {
		address = parseNumber(argv[1]);
	}

	bufferPrintf("Jumping to 0x%x (interrupts disabled)\r\n", address);

	// make as if iBoot was called from ROM
	pmu_set_iboot_stage(0x1F);

	udelay(100000);

	chainload(address);
}
COMMAND("go", "jump to a specified address (interrupts disabled)", cmd_go);

void cmd_jump(int argc, char** argv) {
	if(argc < 2) {
		bufferPrintf("Usage: %s <address>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	bufferPrintf("Jumping to 0x%x\r\n", address);
	udelay(100000);

	CallArm(address);
}
COMMAND("jump", "jump to a specified address (interrupts enabled)", cmd_jump);

void cmd_mwb(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <data>\r\n", argv[0]);
		return;
	}

	uint8_t* address = (uint8_t*) parseNumber(argv[1]);
	uint8_t data = parseNumber(argv[2]);
	*address = data;
	bufferPrintf("Written to 0x%x to 0x%x\r\n", (uint8_t)data, address);
}
COMMAND("mwb", "write a byte into a memory address", cmd_mwb);

void cmd_mws(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <string>\r\n", argv[0]);
		return;
	}

	char* address = (char*) parseNumber(argv[1]);
	strcpy(address, argv[2]);
	bufferPrintf("Written %s to 0x%x\r\n", argv[2], address);
}
COMMAND("mws", "write a string into a memory address", cmd_mws);

void cmd_mw(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <data>\r\n", argv[0]);
		return;
	}

	uint32_t* address = (uint32_t*) parseNumber(argv[1]);
	uint32_t data = parseNumber(argv[2]);
	*address = data;
	bufferPrintf("Written to 0x%x to 0x%x\r\n", data, address);
}
COMMAND("mw", "write a 32-bit dword into a memory address", cmd_mw);

void cmd_echo(int argc, char** argv) {
	int i;
	for(i = 1; i < argc; i++) {
		bufferPrintf("%s ", argv[i]);
	}
	bufferPrintf("\r\n");
}
COMMAND("echo", "echo back a line", cmd_echo);

void cmd_version(int argc, char** argv) {
	bufferPrintf("%s\r\n", OPENIBOOT_VERSION_STR);
}
COMMAND("version", "display the version string", cmd_version);

#endif //CONFIG_A4

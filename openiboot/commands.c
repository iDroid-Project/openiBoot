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
		bufferPrintf("%s\n", cmd->name);

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

void cmd_kernel(int argc, char** argv) {
	uint32_t address;
	uint32_t size;

	if(argc < 3) {
		address = 0x09000000;
		size = received_file_size;
	} else {
		address = parseNumber(argv[1]);
		size = parseNumber(argv[2]);
	}

	set_kernel((void*) address, size);
	bufferPrintf("Loaded kernel at %08x - %08x\r\n", address, address + size);
}
COMMAND("kernel", "load a Linux kernel", cmd_kernel);

void cmd_ramdisk(int argc, char** argv) {
	uint32_t address;
	uint32_t size;

	if(argc < 3) {
		address = 0x09000000;
		size = received_file_size;
	} else {
		address = parseNumber(argv[1]);
		size = parseNumber(argv[2]);
	}

	set_ramdisk((void*) address, size);
	bufferPrintf("Loaded ramdisk at %08x - %08x\r\n", address, address + size);
}
COMMAND("ramdisk", "load a Linux ramdisk", cmd_ramdisk);

void cmd_rootfs(int argc, char** argv) {
	int partition;
	const char* fileName;

	if(argc < 3) {
		bufferPrintf("usage: %s <partition> <path>\r\n", argv[0]);
		return;
	} else {
		partition = parseNumber(argv[1]);
		fileName = argv[2];
	}

	set_rootfs(partition, fileName);
	bufferPrintf("set rootfs to %s on partition %d\r\n", fileName, partition);
}
COMMAND("rootfs", "specify a file as the Linux rootfs", cmd_rootfs);

void cmd_boot_linux(int argc, char** argv) {
	char* arguments = "";

	if(argc >= 2) {
		arguments = argv[1];
	}

	bufferPrintf("Booting kernel with arguments (%s)...\r\n", arguments);

	boot_linux(arguments);
}
COMMAND("boot_linux", "boot a Linux kernel", cmd_boot_linux);

void cmd_boot_ios(int argc, char** argv) {
	Image* image = images_get(fourcc("ibox"));
	if(image == NULL)
		image = images_get(fourcc("ibot"));
	void* imageData;
	images_read(image, &imageData);
	chainload((uint32_t)imageData);
}
COMMAND("boot_ios", "boot a iOS via iBoot", cmd_boot_ios);

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

void cmd_audiohw_position(int argc, char** argv)
{
	bufferPrintf("playback position: %u / %u\r\n", audiohw_get_position(), audiohw_get_total());
}
COMMAND("audiohw_position", "print the playback position", cmd_audiohw_position);

void cmd_audiohw_pause(int argc, char** argv)
{
	audiohw_pause();
	bufferPrintf("Paused.\r\n");
}
COMMAND("audiohw_pause", "pause playback", cmd_audiohw_pause);

void cmd_audiohw_resume(int argc, char** argv)
{
	audiohw_resume();
	bufferPrintf("Resumed.\r\n");
}
COMMAND("audiohw_resume", "resume playback", cmd_audiohw_resume);

void cmd_audiohw_transfers_done(int argc, char** argv)
{
	bufferPrintf("transfers done: %d\r\n", audiohw_transfers_done());
}
COMMAND("audiohw_transfers_done", "display how many times the audio buffer has been played", cmd_audiohw_transfers_done);

void cmd_audiohw_play_pcm(int argc, char** argv)
{
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len> [use-headphones]\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	uint32_t useHeadphones = 0;

	if(argc > 3)
		useHeadphones = parseNumber(argv[3]);

	if(useHeadphones)
	{
		bufferPrintf("playing PCM 0x%x - 0x%x using headphones\r\n", address, address + len);
		audiohw_play_pcm((void*)address, len, FALSE);
	} else
	{
		bufferPrintf("playing PCM 0x%x - 0x%x using speakers\r\n", address, address + len);
		audiohw_play_pcm((void*)address, len, TRUE);
	}
}
COMMAND("audiohw_play_pcm", "queue some PCM data for playback", cmd_audiohw_play_pcm);

void cmd_audiohw_headphone_vol(int argc, char** argv)
{
	if(argc < 2)
	{
		bufferPrintf("%s <left> [right] (between 0:%d and 63:%d dB)\r\n", argv[0], audiohw_settings[SOUND_VOLUME].minval, audiohw_settings[SOUND_VOLUME].maxval);
		return;
	}

	int left = parseNumber(argv[1]);
	int right;

	if(argc >= 3)
		right = parseNumber(argv[2]);
	else
		right = left;

	audiohw_set_headphone_vol(left, right);

	bufferPrintf("Set headphone volumes to: %d / %d\r\n", left, right);
}
COMMAND("audiohw_headphone_vol", "set the headphone volume", cmd_audiohw_headphone_vol);

#ifndef CONFIG_IPOD
void cmd_audiohw_speaker_vol(int argc, char** argv)
{
	if(argc < 2)
	{
#ifdef CONFIG_IPHONE_3G
		bufferPrintf("%s <loudspeaker volume> (between 0 and 100)\r\n", argv[0]);
#else
		bufferPrintf("%s <loudspeaker volume> [speaker volume] (between 0 and 100... 'speaker' is the one next to your ear)\r\n", argv[0]);
#endif
		return;
	}

	int vol = parseNumber(argv[1]);

#ifdef CONFIG_IPHONE_3G
	audiohw_set_speaker_vol(vol);
	bufferPrintf("Set speaker volume to: %d\r\n", vol);
#else
	loudspeaker_vol(vol);

	bufferPrintf("Set loudspeaker volume to: %d\r\n", vol);

	if(argc > 2)
	{
		vol = parseNumber(argv[2]);
		speaker_vol(vol);

		bufferPrintf("Set speaker volume to: %d\r\n", vol);
	}
#endif
}
COMMAND("audiohw_speaker_vol", "set the speaker volume", cmd_audiohw_speaker_vol);
#endif

#endif //CONFIG_A4

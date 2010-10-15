/*
 * oib.c - OpeniBoot Console for Linux (and other UNiXes).
 *
 * Copyright (C) 2008 Yiduo Wang
 * Portions Copyright (C) 2010 Ricky Taylor
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <usb.h>
#include <pthread.h>
#include <readline/readline.h>
#include <errno.h>
#include <getopt.h>

// TODO: We really need to sort the protocol out. -- Ricky26
#define OPENIBOOTCMD_DUMPBUFFER				1
#define OPENIBOOTCMD_DUMPBUFFER_LEN			2
#define OPENIBOOTCMD_DUMPBUFFER_GOAHEAD		3
#define OPENIBOOTCMD_SENDCOMMAND			4
#define OPENIBOOTCMD_SENDCOMMAND_GOAHEAD	5
#define OPENIBOOTCMD_READY					6
#define OPENIBOOTCMD_NOTREADY				7
#define OPENIBOOTCMD_ISREADY				8

#define MAX_TO_SEND 512

typedef struct OpenIBootCmd {
	uint32_t command;
	uint32_t dataLen;
}  __attribute__ ((__packed__)) OpenIBootCmd;

static int getFile(char * commandBuffer);

usb_dev_handle* device;
FILE* outputFile = NULL;
volatile size_t readIntoOutput = 0;

volatile char *dataToWrite;
volatile int amtToWrite = 0;
volatile int ready = 0;
int readPending = 0;
int silent = 0;

pthread_mutex_t sendLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sendCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t readyCond = PTHREAD_COND_INITIALIZER;

#define USB_BYTES_AT_A_TIME 512

void oibc_log(const char *format, ...)
{
	va_list args;

	if(silent)
		return;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int do_receiveBuffer(size_t totalLen)
{
	OpenIBootCmd cmd;
	char* buffer;

	if(totalLen > 0)
	{
		buffer = (char*) malloc(totalLen + 1);

		cmd.command = OPENIBOOTCMD_DUMPBUFFER_GOAHEAD;
		cmd.dataLen = totalLen;
		usb_interrupt_write(device, 4, (char*)&cmd, sizeof(OpenIBootCmd), 1000);

		int read = 0;
		while(read < totalLen) {
			int left = (totalLen - read);
			size_t toRead = (left > USB_BYTES_AT_A_TIME) ? USB_BYTES_AT_A_TIME : left;
			int hasRead;
			hasRead = usb_bulk_read(device, 1, buffer + read, toRead, 1000);
			read += hasRead;
		}

		int discarded = 0;
		if(readIntoOutput > 0) {
			if(readIntoOutput <= read) {
				fwrite(buffer, 1, readIntoOutput, outputFile);
				discarded += readIntoOutput;
				readIntoOutput = 0;
				fclose(outputFile);
			} else {
				fwrite(buffer, 1, read, outputFile);
				discarded += read;
				readIntoOutput -= read;
			}
		}

		*(buffer + read) = '\0';
		printf("%s", buffer + discarded); fflush(stdout);

		free(buffer);
	}

	return 0;
}

int do_sendCommand()
{
	int toSend = 0;
	while(amtToWrite > 0) {
		if(amtToWrite <= MAX_TO_SEND)
			toSend = amtToWrite;
		else
			toSend = MAX_TO_SEND;

		usb_bulk_write(device, 2, (char*)dataToWrite, toSend, 1000);
		dataToWrite += toSend;
		amtToWrite -= toSend;
	}

	amtToWrite = 0; // Just in case.

	pthread_cond_signal(&sendCond);

	return 0;
}

void* doOutput(void* threadid) {
	int ret;
	OpenIBootCmd cmd;

	cmd.command = OPENIBOOTCMD_ISREADY;
	cmd.dataLen = 0;
	usb_interrupt_write(device, 4, (char*)&cmd, sizeof(OpenIBootCmd), 1000);

	while(1)
	{
		ret = usb_interrupt_read(device, 3, (char*)&cmd, sizeof(OpenIBootCmd), 100);
		if(ret > 0)
		{
			switch(cmd.command)
			{
			case OPENIBOOTCMD_DUMPBUFFER_LEN:
				if(do_receiveBuffer(cmd.dataLen))
					goto error;
				readPending = 0;
				break;

			case OPENIBOOTCMD_SENDCOMMAND_GOAHEAD:
				if(do_sendCommand())
					goto error;
				break;

			case OPENIBOOTCMD_READY:
				ready = 1;
				pthread_cond_broadcast(&readyCond);
				break;

			case OPENIBOOTCMD_NOTREADY:
				ready = 0;
				break;

			default:
			error:
				oibc_log("invalid command (%d:%d)\n", cmd.command, cmd.dataLen);
				break;
			}
		}
		else if(ret == -ETIMEDOUT)
		{
			if(!readPending)
			{
				// Check whether there is some buffer to read... -- Ricky26
				cmd.command = OPENIBOOTCMD_DUMPBUFFER;
				cmd.dataLen = 0;
				if(usb_interrupt_write(device, 4, (char*)&cmd, sizeof(OpenIBootCmd), 1000) == 4)
					readPending = 1;
			}
		}
		else	
		{
			oibc_log("failed to read interrupt, error %d: %s.\n", -ret, strerror(-ret));
			pthread_cond_signal(&exitCond);
			pthread_exit((void*)EIO);
		}

		sched_yield();
	}

	pthread_cond_signal(&exitCond);
	pthread_exit((void*)0);
}

void sendBuffer(char* buffer, size_t size) {
	OpenIBootCmd cmd;

	pthread_mutex_lock(&sendLock);

	if(!ready)
		pthread_cond_wait(&readyCond, &sendLock);

	amtToWrite = size;
	dataToWrite = buffer;

	cmd.command = OPENIBOOTCMD_SENDCOMMAND;
	cmd.dataLen = size;

	if(usb_interrupt_write(device, 4, (char*) (&cmd), sizeof(OpenIBootCmd), 1000) < 0)
	{
		oibc_log("failed to send command.\n");
		pthread_mutex_unlock(&sendLock);
		return;
	}

	pthread_cond_wait(&sendCond, &sendLock);
	
	pthread_mutex_unlock(&sendLock);
}

void* doInput(void* threadid) {
	char* commandBuffer = NULL;
	char toSendBuffer[USB_BYTES_AT_A_TIME];

	rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(~!:";
	rl_completion_append_character = '\0';

	while(1) {
		if(commandBuffer != NULL)
			free(commandBuffer);

		commandBuffer = readline(NULL);
		if(commandBuffer && *commandBuffer) {
			add_history(commandBuffer);
			write_history(".oibc-history");
		}

		if(!commandBuffer)
			break;

		char* fileBuffer = NULL;
		int len = strlen(commandBuffer);

		if(commandBuffer[0] == '!') {
			char* atLoc = strchr(&commandBuffer[1], '@');

			if(atLoc != NULL)
				*atLoc = '\0';

			FILE* file = fopen(&commandBuffer[1], "rb");
			if(!file) {
				oibc_log("file not found: %s\n", &commandBuffer[1]);
				continue;
			}
			fseek(file, 0, SEEK_END);
			len = ftell(file);
			fseek(file, 0, SEEK_SET);
			fileBuffer = malloc(len);
			fread(fileBuffer, 1, len, file);
			fclose(file);

			if(atLoc != NULL) {
				sprintf(toSendBuffer, "sendfile %s", atLoc + 1);
			} else {
				sprintf(toSendBuffer, "sendfile 0x09000000");
			}

			sendBuffer(toSendBuffer, strlen(toSendBuffer));
			sendBuffer(fileBuffer, len);
			free(fileBuffer);
		} else if(commandBuffer[0] == '~') {
            if (getFile(commandBuffer+1)==1)
                continue;
            
        } else if (strcmp(commandBuffer,"install") == 0) {
            oibc_log("Backing up your NOR to current directory as norbackup.dump\n");
            sprintf(toSendBuffer, "nor_read 0x09000000 0x0 1048576");

			commandBuffer[len] = '\n';
			sendBuffer(toSendBuffer, strlen(toSendBuffer));

			sprintf(toSendBuffer,"norbackup.dump:1048576"); 
			getFile(toSendBuffer);
			oibc_log("Fetching NOR backup.\n");
			commandBuffer[len] = '\n';
			oibc_log("NOR backed up, starting installation\n");
			sprintf(toSendBuffer,"install");
			sendBuffer(toSendBuffer, strlen(toSendBuffer));
		} else {
			commandBuffer[len] = '\n';
			sendBuffer(commandBuffer, len + 1);
		}

		sched_yield();
	}
	pthread_cond_signal(&exitCond);
	pthread_exit(NULL);
}

int getFile(char * commandBuffer)
{
    char toSendBuffer[USB_BYTES_AT_A_TIME];
    char* sizeLoc = strchr(commandBuffer, ':');
    
    if(sizeLoc == NULL) {
        oibc_log("must specify length to read\n");
        return 1;
    }
    
    *sizeLoc = '\0';
    sizeLoc++;
    
    int toRead;
    sscanf(sizeLoc, "%i", &toRead);
    
    char* atLoc = strchr(commandBuffer, '@');
    
    if(atLoc != NULL)
        *atLoc = '\0';
    
    FILE* file = fopen(commandBuffer, "wb");
    if(!file) {
        oibc_log("cannot open file: %s\n", commandBuffer);
        return 1;
    }
    
    if(atLoc != NULL) {
        sprintf(toSendBuffer, "getfile %s %d", atLoc + 1, toRead);
    } else {
        sprintf(toSendBuffer, "getfile 0x09000000 %d", toRead);
    }
    
    sendBuffer(toSendBuffer, strlen(toSendBuffer));
    outputFile = file;
    readIntoOutput = toRead;
    
    return 0;
}

int main(int argc, char* argv[])
{
	static struct option program_options[] = {
		{"silent",	no_argument,	NULL,	's'},
	};
	
	while(1)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "s", program_options, &option_index);
		if(c == -1)
			break;

		switch(c)
		{
		case 's':
			silent = 1;
			break;
		};
	}

	read_history(".oibc-history");

	usb_init();
	usb_find_busses();
	usb_find_devices();

	struct usb_bus *busses;
	struct usb_bus *bus;
	struct usb_device *dev;

	busses = usb_get_busses();

	int found = 0;
	int c, i, a;
	for(bus = busses; bus; bus = bus->next) {
    		for (dev = bus->devices; dev; dev = dev->next) {
    			/* Check if this device is a printer */
    			//if (dev->descriptor.idVendor != 0x05ac || dev->descriptor.idProduct != 0x1280) {
    			if (dev->descriptor.idVendor != 0x0525 || dev->descriptor.idProduct != 0x1280) {
    				continue;
    			}
    
			/* Loop through all of the interfaces */
			for (i = 0; i < dev->config[0].bNumInterfaces; i++) {
				for (a = 0; a < dev->config[0].interface[i].num_altsetting; a++) {
					if(dev->config[0].interface[i].altsetting[a].bInterfaceClass == 0xFF
						&& dev->config[0].interface[i].altsetting[a].bInterfaceSubClass == 0xFF
						&& dev->config[0].interface[i].altsetting[a].bInterfaceProtocol == 0x51) {
						goto done;
					}
    				}
    			}
    		}
	}

	oibc_log("Failed to find a device in OpeniBoot mode.\n");
	return 1;

done:

	device = usb_open(dev);
	if(!device) {
		oibc_log("Failed to open OpeniBoot device.\n");
		return 2;
	}

	if(usb_claim_interface(device, i) != 0) {
		oibc_log("Failed to claim OpeniBoot device.\n");
		return 3;
	}

	pthread_t inputThread;
	pthread_t outputThread;

	oibc_log("OiB client connected:\n");
    oibc_log("!<filename>[@<address>] to send a file, ~<filename>[@<address>]:<len> to receive a file\n");
	oibc_log("---------------------------------------------------------------------------------------------------------\n");

	pthread_create(&outputThread, NULL, doOutput, NULL);
	pthread_create(&inputThread, NULL, doInput, NULL);

	pthread_mutex_lock(&exitLock);
	pthread_cond_wait(&exitCond, &exitLock);
	pthread_mutex_unlock(&exitLock);

	pthread_cancel(inputThread);
	pthread_cancel(outputThread);

	rl_deprep_terminal(); // If we cancel readline, we must call this to not fsck up the terminal.

	usb_release_interface(device, i);
	usb_close(device);
}

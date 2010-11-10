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

#define MAX_TO_SEND 512
#define SEND_EP		2
#define RECV_EP		1
#define FILE_START_MAGIC "ACM: Starting File: "

static int getFile(char *commandBuffer);
static int sendFile(char *commandBuffer);

usb_dev_handle* device;
FILE* outputFile = NULL;
volatile size_t readAmt = 0;
volatile size_t currReadAmt = 0;

static int silent = 0;

pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t exitCond = PTHREAD_COND_INITIALIZER;

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

void* doOutput(void* threadid)
{
	int ret;
	static char buffer[513];

	while(1)
	{
		int ret = usb_bulk_read(device, RECV_EP, buffer, sizeof(buffer)-1, 5000);
		if(ret >= 0)
		{
			buffer[ret] = 0;

			char *ptr = buffer;
			if(currReadAmt > 0)
			{
				int amt = currReadAmt;
				if(ret < amt)
					amt = ret;

				fwrite(buffer, 1, amt, outputFile);

				ret -= amt;
				currReadAmt -= amt;
				ptr += amt;

				if(currReadAmt == 0)
				{
					fclose(outputFile);
					outputFile = NULL;
				}
			}

			char *p2 = strstr(ptr, FILE_START_MAGIC);
			if(p2)
			{
				*p2 = 0;
				printf("%s", ptr);

				p2 += strlen(FILE_START_MAGIC);

				char *p3 = strchr(p2, '\n');
				if(!p3)
				{
					fprintf(stderr, "Failed to read file part, no newline.\n");
					continue;
				}
				else
				{
					*p3 = 0;
					p3++;

					int loc;
					int size;

					if(sscanf(p2, "%d %d", &loc, &size) > 0)
					{
						int buffLeft = ret - (p3 - buffer) - 1;
						if(buffLeft - size < 0)
						{
							currReadAmt = size - buffLeft;
							size = buffLeft;
						}

						fwrite(p3, 1, size, outputFile);

						ptr = p3 + size;
					}
				}
			}

			printf("%s", ptr);
		}
		else if(ret == -ETIMEDOUT)
		{
			// Do something? -- Ricky26
		}
		else
		{
			fprintf(stderr, "Failed to read: %s\n", strerror(-ret));
			break;
		}
	}

	pthread_cond_signal(&exitCond);
	pthread_exit((void*)0);
}

void sendBuffer(char* buffer, size_t size) {

	int ret = 0;

	if(size == 0)
		ret = usb_bulk_write(device, SEND_EP, buffer, size, 10000);
	else
	{
		while(size > USB_BYTES_AT_A_TIME)
		{
			ret = usb_bulk_write(device, SEND_EP, buffer, USB_BYTES_AT_A_TIME, 10000);
			if(ret >= 0)
			{
				buffer += ret;
				size -= ret;
			}
			else
				break;
		}

		if(size > 0 && ret >= 0)
			ret = usb_bulk_write(device, SEND_EP, buffer, size, 10000);
	}

	if(ret < 0)
		oibc_log("failed to send command (0x%08x:%s).\n", -ret, strerror(-ret));
}

void* doInput(void* threadid) {
	char* commandBuffer = NULL;
	char toSendBuffer[USB_BYTES_AT_A_TIME];

	rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(~!:";
	rl_completion_append_character = '\0';

	// Hack for strange Synopsys bug -- Ricky26
	sendBuffer(NULL, 0);

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

		int len = strlen(commandBuffer);
		if(commandBuffer[0] == '!')
		{
			if(sendFile(commandBuffer+1))
				continue;
		}
		else if(commandBuffer[0] == '~')
		{
            if (getFile(commandBuffer+1))
                continue;
        }
		else if (strcmp(commandBuffer,"install") == 0)
		{
            oibc_log("Backing up your NOR to current directory as norbackup.dump\n");

            sprintf(toSendBuffer, "nor_read 0x09000000 0x0 1048576\n");
			sendBuffer(toSendBuffer, strlen(toSendBuffer));

			oibc_log("Fetching NOR backup.\n");
			sprintf(toSendBuffer, "norbackup.dump:1048576");
			getFile(toSendBuffer);

			oibc_log("NOR backed up, starting installation\n");
			sprintf(toSendBuffer,"install\n");
			sendBuffer(toSendBuffer, strlen(toSendBuffer));
		}
		else
		{
			commandBuffer[len] = '\n';
			sendBuffer(commandBuffer, len + 1);
		}

		sched_yield();
	}
	pthread_cond_signal(&exitCond);
	pthread_exit(NULL);
}

int sendFile(char *commandBuffer)
{
    char toSendBuffer[USB_BYTES_AT_A_TIME];
	char* atLoc = strchr(commandBuffer, '@');

	if(atLoc != NULL)
		*atLoc = '\0';

	FILE* file = fopen(commandBuffer, "rb");
	if(!file)
	{
		oibc_log("file not found: %s\n", commandBuffer);
		return 1;
	}

	fseek(file, 0, SEEK_END);
	size_t len = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *fileBuffer = malloc(len);
	fread(fileBuffer, 1, len, file);
	fclose(file);

	if(atLoc != NULL)
		sprintf(toSendBuffer, "sendfile %s %d\n", atLoc + 1, len);
	else
		sprintf(toSendBuffer, "sendfile 0x09000000 %d\n", len);
	
	fprintf(stderr, "File length %d.\n", len);

	sendBuffer(toSendBuffer, strlen(toSendBuffer));
	sendBuffer(fileBuffer, len);
	free(fileBuffer);

	return 0;
}

int getFile(char *commandBuffer)
{
    char toSendBuffer[USB_BYTES_AT_A_TIME];
    char* sizeLoc = strchr(commandBuffer, ':');
    
    if(sizeLoc == NULL) {
        oibc_log("must specify length to read\n");
        return 1;
    }

	while(readAmt)
		sleep(50);
    
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
        sprintf(toSendBuffer, "recvfile %s %d\n", atLoc + 1, toRead);
    } else {
        sprintf(toSendBuffer, "recvfile 0x09000000 %d\n", toRead);
    }
    
    sendBuffer(toSendBuffer, strlen(toSendBuffer));
    outputFile = file;
    readAmt = toRead;
    
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
					if(dev->config[0].interface[i].altsetting[a].bInterfaceClass == 0xa
						&& dev->config[0].interface[i].altsetting[a].bInterfaceSubClass == 0x0
						&& dev->config[0].interface[i].altsetting[a].bInterfaceProtocol == 0x0) {
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
	fflush(stdin); // Prevent madness

	usb_release_interface(device, i);
	usb_close(device);
}

#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "hardware/power.h"
#include "hardware/usb.h"
#include "hardware/platform.h"
#include "acm.h"
#include "usb.h"
#include "commands.h"
#include "tasks.h"
#include "util.h"

#define ACM_BUFFER_SIZE 1024
#define ACM_EP_SEND		1
#define ACM_EP_RECV		2

static char* acm_send_buffer = NULL;
static char* acm_recv_buffer = NULL;
static char* acm_file_ptr = NULL;
static int acm_file_recv_left = 0;
static int acm_file_send_left = 0;
static int acm_usb_mps = 0;
static int acm_unprocessed = 0;
static int acm_busy = FALSE;
int acm_is_ready = 0;
TaskDescriptor acm_parse_task;

static int acm_send()
{
	if(acm_file_send_left > 0)
	{
		acm_busy = TRUE;

		int amt = acm_file_send_left;
		if(amt > acm_usb_mps)
			amt = acm_usb_mps;

		memcpy(acm_send_buffer, acm_file_ptr, amt);
		acm_file_ptr += amt;
		acm_file_send_left -= amt;

		usb_send_bulk(ACM_EP_SEND, acm_send_buffer, amt);

		if(acm_file_send_left == 0)
			bufferPrintf("ACM: Sent file (finished at 0x%08x)!\n", acm_file_ptr);

		return 1;
	}

	if(getScrollbackLen() > 0)
	{
		acm_busy = TRUE;

		int amt = getScrollbackLen();
		if(amt > acm_usb_mps)
			amt = acm_usb_mps;
		
		bufferFlush(acm_send_buffer, amt);
		usb_send_bulk(ACM_EP_SEND, acm_send_buffer, amt);

		return 1;
	}

	return 0;
}

static void acm_parse(int32_t _amt)
{
	int start = 0;
	int i = 0;

	if(acm_file_ptr != NULL && acm_file_recv_left > 0)
	{
		if(_amt >= acm_file_recv_left)
		{
			memcpy(acm_file_ptr, acm_recv_buffer, acm_file_recv_left);
			i = acm_file_recv_left;
			start = i;

			bufferPrintf("ACM: Received file (finished at 0x%08x)!\n", acm_file_ptr + acm_file_recv_left);

			acm_file_ptr = NULL;
			acm_file_recv_left = 0;
		}
		else
		{

			memcpy(acm_file_ptr, acm_recv_buffer, _amt);
			acm_file_ptr += _amt;
			acm_file_recv_left -= _amt;
			//bufferPrintf("ACM: Got %d of file (%d remain).\n", _amt, acm_file_recv_left);

			EnterCriticalSection(); // Deliberately unended.
			usb_receive_bulk(ACM_EP_RECV, acm_recv_buffer, acm_usb_mps);
			return;
		}
	}

	for(; i < _amt; i++)
	{
		if(acm_recv_buffer[i] == '\n')
		{
			acm_recv_buffer[i] = 0;
			if(i > 0)
				if(acm_recv_buffer[i-1] == '\r')
					acm_recv_buffer[i-1] = 0;

			char safeCommand[ACM_BUFFER_SIZE];
			char *command = &acm_recv_buffer[start];
			strcpy(safeCommand, command);
			int argc;
			char** argv = tokenize(command, &argc);
			//OPIBCommand* curCommand = CommandList;

			if(argc >= 3 && strcmp(argv[0], "sendfile") == 0)
			{
				acm_file_ptr = (char*)parseNumber(argv[1]);
				acm_file_recv_left = parseNumber(argv[2]);
				received_file_size = acm_file_recv_left;
				bufferPrintf("ACM: Started receiving file at 0x%08x - 0x%08x (%d bytes).\n", acm_file_ptr, acm_file_ptr + acm_file_recv_left, acm_file_recv_left);
				i = _amt;
				start = i;
			}
			else if(argc >= 3 && strcmp(argv[0], "recvfile") == 0)
			{
				acm_busy = TRUE;

				acm_file_ptr = (char*)parseNumber(argv[1]);
				acm_file_send_left = parseNumber(argv[2]);

				bufferPrintf("ACM: Started sending file at 0x%08x - 0x%08x (%d bytes).\n", acm_file_ptr, acm_file_ptr + acm_file_send_left, acm_file_send_left);

				int amt = sprintf(acm_send_buffer, "ACM: Starting File: %d %d\n", (uint32_t)acm_file_ptr, acm_file_send_left);
				usb_send_bulk(ACM_EP_SEND, acm_send_buffer, amt+1);

				i = _amt;
				start = i;
			}
			else
			{
				/*int success = FALSE;
				while(curCommand->name != NULL) {
					if(strcmp(argv[0], curCommand->name) == 0) {
						bufferPrintf("ACM: Starting: %s\n", safeCommand);
						curCommand->routine(argc, argv);
						bufferPrintf("ACM: Done: %s\n", safeCommand);
						success = TRUE;
						break;
					}
					curCommand++;
				}

				if(!success) {
					bufferPrintf("ACM: Unknown command: %s\r\n", safeCommand);
				}*/

				bufferPrintf("ACM: Starting %s\n", command);
				if(command_run(argc, argv) == 0)
					bufferPrintf("ACM: Done: %s\n", command);
				else
					bufferPrintf("ACM: Unknown command: %s\n", command);
				
				start = i+1;
			}

			free(argv);
		}
	}

	EnterCriticalSection(); // Deliberately unended.

	if(start < _amt)
	{
		if(acm_unprocessed > 0)
		{
			bufferPrintf("ACM: command too long, discarding...\n");
			acm_unprocessed = 0;
			usb_receive_bulk(ACM_EP_RECV, acm_recv_buffer, acm_usb_mps);
			task_stop();
			return;
		}
		else
			memcpy(acm_recv_buffer, acm_recv_buffer+start, _amt-start);
	}

	acm_unprocessed = _amt-start;
	usb_receive_bulk(ACM_EP_RECV, acm_recv_buffer+acm_unprocessed, acm_usb_mps);
}

static void acm_received(uint32_t _tkn, int32_t _amt)
{
	int attempts = 0;
	while(attempts < 5)
	{
		if(task_start(&acm_parse_task, &acm_parse, (void*)_amt))
			break;

		task_yield();

		bufferPrintf("ACM: Worker already running, yielding...\n");
	}
}

static void acm_sent(uint32_t _tkn, int32_t _amt)
{
	if(!acm_send())
		acm_busy = FALSE;
}

static int acm_setup(USBSetupPacket *setup)
{
	switch(setup->bRequest)
	{
	case USB_GET_DESCRIPTOR:
		bufferPrintf("ACM: Got a strange descriptor request: %d:%d.\n", setup->wValue>>8, setup->wValue&0xf);
		break;
	}

	return 0;
}

static void acm_enumerate(USBConfiguration *conf)
{
	acm_is_ready = 0;

	USBInterface *acm_if = usb_add_interface(conf, 0, 0, 0x0a, 0, 0, usb_add_string_descriptor("IF0"));
	usb_add_endpoint(acm_if, ACM_EP_SEND, USBIn, USBBulk);
	usb_add_endpoint(acm_if, ACM_EP_RECV, USBOut, USBBulk);

	if(!acm_send_buffer)
		acm_send_buffer = memalign(DMA_ALIGN, ACM_BUFFER_SIZE);

	if(!acm_recv_buffer)
		acm_recv_buffer = memalign(DMA_ALIGN, ACM_BUFFER_SIZE);
}

static void acm_started()
{
	acm_is_ready = 0;

	if(usb_get_speed() == USBHighSpeed)
		acm_usb_mps = 512;
	else
		acm_usb_mps = 0x80;

	usb_enable_endpoint(ACM_EP_SEND, USBIn, USBBulk, acm_usb_mps);
	usb_enable_endpoint(ACM_EP_RECV, USBOut, USBBulk, acm_usb_mps);

	acm_busy = FALSE;
	acm_file_ptr = NULL;
	acm_file_recv_left = 0;
	acm_file_send_left = 0;
	acm_unprocessed = 0;

	usb_receive_bulk(ACM_EP_RECV, acm_recv_buffer, acm_usb_mps);

	acm_send();
	acm_is_ready = 1;

	bufferPrintf("ACM: Ready.\n");
}

void acm_start()
{
	task_init(&acm_parse_task, "ACM");

	usb_setup(acm_enumerate, acm_started);
	usb_install_ep_handler(ACM_EP_SEND, USBIn, acm_sent, 0);
	usb_install_ep_handler(ACM_EP_RECV, USBOut, acm_received, 0);
	usb_install_setup_handler(acm_setup);
}
MODULE_INIT(acm_start);

void acm_stop()
{
	free(acm_send_buffer);
	free(acm_recv_buffer);

	task_destroy(&acm_parse_task);
}

void acm_buffer_notify()
{
	if(!acm_busy)
		acm_send();
}


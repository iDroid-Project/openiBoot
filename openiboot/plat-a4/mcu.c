#include "openiboot.h"
#include "util.h"
#include "uart.h"
#include "tasks.h"
#include "timer.h"
#include "arm/arm.h"
#include "commands.h"

uint32_t mcu_inited = 0; // dword_5FF33D40
uint32_t mcu_data = 0; // dword_5FF33D74

typedef struct _mcu_buffer
{
	uint8_t type;
	uint8_t data[23];
	uint32_t length;
} mcu_buffer_t;

mcu_buffer_t mcu_buffer;
/*
typedef struct _mcu_dma_state
{
	uint32_t signalled;
	uint32_t b;
	LinkedList list;
} mcu_dma_state_t;

mcu_dma_state_t mcu_dma[3];

typedef struct _mcu_dma_task
{
	TaskDescriptor *task;
	LinkedList list;
} mcu_dma_task_t;

static void mcu_init_dma_event(mcu_dma_state_t *_dma, uint32_t _b, uint32_t _signalled)
{
	_dma->signalled = _signalled;
	_dma->b = _b;
	_dma->list.prev = &_dma->list;
	_dma->list.next = &_dma->list;
}

static void mcu_handle_dma(mcu_dma_state_t *_dma)
{
	EnterCriticalSection();

	_dma->signalled = 1;

	LinkedList *list = _dma->list.next;
	while(list != &_dma->list)
	{
		mcu_dma_task_t *dma_task = CONTAINER_OF(mcu_dma_task_t, list, list);
		task_wake(dma_task->task);
		free(dma_task);
	}

	_dma->list.next = &_dma->list;
	_dma->list.prev = &_dma->list;

	LeaveCriticalSection();
}
*/

void mcu_uart_write_char(int ureg, uint8_t _char) {
	uart_write(ureg, (char*)&_char, 1);
}

uint16_t mcu_crc16(uint16_t _iv, uint8_t* _buffer, uint32_t _length) {
	uint16_t calced = _iv;
	uint32_t i;
	for (i = 0; i < _length; i++) {
		uint32_t j;
		uint32_t bitmask = 0x80;
		for (j = 0; j < 8; j++) {
			uint32_t something = calced & 0x8000;
			calced <<= 1;
			if (bitmask & _buffer[i])
				something ^= 0x8000;
			if (something)
				calced ^= 0x1021;
			bitmask >>= 1;
		}
	}
	return calced;
}

uint32_t mcu_write(uint32_t _arg0, uint8_t* _buffer, uint32_t _length) {
	if(_length >= 0x10000)
		return -1;
	uint8_t buffer[] = { 0x53, 0x74, (uint8_t)_arg0, 0x1, (uint8_t)(_length>>8), (uint8_t)_length};
	uint32_t i;
	for (i = 0; i < sizeof(buffer); i++) {
		mcu_uart_write_char(4, buffer[i]);
	}
	for (i = 0; i < _length; i++) {
		mcu_uart_write_char(4, _buffer[i]);
	}
	uint16_t calced = mcu_crc16(mcu_crc16(0xFFFF, buffer, 6), _buffer, _length);
	mcu_uart_write_char(4, calced >> 8);
	mcu_uart_write_char(4, calced & 0xFF);
	return 0;
}

uint8_t mcu_read(int ureg, uint32_t timeout) {
	uint8_t buffer;

	uint32_t result = uart_read(ureg, (char*)&buffer, 1, timeout);
	if(!result || result == -1)
		return -1;

	return buffer;
}

static TaskDescriptor mcu_task;
void mcu_run(uint32_t _V) {
	mcu_buffer_t buffer;
	memset(&buffer, 0x0, 40);
	uint8_t read;
	uint32_t goto_start_time;
	uint32_t goto_no_calc = 0;
	while (TRUE) {
		goto_start_time = 0;
		uint64_t startTime = timer_get_system_microtime();
		uint32_t ii = 0;
		uint32_t jj = 0;
		uint16_t calc = 0xFFFF;
		while(1) {
			read = mcu_read(4, 0);
			if(read <= 0 || read == 0xFF) {
				if (has_elapsed(startTime, 1000000)) {
					goto_start_time = 1;
					break;
				}
				task_sleep(10);
				continue;
			}
			if(!ii) {
				if(read != 0x53)
					continue;
				if(ii < jj+6) {
					calc = mcu_crc16(calc, &read, 1);
				}
				ii++;
				continue;
			}
			switch(ii)
			{
				case 1:
					if(read != 0x74) {
						if(read == 0x53)
							ii = 1;
						else
							ii = 0;
						continue;
					}
					break;
				case 2:
					buffer.type = read;
					break;
				case 3:
					break;
				case 4:
					jj = read << 8;
					break;
				case 5:
					jj |= read;
				default:
					if(ii < jj+6) {
						if(ii <= 0x25)
							(&(buffer.type))[ii-5] = read;
						break;
					} else
						goto_no_calc = 1;
			}
			if(!goto_no_calc) {
				if(ii < jj+6) {
					calc = mcu_crc16(calc, &read, 1);
				}
				ii++;
				continue;
			}
			goto_no_calc = 0;
			if(ii-jj == 6) {
				if(read == calc) {
					ii++;
					continue;
				} else {
					goto_start_time = 1;
					break;
				}
			} else if (ii-jj != 7) {
					ii++;
					continue;
			}
			break;
		}
		if(goto_start_time)
			continue;
		if(read != (uint8_t)calc)
			continue;
		buffer.length = (jj < 0x20 ? jj : 0x20);
		if(buffer.type == 0xFF)
			continue;



		uint8_t bla = buffer.type;
		if(buffer.type != 0x9F)
			bla = 1;
		if(buffer.type <= 5)
			bla = 0;
		bla &= 1;
		if(bla)
			continue;

		memcpy(&mcu_buffer, &buffer, 0x28);
		mcu_data = 1;
//		mcu_handle_dma(&mcu_dma[1]);

		while(mcu_data) {
			task_sleep(500);
		}
	}
}

uint32_t mcu_set_passthrough_mode(uint32_t _var) {
	if (!mcu_inited)
		return -1;

	// Not yet.

	return 0;
}

uint32_t mcu_read_imho(uint8_t* _type, uint32_t* _buffer, uint32_t _length, uint32_t* _read_length) {
	uint64_t startTime = timer_get_system_microtime();
	while (!mcu_data) {
		if(has_elapsed(startTime, 4000000))
			return -1;
		task_sleep(500);
	}

	if(_type)
		*_type = mcu_buffer.type;


	if(_buffer) {
		memcpy(_buffer, mcu_buffer.data, (_length < mcu_buffer.length ? _length : mcu_buffer.length));
	}

	if(_read_length)
		*_read_length = mcu_buffer.length;

	mcu_data = 0;
//	mcu_handle_dma(&mcu_dma[2]);
	return 0;
}

uint32_t mcu_weird(int ureg, uint8_t* _char, uint8_t _length) {
	uint8_t var1;
	uint32_t i;
	uint32_t wtf = 100;
	for (i = 0; i <= 6; i++) {
		if(i == 6)
			return -1;
		if(mcu_write(4, _char, _length) || mcu_read_imho(&var1, NULL, 0, NULL))
			continue;
		if(var1 == 0x80)
			break;
		if(var1 != 0x84)
			continue;
		if(--wtf == 0) {
			return -1;
		} else {
			task_sleep(64);
			continue;
		}
	}
	return 0;
}

uint32_t mcu_weird_hook(uint8_t _arg0, uint32_t* _arg1, uint32_t _arg2, uint8_t* _arg3) {
	return mcu_weird(4, (uint8_t*)&_arg0, 1);
}

uint32_t mcu_init() {
	uint8_t var1;
	uint32_t var2;
	uint32_t* buffer = malloc(0x20);
	uart_set(4, 250000, 8, 0, 1);
	if(uart_set_rx_buf(4, UART_IRQ_MODE, 0x400))
		system_panic("mcu_init: uart_set_rx_buf failed\r\n");

	uart_send_break_signal(4, ON);
	task_sleep(1); // 80 microseconds...
	uart_send_break_signal(4, OFF);
//	mcu_init_dma_event(&mcu_dma[0], 1, 1);
//	mcu_init_dma_event(&mcu_dma[1], 1, 0);
//	mcu_init_dma_event(&mcu_dma[2], 1, 0);
	uint32_t task_arg = 0x2000;
	task_init(&mcu_task, "mcu", TASK_DEFAULT_STACK_SIZE);
	task_start(&mcu_task, &mcu_run, &task_arg);
	if(mcu_write(2, NULL, 0) || mcu_read_imho(&var1, buffer, 0x20, &var2) || var1 != 0x80 || mcu_weird_hook(2, buffer, var2, &var1)) {
		mcu_inited = 0;
		bufferPrintf("mcu_init: failed\r\n");
		return -1;
	}
	mcu_inited = 1;
	return 0;
}
/*
void display_init() {
	if(mcu_init())
		bufferPrintf("Oh, damnit.\r\n");
}
MODULE_INIT(display_init);
*/
static void cmd_mcu_setup(int argc, char** argv) {
	bufferPrintf("setting up MCU\r\n");
	mcu_init();
}
COMMAND("mcu_setup", "mcu setup", cmd_mcu_setup);

#include "openiboot.h"
#include "uart.h"
#include "clock.h"
#include "hardware/uart.h"
#include "timer.h"
#include "interrupt.h"
#include "tasks.h"
#include "commands.h"
#include "arm/arm.h"
#include "util.h"

const UARTRegisters HWUarts[] = {
	{UART + UART0 + UART_ULCON, UART + UART0 + UART_UCON, UART + UART0 + UART_UFCON, 0,
		UART + UART0 + UART_UTRSTAT, UART + UART0 + UART_UERSTAT, UART + UART0 + UART_UFSTAT,
		0, UART + UART0 + UART_UTXH, UART + UART0 + UART_URXH, UART + UART0 + UART_UBAUD,
		UART + UART0 + UART_UDIVSLOT},
	{UART + UART1 + UART_ULCON, UART + UART1 + UART_UCON, UART + UART1 + UART_UFCON, UART + UART1 + UART_UMCON,
		UART + UART1 + UART_UTRSTAT, UART + UART1 + UART_UERSTAT, UART + UART1 + UART_UFSTAT,
		UART + UART1 + UART_UMSTAT, UART + UART1 + UART_UTXH, UART + UART1 + UART_URXH, UART + UART1 + UART_UBAUD,
		UART + UART1 + UART_UDIVSLOT},
	{UART + UART2 + UART_ULCON, UART + UART2 + UART_UCON, UART + UART2 + UART_UFCON, UART + UART2 + UART_UMCON,
		UART + UART2 + UART_UTRSTAT, UART + UART2 + UART_UERSTAT, UART + UART2 + UART_UFSTAT,
		UART + UART2 + UART_UMSTAT, UART + UART2 + UART_UTXH, UART + UART2 + UART_URXH, UART + UART2 + UART_UBAUD,
		UART + UART2 + UART_UDIVSLOT},
	{UART + UART3 + UART_ULCON, UART + UART3 + UART_UCON, UART + UART3 + UART_UFCON, UART + UART3 + UART_UMCON,
		UART + UART3 + UART_UTRSTAT, UART + UART3 + UART_UERSTAT, UART + UART3 + UART_UFSTAT,
		UART + UART3 + UART_UMSTAT, UART + UART3 + UART_UTXH, UART + UART3 + UART_URXH, UART + UART3 + UART_UBAUD,
		UART + UART3 + UART_UDIVSLOT},
	{UART + UART4 + UART_ULCON, UART + UART4 + UART_UCON, UART + UART4 + UART_UFCON, UART + UART4 + UART_UMCON,
		UART + UART4 + UART_UTRSTAT, UART + UART4 + UART_UERSTAT, UART + UART4 + UART_UFSTAT,
		UART + UART4 + UART_UMSTAT, UART + UART4 + UART_UTXH, UART + UART4 + UART_URXH, UART + UART4 + UART_UBAUD,
		UART + UART4 + UART_UDIVSLOT},
	{UART + UART5 + UART_ULCON, UART + UART5 + UART_UCON, UART + UART5 + UART_UFCON, UART + UART5 + UART_UMCON,
		UART + UART5 + UART_UTRSTAT, UART + UART5 + UART_UERSTAT, UART + UART5 + UART_UFSTAT,
		UART + UART5 + UART_UMSTAT, UART + UART5 + UART_UTXH, UART + UART5 + UART_URXH, UART + UART5 + UART_UBAUD,
		UART + UART5 + UART_UDIVSLOT},
	{UART + UART6 + UART_ULCON, UART + UART6 + UART_UCON, UART + UART6 + UART_UFCON, UART + UART6 + UART_UMCON,
		UART + UART6 + UART_UTRSTAT, UART + UART6 + UART_UERSTAT, UART + UART6 + UART_UFSTAT,
		UART + UART6 + UART_UMSTAT, UART + UART6 + UART_UTXH, UART + UART6 + UART_URXH, UART + UART6 + UART_UBAUD,
		UART + UART6 + UART_UDIVSLOT}};
UARTSettings UARTs[NUM_UARTS];

static printf_handler_t prev_printf_handler = NULL;

int UartHasInit;

uint32_t UartCommandBufferSize = 256;
char* UartCommandBuffer;
char* curUartCommandBuffer;

static TaskDescriptor uart_task;

void uart_run(uint32_t _V) {
	while(1) {
		uint32_t read = uart_read(_V, curUartCommandBuffer, UartCommandBufferSize-strlen(UartCommandBuffer), 10000);
		curUartCommandBuffer = UartCommandBuffer+read;
		int i;
		for (i = 0; i < strlen(UartCommandBuffer); i++) {
			if (UartCommandBuffer[i] == '\n' || UartCommandBuffer[i] == '\r') {
				EnterCriticalSection();
				char *safeCommand = malloc(i+1);
				memset(safeCommand, 0, sizeof(safeCommand));
				memcpy(safeCommand, UartCommandBuffer, i);
				memset(UartCommandBuffer, 0, UartCommandBufferSize);
				curUartCommandBuffer = UartCommandBuffer;
				LeaveCriticalSection();
				int argc;
				char** argv = command_parse(safeCommand, &argc);
				bufferPrintf("UART: Starting %s\n", safeCommand);
				if(command_run(argc, argv) == 0)
					bufferPrintf("UART: Done: %s\n", safeCommand);
				else
					bufferPrintf("UART: Unknown command: %s\n", safeCommand);
				break;
			}
		}
		EnterCriticalSection();
		if (strlen(UartCommandBuffer) == UartCommandBufferSize) {
			memset(UartCommandBuffer, 0, UartCommandBufferSize);
			curUartCommandBuffer = UartCommandBuffer;
		}
		LeaveCriticalSection();
		task_sleep(500);
	}
}

void uartIRQHandler(uint32_t token) {
	uint32_t size;
	uint32_t canRead;
	uint32_t utrstat = GET_REG(HWUarts[token].UTRSTAT);
	SET_REG(HWUarts[token].UTRSTAT, utrstat);
	size = GET_REG(HWUarts[token].UERSTAT);
	SET_REG(HWUarts[token].UERSTAT, size);
	if(utrstat & 0x40)
		size = UARTs[token].buffer_size;
	if(!(utrstat & 0x18))
		return;
	if(UARTs[token].fifo) {
		while(1) {
			uint32_t ufstat = GET_REG(HWUarts[token].UFSTAT);
			canRead = (ufstat & UART_UFSTAT_RXFIFO_FULL) | (ufstat & UART_UFSTAT_RXCOUNT_MASK);
			if (!canRead || UARTs[token].buffer_written > UARTs[token].buffer_size)
				break;
			*(UARTs[token].buffer + ((UARTs[token].buffer_written + UARTs[token].buffer_read) & (UARTs[token].buffer_size - 1))) = GET_REG8(HWUarts[token].URXH);
			UARTs[token].buffer_written = (UARTs[token].buffer_written & (UARTs[token].buffer_size - 1)) + 1;
		}
	} else {
		if(utrstat & UART_UTRSTAT_RECEIVEDATAREADY) {
			*(UARTs[token].buffer + ((UARTs[token].buffer_written + UARTs[token].buffer_read) & (UARTs[token].buffer_size - 1))) = GET_REG8(HWUarts[token].URXH);
			UARTs[token].buffer_written = (UARTs[token].buffer_written & (UARTs[token].buffer_size - 1)) + 1;
		}
	}
}

void uart_printf_handler(const char *_text)
{
	if(UartHasInit)
		uartPrint(_text);

	if(prev_printf_handler)
		prev_printf_handler(_text);
}

int uart_setup() {
	int i;

	if(UartHasInit) {
		return 0;
	}

	task_init(&uart_task, "uart reader", TASK_DEFAULT_STACK_SIZE);

	UartCommandBuffer = malloc(UartCommandBufferSize);
	memset(UartCommandBuffer, 0, UartCommandBufferSize);
	curUartCommandBuffer = UartCommandBuffer;

	for(i = 0; i < NUM_UARTS; i++) {
		clock_gate_switch(UART_CLOCKGATE+i, ON);

		// set all uarts to transmit 8 bit frames, one stop bit per frame, no parity, no infrared mode
		SET_REG(HWUarts[i].ULCON, UART_8BITS);

		// set all uarts to use polling for rx/tx, no breaks, no loopback, no error status interrupts,
		// no timeouts, pulse interrupts for rx/tx, peripheral clock. Basically, the defaults.
		SET_REG(HWUarts[i].UCON, (UART_UCON_MODE_IRQORPOLL << UART_UCON_RXMODE_SHIFT)
			| (UART_UCON_MODE_IRQORPOLL << UART_UCON_TXMODE_SHIFT));

		// Initialize the settings array a bit so the helper functions can be used properly
		UARTs[i].ureg = i;
		UARTs[i].baud = 115200;

		if (i == 5) {
			SET_REG(HWUarts[i].ULCON, UART_ULCON_UNKN);
		}

		uart_set_clk(i, UART_CLOCK_EXT_UCLK0);
		uart_set_sample_rate(i, 16);
	}

	// Set flow control
	uart_set_flow_control(0, OFF);
	uart_set_flow_control(1, ON);
	uart_set_flow_control(2, ON);
	uart_set_flow_control(3, ON);
	uart_set_flow_control(4, ON);
	uart_set_flow_control(5, ON);
	uart_set_flow_control(6, ON);

	// Reset and enable fifo
	for(i = 0; i < NUM_UARTS; i++) {
		uart_set_fifo(i, ON);
	}

	for(i = 0; i < NUM_UARTS; i++) {
		uart_set_mode(i, UART_POLL_MODE);
	}

	prev_printf_handler = addPrintfHandler(uart_printf_handler);
	UartHasInit = TRUE;

	// Set UART0 to IRQ mode with a 0x400 bytes buffer
	uart_set_rx_buf(0, UART_IRQ_MODE, 0x400);

	// Install and enable the UART interrupts
	for(i = 0; i < NUM_UARTS; i++) {
		interrupt_install(UART_INTERRUPT + i, uartIRQHandler, i);
		interrupt_enable(UART_INTERRUPT + i);
	}

	// Run a new task for handling UART commands (from UART0)
	task_start(&uart_task, &uart_run, 0);

	return 0;
}

int uart_send_break_signal(int ureg, OnOff send) {
	if (send == ON)
		SET_REG(HWUarts[ureg].UCON, GET_REG(HWUarts[ureg].UCON | UART_UCON_BREAK_SIGNAL));
	else
		SET_REG(HWUarts[ureg].UCON, GET_REG(HWUarts[ureg].UCON & (~UART_UCON_BREAK_SIGNAL)));

	return 0;
}

int uart_set(int ureg, uint32_t baud, uint32_t bits, uint32_t parity, uint32_t stop) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	UARTs[ureg].baud = baud;
	uint32_t setting;
	switch(bits) {
		case 5:
			setting = UART_5BITS;
			break;
		case 6:
			setting = UART_6BITS;
			break;
		case 7:
			setting = UART_7BITS;
			break;
		case 8:
			setting = UART_8BITS;
			break;
		default:
			return -1;
	}

	switch(parity) {
		case 0:
			break;
		case 1:
			setting |= 0x20;
			break;
		case 2:
			setting |= 0x28;
		default:
			return -1;
	}

	switch(stop) {
		case 1:
			break;
		case 2:
			setting |= 0x4;
			break;
		default:
			return -1;
	}

	SET_REG(HWUarts[ureg].ULCON, setting);
	uart_set_baud_rate(ureg, UARTs[ureg].baud);
	uart_set_fifo(ureg, ON);

	return 0;
}

int uart_set_fifo(int ureg, OnOff fifo) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	if(fifo == ON) {
		SET_REG(HWUarts[ureg].UFCON, UART_FIFO_RESET_TX | UART_FIFO_RESET_RX);
		SET_REG(HWUarts[ureg].UFCON, UART_FIFO_ENABLE);
	} else
		SET_REG(HWUarts[ureg].UFCON, UART_FIFO_DISABLE);

	UARTs[ureg].fifo = fifo;

	return 0;
}

int uart_set_clk(int ureg, int clock) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	if(clock != UART_CLOCK_PCLK && clock != UART_CLOCK_EXT_UCLK0 && clock != UART_CLOCK_EXT_UCLK1) {
		return -1; // Invalid clock		
	}

	SET_REG(HWUarts[ureg].UCON,
		(GET_REG(HWUarts[ureg].UCON) & (~UART_CLOCK_SELECTION_MASK)) | (clock << UART_CLOCK_SELECTION_SHIFT));

	UARTs[ureg].clock = clock;
	uart_set_baud_rate(ureg, UARTs[ureg].baud);

	return 0;
}

int uart_set_baud_rate(int ureg, uint32_t baud) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	uint32_t clockFrequency = (UARTs[ureg].clock == UART_CLOCK_PCLK) ? clock_get_frequency(FrequencyBasePeripheral) : clock_get_frequency(FrequencyBaseFixed);
	uint32_t div_val = clockFrequency / (baud * UARTs[ureg].sample_rate) - 1;

	SET_REG(HWUarts[ureg].UBAUD, (GET_REG(HWUarts[ureg].UBAUD) & (~UART_DIVVAL_MASK)) | div_val);

	// vanilla iBoot also does a reverse calculation from div_val and solves for baud and reports
	// the "actual" baud rate, or what is after loss during integer division

	UARTs[ureg].baud = baud;

	return 0;
}

int uart_set_sample_rate(int ureg, int rate) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	uint32_t newSampleRate;
	switch(rate) {
		case 4:
			newSampleRate = UART_SAMPLERATE_4;
			break;
		case 8:
			newSampleRate = UART_SAMPLERATE_8;
			break;
		case 16:
			newSampleRate = UART_SAMPLERATE_16;
			break;
		default:
			return -1; // Invalid sample rate
	}

	UARTs[ureg].sample_rate = rate;
	uart_set_baud_rate(ureg, UARTs[ureg].baud);

	return 0;
}

int uart_set_flow_control(int ureg, OnOff flow_control) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	if(flow_control == ON) {
		if(ureg == 0)
			return -1; // uart0 does not support flow control

		SET_REG(HWUarts[ureg].UMCON, UART_UMCON_AFC_BIT);
	} else {
		if(ureg != 0) {
			SET_REG(HWUarts[ureg].UMCON, UART_UMCON_NRTS_BIT);
		}
	}

	UARTs[ureg].flow_control = flow_control;

	return 0;
}

int uart_set_mode(int ureg, uint32_t mode) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	UARTs[ureg].mode = mode;

	if(mode == UART_POLL_MODE) {
		// Setup some defaults, like no loopback mode
		SET_REG(HWUarts[ureg].UCON, 
			GET_REG(HWUarts[ureg].UCON) & (~UART_UCON_UNKMASK) & (~UART_UCON_UNKMASK) & (~UART_UCON_LOOPBACKMODE));

		// Use polling mode
		SET_REG(HWUarts[ureg].UCON, 
			(GET_REG(HWUarts[ureg].UCON) & (~UART_UCON_RXMODE_MASK) & (~UART_UCON_TXMODE_MASK))
			| (UART_UCON_MODE_IRQORPOLL << UART_UCON_RXMODE_SHIFT)
			| (UART_UCON_MODE_IRQORPOLL << UART_UCON_TXMODE_SHIFT));
	} else if(mode == UART_IRQ_MODE) {
		// Setup some defaults, like no loopback mode
		SET_REG(HWUarts[ureg].UCON, 
			GET_REG(HWUarts[ureg].UCON) & (~UART_UCON_UNKMASK) & (~UART_UCON_UNKMASK) & (~UART_UCON_LOOPBACKMODE));

		// Use polling mode
		SET_REG(HWUarts[ureg].UCON, 
			(GET_REG(HWUarts[ureg].UCON) & (~UART_UCON_RXMODE_MASK) & (~UART_UCON_TXMODE_MASK))
			| (UART_UCON_MODE_IRQORPOLL << UART_UCON_RXMODE_SHIFT)
			| (UART_UCON_MODE_IRQORPOLL << UART_UCON_TXMODE_SHIFT));

		SET_REG(HWUarts[ureg].UCON, GET_REG(HWUarts[ureg].UCON) | UART_UCON_MODE_IRQ);
	}

	return 0;
}

int uart_set_rx_buf(int ureg, uint32_t mode, uint32_t size) {
	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	uint8_t* buffer = malloc(size);
	if (!buffer)
		return -1;

	SET_REG(HWUarts[ureg].UCON, GET_REG(HWUarts[ureg].UCON) & (~0x1000));
	if (UARTs[ureg].buffer)
		free(UARTs[ureg].buffer);
	UARTs[ureg].buffer = buffer;
	UARTs[ureg].buffer_size = size;
	UARTs[ureg].buffer_read = 0;
	UARTs[ureg].buffer_written = 0;
	uart_set_mode(ureg, mode);

	return 0;
}

int uart_write(int ureg, const char *buffer, uint32_t length) {
	if(!UartHasInit) {
		uart_setup();
	}

	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	const UARTRegisters* uart = &HWUarts[ureg];
	UARTSettings* settings = &UARTs[ureg];

	if(settings->mode != UART_POLL_MODE && settings->mode != UART_IRQ_MODE)
		return -1; // unhandled uart mode

	int written = 0;
	while(written < length) {
		if(settings->fifo) {
			// spin until the tx fifo buffer is no longer full
			while((GET_REG(uart->UFSTAT) & UART_UFSTAT_TXFIFO_FULL) != 0);
		} else {
			// spin while not Transmitter Empty
			while((GET_REG(uart->UTRSTAT) & UART_UTRSTAT_TRANSMITTEREMPTY) == 0);
		}

		if(settings->flow_control) {		// only need to do this when there is flow control
			// spin while not Transmitter Empty
			while((GET_REG(uart->UTRSTAT) & UART_UTRSTAT_TRANSMITTEREMPTY) == 0);

			// spin while not Clear To Send
			while((GET_REG(uart->UMSTAT) & UART_UMSTAT_CTS) == 0);
		}

		SET_REG(uart->UTXH, *buffer);

		buffer++;
		written++;
	}

	return written;
}

int uart_read(int ureg, char *buffer, uint32_t length, uint64_t timeout) {
	if(!UartHasInit)
		return -1;

	if(ureg >= NUM_UARTS)
		return -1; // Invalid ureg

	const UARTRegisters* uart = &HWUarts[ureg];
	UARTSettings* settings = &UARTs[ureg];

	if(settings->mode != UART_POLL_MODE && settings->mode != UART_IRQ_MODE)
		return -1; // unhandled uart mode

	uint64_t startTime = timer_get_system_microtime();
	int written = 0;
	uint32_t discard;

	if (settings->mode == UART_POLL_MODE) {
		while(written < length) {
			register int canRead = 0;
			if(settings->fifo) {
				uint32_t ufstat = GET_REG(uart->UFSTAT);
				canRead = (ufstat & UART_UFSTAT_RXFIFO_FULL) | (ufstat & UART_UFSTAT_RXCOUNT_MASK);
			} else {
				canRead = GET_REG(uart->UTRSTAT) & UART_UTRSTAT_RECEIVEDATAREADY;
			}

			if(canRead) {
				if(GET_REG(uart->UERSTAT)) {
					discard = GET_REG(uart->URXH);
				} else {
					*buffer = GET_REG(uart->URXH);
					written++;
					buffer++;
				}
			} else {
				if((timer_get_system_microtime() - startTime) >= timeout) {
					break;
				}
			}
		}
	} else if (settings->mode == UART_IRQ_MODE) {
		do {
			if(!settings->buffer_written)
				continue;
			SET_REG(uart->UCON, GET_REG(uart->UCON) & (~0x1000));
			while(settings->buffer_written) {
				*buffer = *(settings->buffer+settings->buffer_read);
				buffer++;
				written++;
				settings->buffer_read = (settings->buffer_read + 1) & (settings->buffer_size - 1);
				settings->buffer_written--;
			}
			SET_REG(uart->UCON, GET_REG(uart->UCON) | 0x1000);
		} while((timer_get_system_microtime() - startTime) < timeout);
	}

	return written;
}

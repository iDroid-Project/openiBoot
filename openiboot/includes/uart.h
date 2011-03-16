#ifndef UART_H
#define UART_H

#include "openiboot.h"

#define UART_POLL_MODE 0
#define UART_IRQ_MODE 1

typedef struct UARTSettings {
	uint32_t ureg;
	uint32_t baud;
	uint32_t sample_rate;
	OnOff flow_control;
	OnOff fifo;
	uint32_t mode;
#if defined(CONFIG_A4)
	uint8_t* buffer;
	uint32_t buffer_size;
	uint32_t buffer_read;
	uint32_t buffer_written;
#endif
	uint32_t clock;
} UARTSettings;

typedef struct UARTRegisters {
	uint32_t ULCON;
	uint32_t UCON;
	uint32_t UFCON;
	uint32_t UMCON;

	uint32_t UTRSTAT;
	uint32_t UERSTAT;
	uint32_t UFSTAT;
	uint32_t UMSTAT;

	uint32_t UTXH;
	uint32_t URXH;
	uint32_t UBAUD;
	uint32_t UDIVSLOT;
} UARTRegisters;

int uart_setup();
int uart_set_clk(int ureg, int clock);
int uart_set_sample_rate(int ureg, int rate);
int uart_set_flow_control(int ureg, OnOff flow_control);
int uart_set_mode(int ureg, uint32_t mode);
int uart_set_baud_rate(int ureg, uint32_t baud);
int uart_set_fifo(int ureg, OnOff fifo);
int uart_set(int ureg, uint32_t baud, uint32_t bits, uint32_t parity, uint32_t stop);
int uart_send_break_signal(int ureg, OnOff send);
uint32_t uart_set_rx_buf(int ureg, uint32_t mode, uint32_t size);


int uart_write(int ureg, const char *buffer, uint32_t length);
int uart_read(int ureg, char *buffer, uint32_t length, uint64_t timeout);

extern int UartHasInit;

#endif


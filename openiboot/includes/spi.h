#ifndef SPI_H
#define SPI_H

#include "openiboot.h"

typedef enum SPIClockSource {
	PCLK = 0,
	NCLK = 1
} SPIClockSource;

typedef enum SPIWordSize {
	SPIWordSize8 = 8,
	SPIWordSize16 = 16,
	SPIWordSize32 = 32
} SPIWordSize;

typedef struct SPIInfo {
	int wordSize;
	int isActiveLow;
	int lastClockEdgeMissing;
	SPIClockSource clockSource;
	int baud;
	int isMaster;
	int option5;
	const volatile uint8_t* txBuffer;
	volatile int txCurrentLen;
	volatile int txTotalLen;
	volatile uint8_t* rxBuffer;
	volatile int rxCurrentLen;
	volatile int rxTotalLen;
	volatile int counter;
	int setupOptions;
	volatile int txDone;
	volatile int rxDone;
} SPIInfo;

int spi_setup();
int spi_tx(int port, const uint8_t* buffer, int len, int block, int unknown);
int spi_rx(int port, uint8_t* buffer, int len, int block, int noTransmitJunk);
int spi_txrx(int port, const uint8_t* outBuffer, int outLen, uint8_t* inBuffer, int inLen, int block);
void spi_set_baud(int port, int baud, SPIWordSize wordSize, int isMaster, int isActiveLow, int lastClockEdgeMissing);
void spi_on_off(uint8_t spi, OnOff on_off);
int spi_status(uint8_t spi);

#endif

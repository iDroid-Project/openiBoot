#ifndef HW_SPI_H
#define HW_SPI_H

// Device
#define SPI0 0x3C300000
#define SPI1 0x3CE00000
#define SPI2 0x3D200000
#define SPI3 0x3DA00000
#define SPI4 0x3E100000

// Registers

#define CONTROL 0x0
#define SETUP 0x4
#define STATUS 0x8
#define SPIPIN 0xC
#define TXDATA 0x10
#define RXDATA 0x20
#define CLKDIVIDER 0x30
#define SPCNT 0x34
#define SPIDD 0x38
#define UNKREG4 0x3C
#define UNKREG5 0x40
#define UNKREG6 0x44
#define UNKREG7 0x48
#define TXBUFFERLEN 0x4C

// Setup register
#define SPISETUP_NO_TRANSMIT_JUNK           (1 << 0)    // 1 bit
#define SPISETUP_LAST_CLOCK_EDGE_MISSING    (1 << 1)    // 1 bit
#define SPISETUP_IS_ACTIVE_LOW              (1 << 2)    // 1 bit
#define SPISETUP_IS_MASTER                  (1 << 3)    // 2 bits
#define SPISETUP_OPTION5                    (1 << 5)    // 2 bits
#define SPISETUP_UNKN1                      (1 << 7)    // 1 bit
#define SPISETUP_UNKN2                      (1 << 8)    // 1 bit
#define SPISETUP_CLOCKSOURCE                (1 << 14)   // 1 bit
#define SPISETUP_WORDSIZE                   (1 << 15)   // 2 bits
#define SPISETUP_UNKN3                      (1 << 21)   // 1 bit

// Values
#define MAX_TX_BUFFER 16
#define TX_BUFFER_LEFT(x) GET_BITS(x, 6, 5)
#define RX_BUFFER_LEFT(x) GET_BITS(x, 11, 5)

#define CLOCK_SHIFT 14
#define WORDSIZE_SHIFT 15
#define MAX_DIVIDER 0x3FF

#define SPI0_CLOCKGATE 0xE
#define SPI1_CLOCKGATE 0xF
#define SPI2_CLOCKGATE 0x10
#define SPI3_CLOCKGATE 0x11
#define SPI4_CLOCKGATE 0x12

#define SPI0_IRQ 0x9
#define SPI1_IRQ 0xA
#define SPI2_IRQ 0xB
#define SPI3_IRQ 0x1C
#define SPI4_IRQ 0x37

#define GPIO_SPI0_CS0 0x0
#define GPIO_SPI1_CS0 0x406
#define GPIO_SPI4_CS0 0xA07

#define NUM_SPIPORTS 5

typedef struct SPIRegister {
	uint32_t control;
	uint32_t setup;
	uint32_t status;
	uint32_t pin;
	uint32_t txData;
	uint32_t rxData;
	uint32_t clkDivider;
	uint32_t cnt;
	uint32_t idd;
	uint32_t unkReg4;
	uint32_t unkReg5;
	uint32_t unkReg6;
	uint32_t unkReg7;
	uint32_t txBufferLen;
} SPIRegister;

#endif


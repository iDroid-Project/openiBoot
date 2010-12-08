#ifndef HW_SPI_H
#define HW_SPI_H

// Device
#define SPI0 0x82000000
#define SPI1 0x82100000
#define SPI2 0x82200000
#define SPI3 0x82300000
#define SPI4 0x82400000

#define SPI0_CLOCKGATE 0x9
#define SPI1_CLOCKGATE 0xA
#define SPI2_CLOCKGATE 0xB
#define SPI3_CLOCKGATE 0xC
#define SPI4_CLOCKGATE 0xD

#define SPI0_IRQ 0x1D
#define SPI1_IRQ 0x1C
#define SPI2_IRQ 0x1B
#define SPI3_IRQ 0x1A
#define SPI4_IRQ 0x19

#define NUM_SPIPORTS 5

// Registers

#define SPI_CONTROL(x)	((x)+0x0)

#endif


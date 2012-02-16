#ifndef HW_SPI_H
#define HW_SPI_H

// Device
#define SPI0 0x82000000
#define SPI1 0x82100000
#define SPI2 0x82200000
#define SPI3 0x82300000
#define SPI4 0x82400000

#define SPI0_CLOCKGATE 0x2B
#define SPI1_CLOCKGATE 0x2C
#define SPI2_CLOCKGATE 0x2D
#define SPI3_CLOCKGATE 0x2E
#define SPI4_CLOCKGATE 0x2F

#define SPI0_IRQ 0x1D
#define SPI1_IRQ 0x1E
#define SPI2_IRQ 0x1F
#define SPI3_IRQ 0x20
#define SPI4_IRQ 0x21

#define NUM_SPIPORTS 5

// Setup register
#define SPISETUP_NO_TRANSMIT_JUNK           (1 << 0)    // 1 bit
#define SPISETUP_LAST_CLOCK_EDGE_MISSING    (1 << 1)    // 1 bit
#define SPISETUP_IS_ACTIVE_LOW              (1 << 2)    // 1 bit
#define SPISETUP_IS_MASTER                  (1 << 3)    // 2 bits
#define SPISETUP_OPTION5                    (1 << 5)    // 2 bits
#define SPISETUP_UNKN1                      (1 << 7)    // 1 bit
#define SPISETUP_UNKN2                      (1 << 8)    // 1 bit
#define SPISETUP_CLOCKSOURCE                (1 << 14)   // 1 bit
#define SPISETUP_WORDSIZE_SHIFT             15			// 2 bits
#define SPISETUP_UNKN3                      (1 << 21)   // 1 bit

#define STATUS_RX	1

#define TX_BUFFER_LEFT(x) (((x) >> 6) & 0x1f)
#define RX_BUFFER_LEFT(x) (((x) >> 11) & 0x1f)

#define MAX_TX_BUFFER 0x1f

#endif


#ifndef HW_NOR_H
#define HW_NOR_H

// The same prototocol works for a few different chips. The datasheet for
// one of them (atmel AT25DF081A) can be found here:
// http://www.atmel.com/dyn/resources/prod_documents/doc8715.pdf

// Values
#define NOR_SPI_READ 0x03           // read bytes
#define NOR_SPI_WREN 0x06           // write enable
#define NOR_SPI_WRDI 0x04           // write disable
#define NOR_SPI_PRGM 0x02           // write individual bytes
#define NOR_SPI_RDSR 0x05           // read status register
#define NOR_SPI_WRSR 0x01           // write status register
#define NOR_SPI_EWSR 0x50           // enable write status register
#define NOR_SPI_AIPG 0xAD           // write using AAI (auto address incrementing)
#define NOR_SPI_ERSE_4KB 0x20       // erase 4KB
#define NOR_SPI_JEDECID 0x9F        // get JEDEC device info

#define NOR_SPI_SR_BUSY 0           // write in progress
#define NOR_SPI_SR_WEL 1            // write enable latch
#define NOR_SPI_SR_BP0 2            // block protect 0
#define NOR_SPI_SR_BP1 3            // block protect 1
#define NOR_SPI_SR_BP2 4            // block protect 2
#define NOR_SPI_SR_BP3 5            // block protect 3
#define NOR_SPI_SR_AAI 6            // auto address increment programming
#define NOR_SPI_SR_BPL 7            // block protect write disable
#define NOR_SPI_SR_BP_ALL	((1<<NOR_SPI_SR_BP0)|(1<<NOR_SPI_SR_BP1)|(1<<NOR_SPI_SR_BP2)|(1<<NOR_SPI_SR_BP3))

#define NOR_T_BP 10000
#define NOR_T_SE 25000

#if defined(CONFIG_S5L8720)
	#define NOR_CS 0x406
	#define NOR_SPI 1
	#define NOR_MAX_READ 16		// The datasheet for the 8720 chip at least doesn't mention anything
								// about a max number of bytes per read, although these values are in
								// iBoot. Removing the max would probably give a speed increase and
								// shouldn't have any bad effects... try this once nor is fixed again
								// ~kleemajo
#elif defined(CONFIG_S5L8920)
	#define NOR_CS 0x1204
	#define NOR_SPI 0
	#define NOR_MAX_READ 4
#elif defined(CONFIG_S5L8900)
	#define NOR_CS	GPIO_SPI0_CS0
	#define NOR_SPI	0
	#define NOR_MAX_READ 16
#endif

#endif


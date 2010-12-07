#ifndef HW_NOR_H
#define HW_NOR_H

// Device
#define NOR 0x24000000

// Registers
#define NOR_COMMAND 0xAAAA
#define LOCK 0x5554
#define VENDOR 0
#define DEVICE 2

// Values
#define DATA_MODE 0xFFFF
#define COMMAND_UNLOCK 0xAAAA
#define COMMAND_LOCK 0xF0F0
#define COMMAND_IDENTIFY 0x9090
#define COMMAND_WRITE 0xA0A0
#define COMMAND_ERASE 0x8080
#define ERASE_DATA 0x3030
#define LOCK_UNLOCK 0x5555

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

#define NOR_T_BP 10000
#define NOR_T_SE 25000

#define NOR_CLOCKGATE 0x1E

#define NOR_SPI_CS0 GPIO_SPI1_CS0
#define NOR_SPI_PORT 1

#define NOR_TIMEOUT 50000

#endif


#ifndef HW_SDIO_H
#define HW_SDIO_H


#define SDIO_CLOCKGATE 0x24
#define SDIO_INTERRUPT 0x26
#define SDIO_GPIO_DEVICE_RESET 0x6

// hardware registers
#define SDIO			0x80000000
#define SDIO_SDMAADD		0x0
#define SDIO_BLK		0x4
#define SDIO_BLKCNT		0x6
#define SDIO_ARGU		0x8
#define SDIO_TRNMOD		0xC
#define SDIO_CMD		0xE
#define SDIO_RESP0		0x10
#define SDIO_RESP1		0x14
#define SDIO_RESP2		0x18
#define SDIO_RESP3		0x1C
#define SDIO_DBUF		0x20
#define SDIO_STATE		0x24
#define SDIO_HOSTCTL	0x28
#define SDIO_PWRCON		0x29
#define SDIO_BLKGAP		0x2A
#define SDIO_WAKCON		0x2B
#define SDIO_CLKCON		0x2C
#define SDIO_TIMEOUTCON	0x2E
#define SDIO_SWRESET	0x2F
#define SDIO_IRQ		0x30
#define SDIO_IRQEN		0x34
#define SDIO_ISREN		0x38
#define SDIO_ACMD12ERRSTAT	0x3C
#define SDIO_CAPLO		0x40
#define SDIO_CAPHI		0x44
#define SDIO_MAXCAPLO		0x48
#define SDIO_MAXCAPHI		0x4C
#define SDIO_ACMD12		0x50
#define SDIO_ADMA		0x54
#define SDIO_INFO		0xFC

// SWRESET
#define SDIO_SWRESET_RSTDAT	(1 << 2)
#define SDIO_SWRESET_RSTCMD	(1 << 1)
#define SDIO_SWRESET_RSTALL (1 << 0)
#define SDIO_SWRESET_ALL	(SDIO_SWRESET_RSTDAT | SDIO_SWRESET_RSTCMD | SDIO_SWRESET_RSTALL)

//settings
#define SDIO_Base_Frequency 51300000 // 51.3MHz
#define SDIO_Max_Frequency 51900000 // 51.9MHz

#endif

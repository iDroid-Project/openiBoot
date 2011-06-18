#ifndef HW_SDIO_H
#define HW_SDIO_H

#define SDIO 0x80000000
#define SDIO_CLOCKGATE 0x24
#define SDIO_INTERRUPT 0x26
#define SDIO_GPIO_DEVICE_RESET 0x6
#define SDIO_SDMAADD		0x0
#define SDIO_BLK		0x4
#define SDIO_ARGU		0x8
#define SDIO_CMD		0xC
#define SDIO_RESP0		0x10
#define SDIO_RESP1		0x14
#define SDIO_RESP2		0x18
#define SDIO_RESP3		0x1C
#define SDIO_DBUF		0x20
#define SDIO_STATE		0x24
#define SDIO_CTRL1		0x28
#define SDIO_CLRL2		0x2C
#define SDIO_IRQ		0x30
#define SDIO_IRQEN		0x34
#define SDIO_ISREN		0x38
#define SDIO_ACM12ERRSTAT	0x3C
#define SDIO_CAPLO		0x40
#define SDIO_CAPHI		0x44
#define SDIO_MAXCAPLO		0x48
#define SDIO_MAXCAPHI		0x4C
#define SDIO_ACM12		0x50
#define SDIO_ADMA		0x54
#define SDIO_INFO		0xFC
#define SDIO_BLK_SIZE(x)	((x << 16) >> 16)
#define SDIO_BLK_COUNT(x)	(x >> 16)
#define SDIO_CMD_TRXMODE(x)	((x << 16) >> 16)
#define SDIO_CMD_COMMAND(x)	(x >> 16)
#define SDIO_CTRL1_HOST(x)	(x & 0xFF)
#define SDIO_CTRL1_POWER(x)	((x & 0xFF00) >> 8)
#define SDIO_CTRL1_BLOCKGAP(x)	((x & 0xFF0000) >> 16)
#define SDIO_CTRL1_WAKEUP(x)	(x >> 24)
#define SDIO_CTRL2_CLOCK(x)	((x << 16) >> 16)
#define SDIO_CTRL2_TIMEOUT(x)	((x & 0xFF0000) >> 16)
#define SDIO_CTRL2_SOFTRESET(x)	(x >> 24)
#define SDIO_IRQ_STATUS(x)	((x << 16) >> 16)
#define SDIO_IRQ_ERRSTAT(x)	(x >> 16)
#define SDIO_IRQEN_STATUS(x)	((x << 16) >> 16)
#define SDIO_IRQEN_ERRSTATUS(x)	(x >> 16)
#define SDIO_ISREN_STATUS(x)	((x << 16) >> 16)
#define SDIO_ISREN_ERRSTATUS(x)	(x >> 16)
#define SDIO_ACM12_EVTSTATUS(x)	((x << 16) >> 16)
#define SDIO_ACM12_IRQSTATUS(x)	(x >> 16)
#define SDIO_ADMA_ERRSTATUS(x)	((x << 16) >> 16)
#define SDIO_ADMA_ADDRESS(x)	(x >> 16)
#define SDIO_INFO_SLOTIRQ(x)	((x << 16) >> 16)
#define SDIO_INFO_VERSION(x)	(x >> 16)

#endif

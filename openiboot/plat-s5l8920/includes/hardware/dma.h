#ifndef HW_DMA_H
#define HW_DMA_H

#include "hardware/a4.h"

// Device
#define DMA 0x87000000

// Registers
#define DMA_OFF 0x8
#define DMA_STATUS 0x10
#define DMA_INTERRUPT_ERROR 0x18
#define DMA_CHANNEL_INTERRUPT_BASE 0x2b

// Channel Registers
#define DMA_SETTINGS 0x4
#define DMA_TXRX_REGISTER 0x8
#define DMA_SIZE 0xC

// AES Registers
#define DMA_AES 0x800000
#define DMA_AES_KEY_0 0x20
#define DMA_AES_KEY_1 0x24
#define DMA_AES_KEY_2 0x28
#define DMA_AES_KEY_3 0x2C
#define DMA_AES_KEY_4 0x30
#define DMA_AES_KEY_5 0x34
#define DMA_AES_KEY_6 0x38
#define DMA_AES_KEY_7 0x3C

#endif

#ifndef HW_NOR_H
#define HW_NOR_H

#include "openiboot.h"

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

#endif


#ifndef  VFL_H
#define  VFL_H

#include "openiboot.h"

#define ERROR_ARG	0x80000001
#define ERROR_NAND	0x80000002
#define ERROR_EMPTY	0x80000003

uint32_t VFL_ReadBlockDriverSign(uint32_t* buffer, uint32_t bytesToRead);
uint32_t VFL_ReadBlockZeroSign(uint32_t* buffer, uint32_t bytesToRead);

#endif //VFL_H

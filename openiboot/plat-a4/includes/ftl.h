#ifndef  FTL_H
#define  FTL_H

#include "openiboot.h"

#define ERROR_ARG	0x80000001
#define ERROR_NAND	0x80000002
#define ERROR_EMPTY	0x80000003

uint32_t get_scfg_info(uint32_t ce, uint32_t* headerBuffer, uint32_t* dataBuffer, uint32_t bytesToRead, char* infoTypeName, uint32_t nameSize, uint32_t zero1, uint32_t zero2);

#endif //FTL_H

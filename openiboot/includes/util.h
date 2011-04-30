#ifndef UTIL_H
#define UTIL_H

#include "openiboot.h"
#include <stdarg.h>

#ifdef DEBUG
#define DebugPrintf bufferPrintf
#else
#define DebugPrintf(...)
#endif

typedef void (*printf_handler_t)(const char *_str);

void panic();
void system_panic(const char* format, ...);

void __assert(const char* file, int line, const char* m);
void* memset(void* x, int fill, uint32_t size);
void* memcpy(void* dest, const void* src, uint32_t size);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strchr(const char* s1, int s2);
char* strstr(const char* s1, const char* s2);
char* strdup(const char* str);
char* strcpy(char* dst, const char* src);
int memcmp(const void* s1, const void* s2, uint32_t size);
void* memmove(void *dest, const void* src, size_t length);
size_t strlen(const char* str);
int tolower(int c);
int putchar(int c);
unsigned long int parseNumber(const char* str);
unsigned long int strtoul(const char* str, char** endptr, int base);
char** tokenize(char* commandline, int* argc);
void dump_memory(uint32_t start, int length);
void buffer_dump_memory(uint32_t start, int length);
void hexdump(void *start, int length);
void buffer_dump_memory2(uint32_t start, int length, int width);
int addToBuffer(const char* toBuffer, int len);
printf_handler_t addPrintfHandler(printf_handler_t);
void bufferPrint(const char* toBuffer);
void bufferPrintf(const char* format, ...);
void bufferFlush(char* destination, size_t length);
char* getScrollback();
size_t getScrollbackLen();

void hexToBytes(const char* hex, uint8_t** buffer, int* bytes);
void bytesToHex(const uint8_t* buffer, int bytes);

uint32_t crc32(uint32_t* ckSum, const void *buffer, size_t len);
uint32_t adler32(uint8_t *buf, int32_t len);

const char *strerr(error_t _e);

uint32_t next_power_of_two(uint32_t n);
inline void auto_store(void *_ptr, size_t _sz, uint32_t _val);

#include "printf.h"
#include "malloc-2.8.3.h"

#endif

/*
 * util.h
 *
 * Copyright 2010 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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

/**
 * panic
 *
 * OpeniBoot has just shit a brick, and now you are fucked - time for a reboot
 *
 */
void panic();

/**
 * system_panic
 *
 * The bottom wiping performed prior to a total and sudden panic
 *
 * @param format Format of what is being printed to the buffer
 *
 */
void system_panic(const char* format, ...);

/**
 * __assert
 *
 * Sucks to be you right now...
 *
 * @param file Need help with this? if you do, stop looking at this code right now
 * @param line I'll give you a clue, this will be a number
 * @param m Whatever it is you wish to shout about
 *
 */
void __assert(const char* file, int line, const char* m);

/**
 * memset
 *
 * Fills a block of memory
 * 
 * @param x The bread
 * @param fill The filling
 * @param size The amount of filling required
 */
void* memset(void* x, int fill, uint32_t size);

/**
 * memcpy
 *
 * Copies a block of memory
 * 
 * @param dest Pointer to the destination
 * @param src Pointer to the source
 * @param size Well how many 'kin bytes do you need?
 */
void* memcpy(void* dest, const void* src, uint32_t size);

/**
 * strcmp
 *
 * Compares strings an' shit yo'
 * 
 * @param s1 First string
 * @param s2 String to compare against s1
 */
int strcmp(const char* s1, const char* s2);

/**
 * strncmp
 *
 * Compare characters of two strings
 * 
 * @param s1 First string
 * @param s2 String to compare against s1
 * @param n Maximum number of characters to compare
 */
int strncmp(const char* s1, const char* s2, size_t n);

/**
 * strchr
 *
 * Locate first occurrence of character in string
 * 
 * @param s1 First string
 * @param s2 String to compare against s1
 */
char* strchr(const char* s1, int s2);

/**
 * strstr
 * Find sharp shiny things in a heap of dead grass
 * 
 * @param s1 Needle
 * @param s2 Haystack
 */
char* strstr(const char* s1, const char* s2);

/**
 * strdup
 * Just like cloning, only different
 * 
 * @param str String to duplicate
 */
char* strdup(const char* str);

/**
 * strcpy
 * I couldn't think of anything witty for this one, and tbh you should know what it does
 * 
 * @param dst Destination
 * @param src Source
 */
char* strcpy(char* dst, const char* src);

/**
 * memcmp
 * compare bytes in memory
 * 
 * @param s1 What you want to compare
 * @param s2 What you want to compare it against
 * @param size mmmm, tasty bytes
 */
int memcmp(const void* s1, const void* s2, uint32_t size);

/**
 * memmove
 * Adapted from public domain memmove function from  David MacKenzie <djm@gnu.ai.mit.edu>.
 * 
 * @param dest Destination
 * @param src Source
 * @param length mmmm, tasty bytes
 */
void* memmove(void *dest, const void* src, size_t length);

/**
 * strlen
 * How long is my di.. I mean string
 * 
 * @param str The string
 */
size_t strlen(const char* str);

/**
 * tolower
 * Makes it all proper little an' that
 * 
 * @param c letter
 */
int tolower(int c);

/**
 * putchar
 * Prints a letter to the buffer
 * 
 * @param c letter
 */
int putchar(int c);


/**
 * puts
 * Prints a string to the buffer
 * 
 * @param str something stringy
 */
int puts(const char *str);

/**
 * parseNumber
 * Automagically makes numerical things from stringy things
 * 
 * @param str something stringy
 */
unsigned long int parseNumber(const char* str);

/**
 * strtoul
 * Convert a string to an unsigned long
 * 
 * @param str something stringy
 * @param endptr the next char in str after the numerical value
 * @param base The data encoding (int)
 */
unsigned long int strtoul(const char* str, char** endptr, int base);

/**
 * tokenize
 * ooh tokens
 * 
 * @param commandline Teh commands
 * @param argc Arguments, women love them
 */
char** tokenize(char* commandline, int* argc);

/**
 * dump_memory
 * Quick - get a bucket!
 * 
 * @param start Where do we start?
 * @param length Well, how much do you want exactly?
 */
void dump_memory(uint32_t start, int length);

/**
 * buffer_dump_memory
 * Cause OpeniBoot to have diahorea
 * 
 * @param start Where do we start?
 * @param length Well, how much do you want exactly?
 */
void buffer_dump_memory(uint32_t start, int length);

/**
 * hexdump
 * Cause OpeniBoot to have hexidecimal-diahorea
 * 
 * @param start Where do we start?
 * @param length Well, how much do you want exactly?
 */
void hexdump(void *start, int length);

/**
 * buffer_dump_memory2
 * Cause OpeniBoot to have diahorea2.0
 * 
 * @param start Where do we start?
 * @param length Well, how much do you want exactly?
 */
void buffer_dump_memory2(uint32_t start, int length, int width);
int addToBuffer(const char* toBuffer, int len);
printf_handler_t addPrintfHandler(printf_handler_t);
void bufferPrint(const char* toBuffer);
void bufferPrintf(const char* format, ...);
void bufferFlush(char* destination, size_t length);
char* getScrollback();
size_t getScrollbackLen();

/**
 * hexToBytes
 * This should need no explanation
 * 
 * @param hex hexskidecskimal
 * @param buffer no, not those things trains bump in to
 * @param bytes well, how many?
 */
void hexToBytes(const char* hex, uint8_t** buffer, int* bytes);

/**
 * bytesToHex
 * The reverse of the above - This should also need no explanation
 * 
 * @param buffer no, not those things trains bump in to
 * @param bytes well, how many?
 */
void bytesToHex(const uint8_t* buffer, int bytes);

uint32_t crc32(uint32_t* ckSum, const void *buffer, size_t len);
uint32_t adler32(uint8_t *buf, int32_t len);

const char *strerr(error_t _e);

uint32_t next_power_of_two(uint32_t n);
inline void auto_store(void *_ptr, size_t _sz, uint32_t _val);

#include "printf.h"
#include "malloc-2.8.3.h"

#endif

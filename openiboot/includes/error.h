/**
 *  @file
 *
 *  This file defines errors OiB can return.
 *
 *  @defgroup Error
 */

/**
 * error.h
 *
 * Copyright 2011 iDroid Project
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

#ifndef  ERROR_H
#define  ERROR_H



#define ERROR_BIT			(0x80000000)
#define ERROR(x)			((x) | ERROR_BIT)
#define ERROR_CODE(x)		((x) &~ ERROR_BIT)
#define FAILED(x)			(((x) & ERROR_BIT) != 0)
#define SUCCEEDED(x)		(((x) & ERROR_BIT) == 0)
#define SUCCESS_VALUE(x)	((x) &~ ERROR_BIT)

#define FAIL_ON(x, y)			do { if(x) { return y; } } while(0)
#define SUCCEED_ON(x)			FAIL_ON((x), SUCCESS)
#define CHAIN_FAIL(x)			do { error_t __val = (x); if(FAILED(__val)) { return __val; } } while(0)
#define CHAIN_FAIL_STORE(x, y)	do { if(SUCCEEDED(x)) { x = (y); } } while(0)

/**
 *  The error type enumeration.
 *
 *  This is the different kinds of errors OiB can return upon execution
 *  of a function.
 *
 *  @ingroup Error
 */
enum
{
	SUCCESS = 0,                /* Success */
	EINVAL = ERROR(1),          /* Invalid argument */
	EIO = ERROR(2),             /* I/O Error */
	ENOENT = ERROR(3),          /* No such file */
	ENOMEM = ERROR(4),          /* Out of Memory */
	ETIMEDOUT = ERROR(5),       /* Timed out */
};

typedef uint32_t error_t;

#endif //ERROR_H

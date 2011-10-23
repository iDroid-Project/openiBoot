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

#define FAIL_ON(x, y)		do { if(x) { return y; } } while(0)
#define SUCCEED_ON(x)		FAIL_ON((x), SUCCESS)
#define CHAIN_FAIL(x)		do { error_t val = (x); if(FAILED(val)) { return val; } } while(0)

enum
{
	SUCCESS = 0,
	EINVAL = ERROR(1),
	EIO = ERROR(2),
	ENOENT = ERROR(3),
	ENOMEM = ERROR(4),
	ETIMEDOUT = ERROR(5),
};

typedef uint32_t error_t;

#endif //ERROR_H

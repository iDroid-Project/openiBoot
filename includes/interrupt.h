/*
 * interrupt.h
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

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "openiboot.h"
#include "hardware/interrupt.h"

typedef void (*InterruptServiceRoutine)(uint32_t token);

typedef struct InterruptHandler {
        InterruptServiceRoutine handler;
        uint32_t token;
        uint32_t useEdgeIC;
} InterruptHandler;

extern InterruptHandler InterruptHandlerTable[VIC_MaxInterrupt];

int interrupt_setup();
int interrupt_install(int irq_no, InterruptServiceRoutine handler, uint32_t token);
int interrupt_enable(int irq_no);
int interrupt_disable(int irq_no);
int interrupt_clear(int irq_no);

int interrupt_set_int_type(int irq_no, uint8_t type);

#endif

/**
 * accel.h
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


/*
 This is depreciated, replaced with module system.
 */

#ifndef ACCEL_H
#define ACCEL_H

/**
 *  @file 
 *
 *  This file defines the functions for collecting accelerometer status and
 *  setting up the accelerometer.
 *
 */

#include "openiboot.h"

int accel_setup();

int accel_get_x();
int accel_get_y();
int accel_get_z();

#endif

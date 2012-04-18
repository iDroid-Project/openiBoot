/**
 *  @file 
 *
 *  Header file for the Ambient Light sensor (ALS)
 *
 *  Defines the setup and functionality of the Ambient Light Sensor.
 *
 *  @defgroup ALS
 */

/*
 * als.h
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

#ifndef ALS_H
#define ALS_H

/**
 *  Setup the Ambient Light Sensor
 *
 *  Returns 0 on correct initialisation and -1 on incorrect initialisation.
 *
 *  @ingroup ALS
 */
int als_setup();

/**
 *  Set the high light threshold value
 *
 *  @ingroup ALS
 */
void als_sethighthreshold(unsigned int value);

/**
 *  Set the low light threshold value
 *
 *  @ingroup ALS
 */
void als_setlowthreshold(unsigned int value);

/**
 *  Collect data from the Ambient Light Sensor
 *
 *  @ingroup ALS
 */
unsigned int als_data();

void als_setchannel(int channel);
void als_enable_interrupt();
void als_disable_interrupt();

#endif

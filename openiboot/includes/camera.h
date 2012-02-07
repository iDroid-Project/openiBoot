/**
 *	@file 
 *
 *  Header file for camera functions in OiB
 *
 *  @defgroup Camera
 */

/**
 * actions.h
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

#ifndef CAMERA_H
#define CAMERA_H

/**
 *	Setup the camera to work in OiB
 *
 *	Note: The camera setup currently on recognises the existence of the camera and returns its model number
 *
 *	Returns 0 on successful setup.
 *
 *  @ingroup Camera
 */
int camera_setup();

#endif

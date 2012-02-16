/*
 * audiohw-null.c - Dummy audio driver
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

#include "openiboot.h"
#include "audiocodec.h"

void audiohw_init()
{
}

void audiohw_postinit()
{
}

void audiohw_play_pcm(const void* addr_in, uint32_t size, int use_speaker)
{
}

void audiohw_set_headphone_vol(int vol_l, int vol_r)
{
}

void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
}

void audiohw_set_aux_vol(int vol_l, int vol_r)
{
}

int audiohw_transfers_done()
{
	return 1;
}

void audiohw_pause()
{
}

void audiohw_resume()
{
}

uint32_t audiohw_get_position()
{
	return 0;
}

uint32_t audiohw_get_total()
{
	return 0;
}

void audiohw_mute(int mute)
{
}

void audiohw_switch_normal_call(int in_call)
{
}

void audiohw_set_speaker_vol(int vol)
{
}

const struct sound_settings_info audiohw_settings[] = {
};

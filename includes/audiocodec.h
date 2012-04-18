/*
 * audiocodec.h
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

struct sound_settings_info {
	const char *unit;
	char numdecimals;
	char steps;
	short minval;
	short maxval;
	short defaultval;
};

enum {
	SOUND_VOLUME = 0,
	SOUND_BASS,
	SOUND_TREBLE,
	SOUND_BALANCE,
	SOUND_CHANNELS,
	SOUND_STEREO_WIDTH,
#if defined(HAVE_RECORDING)
	SOUND_LEFT_GAIN,
	SOUND_RIGHT_GAIN,
	SOUND_MIC_GAIN,
#endif
	SOUND_BASS_CUTOFF,
	SOUND_TREBLE_CUTOFF,
	SOUND_LAST_SETTING, /* Keep this last */
};

void audiohw_init();
void audiohw_postinit();
void audiohw_play_pcm(const void* addr_in, uint32_t size, int use_speaker);
void audiohw_set_headphone_vol(int vol_l, int vol_r);
void audiohw_set_lineout_vol(int vol_l, int vol_r);
void audiohw_set_aux_vol(int vol_l, int vol_r);
int audiohw_transfers_done();
void audiohw_pause();
void audiohw_resume();
uint32_t audiohw_get_position();
uint32_t audiohw_get_total();
void audiohw_mute(int mute);
void audiohw_switch_normal_call(int in_call);
void audiohw_set_speaker_vol(int vol);


extern const struct sound_settings_info audiohw_settings[];

/*
 * audiohw.c - OpeniBoot Audio functions
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
#include "commands.h"
#include "wmcodec.h"
#include "util.h"

MODULE_INIT_BOOT(audiohw_init);
MODULE_INIT(audiohw_postinit);

static int cmd_audiohw_position(int argc, char** argv)
{
	bufferPrintf("playback position: %u / %u\r\n", audiohw_get_position(), audiohw_get_total());

	return 0;
}
COMMAND("audiohw_position", "print the playback position", cmd_audiohw_position);

static int cmd_audiohw_pause(int argc, char** argv)
{
	audiohw_pause();
	bufferPrintf("Paused.\r\n");

	return 0;
}
COMMAND("audiohw_pause", "pause playback", cmd_audiohw_pause);

static int cmd_audiohw_resume(int argc, char** argv)
{
	audiohw_resume();
	bufferPrintf("Resumed.\r\n");

	return 0;
}
COMMAND("audiohw_resume", "resume playback", cmd_audiohw_resume);

static int cmd_audiohw_transfers_done(int argc, char** argv)
{
	bufferPrintf("transfers done: %d\r\n", audiohw_transfers_done());

	return 0;
}
COMMAND("audiohw_transfers_done", "display how many times the audio buffer has been played", cmd_audiohw_transfers_done);

static int cmd_audiohw_play_pcm(int argc, char** argv)
{
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len> [use-headphones]\r\n", argv[0]);
		return -1;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	uint32_t useHeadphones = 0;

	if(argc > 3)
		useHeadphones = parseNumber(argv[3]);

	if(useHeadphones)
	{
		bufferPrintf("playing PCM 0x%x - 0x%x using headphones\r\n", address, address + len);
		audiohw_play_pcm((void*)address, len, FALSE);
	} else
	{
		bufferPrintf("playing PCM 0x%x - 0x%x using speakers\r\n", address, address + len);
		audiohw_play_pcm((void*)address, len, TRUE);
	}

	return 0;
}
COMMAND("audiohw_play_pcm", "queue some PCM data for playback", cmd_audiohw_play_pcm);

static int cmd_audiohw_headphone_vol(int argc, char** argv)
{
	if(argc < 2)
	{
		bufferPrintf("%s <left> [right] (between 0:%d and 63:%d dB)\r\n", argv[0], audiohw_settings[SOUND_VOLUME].minval, audiohw_settings[SOUND_VOLUME].maxval);
		return -1;
	}

	int left = parseNumber(argv[1]);
	int right;

	if(argc >= 3)
		right = parseNumber(argv[2]);
	else
		right = left;

	audiohw_set_headphone_vol(left, right);

	bufferPrintf("Set headphone volumes to: %d / %d\r\n", left, right);

	return 0;
}
COMMAND("audiohw_headphone_vol", "set the headphone volume", cmd_audiohw_headphone_vol);

static int cmd_audiohw_speaker_vol(int argc, char** argv)
{
	if(argc < 2)
	{
		bufferPrintf("%s <loudspeaker volume> (between 0 and 100)\r\n", argv[0]);
		return -1;
	}

	int vol = parseNumber(argv[1]);

	audiohw_set_speaker_vol(vol);
	bufferPrintf("Set speaker volume to: %d\r\n", vol);

	return 0;
}
COMMAND("audiohw_speaker_vol", "set the speaker volume", cmd_audiohw_speaker_vol);

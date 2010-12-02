#include "openiboot.h"
#include "wmcodec.h"

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

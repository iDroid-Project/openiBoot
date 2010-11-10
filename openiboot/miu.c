#include "openiboot.h"
#include "miu.h"
#include "clock.h"
#include "hardware/power.h"
#include "util.h"
#ifdef CONFIG_IPHONE_4
#include "chipid.h"
#endif

int miu_setup() {
#ifndef CONFIG_IPHONE_4
	if(POWER_ID_EPOCH(*((uint8_t*)(POWER + POWER_ID))) != 3) {
#else
	if(POWER_ID_EPOCH(GET_REG(POWER + POWER_ID)) != chipid_get_power_epoch()) {
#endif
		// Epoch mismatch
		bufferPrintf("miu: epoch mismatch\r\n");
		return -1;
	}

#ifndef CONFIG_IPHONE_4
	clock_set_bottom_bits_38100000(1);
#endif

	return 0;
}


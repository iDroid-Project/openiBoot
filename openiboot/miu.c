#include "openiboot.h"
#include "miu.h"
#include "clock.h"
#include "hardware/power.h"
#include "util.h"
#if defined(CONFIG_IPHONE_4) || defined(CONFIG_IPAD)
#include "chipid.h"
#endif

int miu_setup() {
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	if(POWER_ID_EPOCH(*((uint8_t*)(POWER + POWER_ID))) != 3) {
#else
	if(POWER_ID_EPOCH(GET_REG(POWER + POWER_ID)) != chipid_get_power_epoch()) {
#endif
		// Epoch mismatch
		bufferPrintf("miu: epoch mismatch\r\n");
		return -1;
	}

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	clock_set_bottom_bits_38100000(1);
#endif

	return 0;
}


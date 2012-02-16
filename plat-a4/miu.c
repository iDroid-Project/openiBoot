#include "openiboot.h"
#include "miu.h"
#include "clock.h"
#include "hardware/power.h"
#include "util.h"
#include "chipid.h"

int miu_setup() {
	if(POWER_ID_EPOCH(GET_REG(POWER + POWER_ID)) != chipid_get_power_epoch()) {
		// Epoch mismatch
		bufferPrintf("miu: epoch mismatch\r\n");
		return -1;
	}

	return 0;
}


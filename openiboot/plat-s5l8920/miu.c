#include "openiboot.h"
#include "miu.h"
#include "clock.h"
#include "chipid.h"
#include "hardware/chipid.h"
#include "util.h"

int miu_setup() {
	if(GET_REG(POWERID) != chipid_get_power_epoch()) {
		// Epoch mismatch
		bufferPrintf("miu: epoch mismatch\r\n");
		return -1;
	}

	return 0;
}


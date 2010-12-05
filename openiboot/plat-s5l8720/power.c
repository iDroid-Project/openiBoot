#include "openiboot.h"
#include "power.h"
#include "hardware/power.h"
#include "clock.h"

int power_setup() {
	// Deprecated in 2.x
	SET_REG(POWER + POWER_CONFIG0, POWER_CONFIG0_RESET);
	SET_REG(POWER + POWER_CONFIG2, POWER_CONFIG2_RESET);
	SET_REG(POWER + POWER_CONFIG3, clock_get_base_frequency() / POWER_CONFIG3_RESET_DIVIDER);
	return 0;
}

int power_ctrl(uint32_t device, OnOff on_off) {
	if(on_off == ON) {
		SET_REG(POWER + POWER_ONCTRL, device);
	} else {
		SET_REG(POWER + POWER_OFFCTRL, device);
	}

	/* wait for the new state to take effect */
	while((GET_REG(POWER + POWER_SETSTATE) & device) != (GET_REG(POWER + POWER_STATE) & device));

	return 0;
}

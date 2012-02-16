#include "openiboot.h"
#include "power.h"
#include "hardware/power.h"
#include "clock.h"

int power_setup() {
	SET_REG(POWER + POWER_CONFIG0, POWER_CONFIG0_RESET);
	SET_REG(POWER + POWER_CONFIG2, POWER_CONFIG2_RESET);
	SET_REG(POWER + POWER_CONFIG3, clock_get_base_frequency() / POWER_CONFIG3_RESET_DIVIDER);
	return 0;
}

int power_ctrl(uint32_t device, OnOff on_off) {
	// only clock gates are used for the S5L8720
	return 0;
}

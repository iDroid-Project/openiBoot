#include "openiboot.h"
#include "chipid.h"
#include "hardware/chipid.h"

int chipid_spi_clocktype() {
	return GET_SPICLOCKTYPE(GET_REG(CHIPID + SPICLOCKTYPE));
}

unsigned int chipid_get_power_epoch() {
	return CHIPID_GET_POWER_EPOCH(GET_REG(CHIPID));
}

#if defined(CONFIG_IPHONE_4) || defined(CONFIG_IPAD)
unsigned int chipid_get_gpio() {
	return CHIPID_GET_GPIO(GET_REG(CHIPID));
}
#endif

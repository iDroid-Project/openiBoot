#include "openiboot.h"
#include "buttons.h"
#include "hardware/buttons.h"
#include "pmu.h"
#include "gpio.h"

int buttons_is_pushed(int which) {
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	if(gpio_pin_state(which) && pmu_get_reg(BUTTONS_IIC_STATE))
#else
	if(gpio_pin_state(which))
#endif
		return TRUE;
	else
		return FALSE;
}

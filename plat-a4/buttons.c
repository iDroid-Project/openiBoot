#include "openiboot.h"
#include "buttons.h"
#include "hardware/buttons.h"
#include "gpio.h"

int buttons_is_pushed(int which) {
	if(!gpio_pin_state(which))
		return TRUE;
	else
		return FALSE;
}

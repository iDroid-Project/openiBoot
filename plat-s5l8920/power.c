#include "openiboot.h"
#include "power.h"
#include "hardware/power.h"

// S5L8920 no longer has power_ctrl, just
// lots of clock gates. -- Ricky26

int power_setup()
{
	return 0;
}

int power_ctrl(uint32_t device, OnOff on_off)
{
	return 0;
}

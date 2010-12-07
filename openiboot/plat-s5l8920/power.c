#include "openiboot.h"
#include "power.h"
#include "hardware/power.h"

int power_setup()
{
	return 0;
}

int power_ctrl(uint32_t device, OnOff on_off) {
	if (device > POWER_STATE_MAX) {
		return -1;
	}
	uint32_t device_register = POWER + POWER_STATE + (device << 2);

	if (on_off == ON) {
		SET_REG(device_register, GET_REG(device_register) | 0xF);
	} else {
		SET_REG(device_register, GET_REG(device_register) & ~0xF);
	}
	
	/* wait for the new state to take effect */
	//while ((GET_REG(device_register) & 0xF) != ((GET_REG(device_register) >> 4) & 0xF));

	return 0;
}

void power_ctrl_many(uint64_t _gates, OnOff _val)
{
	uint32_t reg = POWER+POWER_STATE;
	int num;
	for(num = 0; num < 52; num++)
	{
		if(_gates & 1)
		{
			if(_val == ON)
				SET_REG(reg, GET_REG(reg) | 0xF);
			else
				SET_REG(reg, GET_REG(reg) &~ 0xF);
		}

		reg += sizeof(uint32_t);
		_gates >>= 1;
	}
}

#include "openiboot.h"
#include "commands.h"
#include "pmu.h"
#include "a4/pmu.h"
#include "hardware/pmu.h"
#include "hardware/usbphy.h"
#include "i2c.h"
#include "timer.h"
#include "gpio.h"
#include "util.h"

int sub_5FF085D8(int a, int b ,int c)
{
	uint8_t registers = 0x50 + a;
	uint8_t recv_buff = 0;
	uint8_t data = 0;
	int result;

	if (a > 10) return -1;
	
	result = i2c_rx(PMU_I2C_BUS, PMU_GETADDR, (void*)&registers, 1, (void*)&recv_buff, 1);
	if (result != I2CNoError) return result;
	
	recv_buff &= 0x1D;
	
	if (b == 0) {
		data = recv_buff | 0x60;
	} else {
		data = recv_buff;
		if (c != 0)
			data |= 2;
	}
	
	pmu_write_reg(registers, data, 1);
	return 0;
}

int pmu_get_reg(int reg) {
	uint8_t registers[1];
	uint8_t out[1];

	registers[0] = reg;

	i2c_rx(PMU_I2C_BUS, PMU_GETADDR, registers, 1, out, 1);
	return out[0];
}

int pmu_get_regs(int reg, uint8_t* out, int count) {
	uint8_t registers[1];

	registers[0] = reg;

	return i2c_rx(PMU_I2C_BUS, PMU_GETADDR, registers, 1, out, count);
}

int pmu_write_reg(int reg, int data, int verify) {
	uint8_t command[2];

	command[0] = reg;
	command[1] = data;

	i2c_tx(PMU_I2C_BUS, PMU_SETADDR, command, sizeof(command));

	if(!verify)
		return 0;

	uint8_t pmuReg = reg;
	uint8_t buffer = 0;
	i2c_rx(PMU_I2C_BUS, PMU_GETADDR, &pmuReg, 1, &buffer, 1);

	if(buffer == data)
		return 0;
	else
		return -1;
}

int pmu_write_regs(const PMURegisterData* regs, int num) {
	int i;
	for(i = 0; i < num; i++) {
		pmu_write_reg(regs[i].reg, regs[i].data, 1);
	}

	return 0;
}

static int query_adc(int mux) {
	uint8_t buf[2];
	uint32_t mux_masked, result;
	uint64_t timeout;

	mux_masked = mux & 0xF;
	result = pmu_get_reg(PMU_ADC_REG);
	
	if (mux == 3) {
		mux_masked |= 0x20;
		pmu_write_reg(PMU_MUXSEL_REG, mux_masked, 0);
		udelay(80000);
	}
	
	pmu_write_reg(PMU_MUXSEL_REG, mux_masked | 0x10, 0);
	
	timeout = timer_get_system_microtime() + 50000;
	while (TRUE) {
		udelay(1000);
		if (timer_get_system_microtime() > timeout)
			return -1;
			
		result = pmu_get_reg(PMU_ADC_REG);
		
		if (result & 0x20)
			break;
	}
	
	pmu_get_regs(PMU_ADCVAL_REG, buf, 2);
	pmu_write_reg(PMU_MUXSEL_REG, 0, 0);
	
	return (buf[1] << 4) | (buf[0] & 0xF);
}

static void usbphy_charger_identify(unsigned int sel) {
	if (sel > 2)
		return;
	
	clock_gate_switch(USB_CLOCKGATE_UNK1, 1);
	SET_REG(USB_PHY + OPHYUNK3, (GET_REG(USB_PHY + OPHYUNK3) & (~0x6)) | (sel*2));
	clock_gate_switch(USB_CLOCKGATE_UNK1, 0);
}

static PowerSupplyType identify_usb_charger() {
	int dn;
	int dp;

	usbphy_charger_identify(2);
	udelay(2000);
	
	dn = query_adc(6);
	if(dn < 0)
		dn = 0;

	usbphy_charger_identify(1);
	udelay(2000);
	
	dp = query_adc(6);
	if(dp < 0)
		dp = 0;
	
	usbphy_charger_identify(0);

	if(dn < 99 || dp < 99) {
		return PowerSupplyTypeUSBHost;
	}

	int x = (dn * 1000) / dp;
	
	if (x <= 0x5E1)
		return PowerSupplyTypeUSBBrick1000mA;
	else if (x < 0x460 || x > 0x307)
		return PowerSupplyTypeUSBHost;
	else
		return PowerSupplyTypeUSBBrick2000mA;
}

PowerSupplyType pmu_get_power_supply() {
	uint8_t val = pmu_get_reg(PMU_POWERSUPPLY_REG);
	
	if (val & PMU_POWERSUPPLY_USB)
		return identify_usb_charger();
	else if (val & PMU_POWERSUPPLY_FIREWIRE)
		return PowerSupplyTypeFirewire;
	else
		return PowerSupplyTypeBattery;
}

void cmd_pmu_powersupply(int argc, char** argv) {
	PowerSupplyType power = pmu_get_power_supply();
	bufferPrintf("power supply type: ");
	switch(power) {
		case PowerSupplyTypeError:
			bufferPrintf("Unknown");
			break;
			
		case PowerSupplyTypeBattery:
			bufferPrintf("Battery");
			break;
			
		case PowerSupplyTypeFirewire:
			bufferPrintf("Firewire");
			break;
			
		case PowerSupplyTypeUSBHost:
			bufferPrintf("USB host");
			break;
			
		case PowerSupplyTypeUSBBrick500mA:
			bufferPrintf("500 mA brick");
			break;
			
		case PowerSupplyTypeUSBBrick1000mA:
			bufferPrintf("1000 mA brick");
			break;
			
		case PowerSupplyTypeUSBBrick2000mA:
			bufferPrintf("2000 mA brick");
			break;
	}
	bufferPrintf("\r\n");
}
COMMAND("pmu_powersupply", "get the power supply type", cmd_pmu_powersupply);

static void pmu_init_boot()
{
	// TODO
}
MODULE_INIT_BOOT(pmu_init_boot);

static void pmu_init()
{
	// TODO
}
MODULE_INIT(pmu_init);


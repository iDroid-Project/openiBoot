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

int sub_5FF085D8(int regidx, int b ,int c)
{
	uint8_t registers = PMU_UNKREG_START + regidx;
	uint8_t recv_buff = 0;
	uint8_t data = 0;
	int result;

	if (regidx > PMU_UNKREG_END - PMU_UNKREG_START)
		return -1;
	
	result = i2c_rx(PMU_I2C_BUS, PMU_GETADDR, (void*)&registers, 1, (void*)&recv_buff, 1);
	if (result != I2CNoError)
		return result;
	
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

void pmu_write_oocshdwn(int data) {
	uint8_t poweroffData[] = {0xC0, 0xFF, 0xBF, 0xFF, 0xAE, 0xFF};
	uint8_t buffer[sizeof(poweroffData) + 1];
	
	pmu_get_reg(PMU_UNK2_REG);
	
	buffer[0] = PMU_UNK1_REG;
	memcpy(&buffer[1], poweroffData, sizeof(poweroffData));
	
	i2c_tx(PMU_I2C_BUS, PMU_SETADDR, buffer, sizeof(buffer));
	
	if (data == 1) {
		uint8_t result, reg;
		
		// sub_5FF0D99C();  // Something to do with gas gauge.
		
		for (reg = PMU_UNKREG_START; reg <= PMU_UNKREG_END; reg++) {
			result = pmu_get_reg(reg);
			
			if (!((result & 0xE0) <= 0x5F || (result & 2) == 0))
				pmu_write_reg(reg, result & 0xFD, FALSE);
		}
	}
	
	pmu_write_reg(PMU_OOCSHDWN_REG, data, FALSE);
	
	while(TRUE) {
		udelay(100000);
	}
}

void pmu_poweroff() {
	OpenIBootShutdown();
	// lcd_shutdown();
	pmu_write_oocshdwn(1);
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
	if (sel > PMU_CHARGER_IDENTIFY_MAX)
		return;
	
	clock_gate_switch(USB_CLOCKGATE_UNK1, ON);
	SET_REG(USB_PHY + OPHYUNK3, (GET_REG(USB_PHY + OPHYUNK3) & (~0x6)) | (sel*2));
	clock_gate_switch(USB_CLOCKGATE_UNK1, OFF);
}

static PowerSupplyType identify_usb_charger() {
	int dn, dp, x;

	usbphy_charger_identify(PMU_CHARGER_IDENTIFY_DN);
	udelay(2000);
	
	dn = query_adc(PMU_ADCMUX_USBCHARGER);
	if (dn < 0)
		dn = 0;

	usbphy_charger_identify(PMU_CHARGER_IDENTIFY_DP);
	udelay(2000);
	
	dp = query_adc(PMU_ADCMUX_USBCHARGER);
	if (dp < 0)
		dp = 0;
	
	usbphy_charger_identify(PMU_CHARGER_IDENTIFY_NONE);

	if (dn < 99 || dp < 99)
		return PowerSupplyTypeUSBHost;

	x = (dn * 1000) / dp;
	
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

int pmu_get_battery_voltage() {
	return (pmu_get_reg(PMU_VOLTAGE_HIGH_REG) << 8) | pmu_get_reg(PMU_VOLTAGE_LOW_REG);
}

void cmd_poweroff(int argc, char** argv) {
	pmu_poweroff();
}
COMMAND("poweroff", "power off the device", cmd_poweroff);

void cmd_pmu_voltage(int argc, char** argv) {
	bufferPrintf("battery voltage: %d mV\r\n", pmu_get_battery_voltage());
}
COMMAND("pmu_voltage", "get the battery voltage", cmd_pmu_voltage);

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

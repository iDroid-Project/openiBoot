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
#include "lcd.h"

static uint32_t GPMemCachedPresent = 0;
static uint8_t GPMemCache[PMU_MAXREG + 1];

typedef struct _pmu_ldo
{
	uint16_t base_voltage;
	uint8_t step_size;
	uint8_t step_count;
	uint8_t step_mask;
	uint8_t gate_mask;
	uint8_t reg;
	uint8_t mask;
} pmu_ldo_t;

static pmu_ldo_t pmu_ldo[] = {
	{ 2500, 50, 21, 0x1F, 1, 0x14, 4 },
	{ 1650, 5, 31, 0x1F, 0, 0x14, 8 },
	{ 2500, 50, 16, 0x1F, 0, 0x14, 0x10 },
	{ 1800, 50, 30, 0x1F, 0, 0x14, 0x20 },
	{ 2500, 50, 22, 0x1F, 2, 0x14, 0x40 },
	{ 2500, 50, 22, 0x1F, 4, 0x14, 0x80 },
	{ 1500, 100, 31, 0x1F, 0, 0x15, 1 },
	{ 2000, 50, 31, 0x1F, 0, 0x15, 2 },
	{ 1200, 25, 12, 0xF, 0, 0x15, 4 },
	{ 1700, 50, 26, 0x1F, 8, 0x15, 8 },
	{ 1700, 50, 26, 0x1F, 0, 0x15, 0x10 },
	{ 600, 25, 28, 0x1F, 0, 0x15, 0x20 },
	{ 0, 0, 0, 0, 0, 0x16, 2 },
	{ 0, 0, 0, 0, 0, 0x16, 1 },
	{ 0, 0, 0, 0, 0, 0x16, 0x10 },
	{ 0, 0, 0, 0, 0, 0x16, 0x20 },
	{ 0, 0, 0, 0, 0, 0x16, 8 },
	{ 0, 0, 0, 0, 0, 0x18, 4 },
	{ 0, 0, 0, 0, 0, 0x18, 0x80 },
};

static void pmu_init_boot()
{
	pmu_set_iboot_stage(0x20);
}
MODULE_INIT_BOOT(pmu_init_boot);

static void pmu_init()
{
	pmu_set_iboot_stage(0);
}
MODULE_INIT(pmu_init);

error_t pmu_setup_gpio(int _idx, int _dir, int _pol)
{
	uint8_t reg = PMU_GPIO + _idx;
	uint8_t val;

	if (_idx >= PMU_GPIO_COUNT)
		return EINVAL;

	val = pmu_get_reg(reg);
	val &= 0x1D;
	
	if(!_dir) // Input
	{
		val |= 0x40;
	}
	else // Output
	{
		if(_pol)
			val |= 2; // High
		else
			val &=~ 2; // Low
	}
	
	pmu_write_reg(reg, val, 1);
	return SUCCESS;
}

error_t pmu_setup_ldo(int _idx, uint16_t _v, uint8_t _enable_gates, uint8_t _enable)
{
	uint8_t val;

	if(_idx >= PMU_LDO_COUNT)
		return	EINVAL;

	if(pmu_ldo[_idx].base_voltage && !_v)
		return EINVAL;

	if(pmu_ldo[_idx].gate_mask)
	{
		val = pmu_get_reg(PMU_LDO_GATES);
		val &=~ pmu_ldo[_idx].gate_mask;
		if(_enable && _enable_gates)
			val |= pmu_ldo[_idx].gate_mask;

		CHAIN_FAIL(pmu_write_reg(PMU_LDO_GATES, val, 1));
	}

	if(pmu_ldo[_idx].base_voltage)
	{
		uint8_t steps = (_v - pmu_ldo[_idx].base_voltage)
			/ pmu_ldo[_idx].step_size;
		if(steps > pmu_ldo[_idx].step_count)
		{
			bufferPrintf("pmu: Power too great for %d: %d > %d!\n",
					_idx, steps, pmu_ldo[_idx].step_count);
			return EINVAL;
		}

		val = pmu_get_reg(PMU_LDO_V + _idx);
		val &=~ pmu_ldo[_idx].step_mask;
		val |= (steps & pmu_ldo[_idx].step_mask);
		CHAIN_FAIL(pmu_write_reg(PMU_LDO_V + _idx, val, 1));
	}

	val = pmu_get_reg(pmu_ldo[_idx].reg);
	val &=~ pmu_ldo[_idx].mask;

	if(_enable)
		val |= pmu_ldo[_idx].mask;

	if(_idx == 0x16)
	{
		// This is some haxxy dependancy code.
		// Basically, pins 4 and 5 from 0x16
		// rely on pin 3 being enabled.
		//
		// (Actually it's broken on disable,
		//  but I'll fix it when it needs fixing.)
		//  -- Ricky26
		if(!(pmu_ldo[_idx].mask & 8))
		{
			if(val & 0x30)
				val |= 8;
			else
				val &=~ 8;
		}
	}

	CHAIN_FAIL(pmu_write_reg(pmu_ldo[_idx].reg, val, 1));
	return SUCCESS;
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

	if (!verify)
		return 0;

	uint8_t pmuReg = reg;
	uint8_t buffer = 0;
	i2c_rx(PMU_I2C_BUS, PMU_GETADDR, &pmuReg, 1, &buffer, 1);

	if (buffer == data)
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
	lcd_set_backlight_level(0);
	lcd_shutdown();
	pmu_set_iboot_stage(0);
	pmu_write_oocshdwn(1);
}

void pmu_set_iboot_stage(uint8_t stage) {
	uint8_t state = 0;
	
	pmu_get_gpmem_reg(PMU_IBOOTSTATE, &state);
	
	if ((state & 0xD0) != 0x80) {
		// There was some check here, omitted for now. -Oranav
		pmu_set_gpmem_reg(PMU_IBOOTSTAGE, stage);
	}
}

int pmu_get_gpmem_reg(int reg, uint8_t* out) {
	if (reg > PMU_MAXREG)
		return -1;
	
	// Check whether the register is already cached.
	if ((GPMemCachedPresent & (0x1 << reg)) == 0) {
		uint8_t registers[1];

		registers[0] = reg ^ 0x80;

		if (i2c_rx(PMU_I2C_BUS, PMU_GETADDR, registers, 1, &GPMemCache[reg], 1) != 0) {
			return -1;
		}

		GPMemCachedPresent |= (0x1 << reg);
	}

	*out = GPMemCache[reg];

	return 0;	
}

int pmu_set_gpmem_reg(int reg, uint8_t data) {
	if (reg > PMU_MAXREG)
		return -1;
	
	// If the data isn't cached,
	// Or if the cached data differs than what we write, write it.
	if ((GPMemCachedPresent & (0x1 << reg)) == 0 || GPMemCache[reg] != data) {
		GPMemCache[reg] = data;
		GPMemCachedPresent |= (0x1 << reg);
		pmu_write_reg(reg ^ 0x80, data, FALSE);
	}

	return 0;
}

static int query_adc(int mux) {
	uint8_t buf[2];
	uint32_t mux_masked, result = 0;
	uint64_t startTime;

	mux_masked = mux & 0xF;
	result = pmu_get_reg(PMU_ADC_REG);
	
	if (mux == 3) {
		mux_masked |= 0x20;
		pmu_write_reg(PMU_MUXSEL_REG, mux_masked, 0);
		udelay(80000);
	}
	
	pmu_write_reg(PMU_MUXSEL_REG, mux_masked | 0x10, 0);
	
	startTime = timer_get_system_microtime();
	do {
		udelay(1000);
		if (has_elapsed(startTime, 50000))
			return -1;
			
		result = pmu_get_reg(PMU_ADC_REG);
	} while (!(result & 0x20));
	
	pmu_get_regs(PMU_ADCVAL_REG, buf, 2);
	pmu_write_reg(PMU_MUXSEL_REG, 0, 0);
	
	return (buf[1] << 4) | (buf[0] & 0xF);
}

static void usbphy_charger_identify(uint8_t sel) {
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

static error_t cmd_poweroff(int argc, char** argv)
{
	pmu_poweroff();

	return SUCCESS;
}
COMMAND("poweroff", "power off the device", cmd_poweroff);

static error_t cmd_pmu_voltage(int argc, char** argv)
{
	bufferPrintf("battery voltage: %d mV\r\n", pmu_get_battery_voltage());

	return SUCCESS;
}
COMMAND("pmu_voltage", "get the battery voltage", cmd_pmu_voltage);

static error_t cmd_pmu_powersupply(int argc, char** argv)
{
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

	return SUCCESS;
}
COMMAND("pmu_powersupply", "get the power supply type", cmd_pmu_powersupply);

static error_t cmd_pmu_nvram(int argc, char** argv)
{
	uint8_t reg;

	pmu_get_gpmem_reg(PMU_IBOOTSTATE, &reg);
	bufferPrintf("0: [iBootState] %02x\r\n", reg);
	pmu_get_gpmem_reg(PMU_IBOOTDEBUG, &reg);
	bufferPrintf("1: [iBootDebug] %02x\r\n", reg);
	pmu_get_gpmem_reg(PMU_IBOOTSTAGE, &reg);
	bufferPrintf("2: [iBootStage] %02x\r\n", reg);
	pmu_get_gpmem_reg(PMU_IBOOTERRORCOUNT, &reg);
	bufferPrintf("3: [iBootErrorCount] %02x\r\n", reg);
	pmu_get_gpmem_reg(PMU_IBOOTERRORSTAGE, &reg);
	bufferPrintf("4: [iBootErrorStage] %02x\r\n", reg);

	return SUCCESS;
}
COMMAND("pmu_nvram", "list powernvram registers", cmd_pmu_nvram);

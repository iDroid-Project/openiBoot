#include "openiboot.h"
#include "arm/arm.h"
#include "commands.h"
#include "pmu.h"
#include "hardware/pmu.h"
#include "hardware/radio.h"
#include "i2c.h"
#include "timer.h"
#include "gpio.h"
#include "lcd.h"
#include "util.h"
#include "usbphy.h"

static uint32_t GPMemCachedPresent = 0;
static uint8_t GPMemCache[PMU_MAXREG + 1];

static void pmu_init_boot()
{
	//pmu_charge_settings(TRUE, FALSE, FALSE);
}
MODULE_INIT_BOOT(pmu_init_boot);

static void pmu_init()
{
	pmu_set_iboot_stage(0);
}
MODULE_INIT(pmu_init);

void pmu_write_oocshdwn(int data) {
	uint8_t registers[1];
	uint8_t discardData[5];
	uint8_t poweroffData[] = {7, 0xD1, 0xFF, 0xF0};
	registers[0] = 1;
	i2c_rx(PMU_I2C_BUS, PMU_GETADDR, registers, sizeof(registers), discardData, 3);

	i2c_tx(PMU_I2C_BUS, PMU_SETADDR, poweroffData, sizeof(poweroffData));
	pmu_write_reg(PMU_OOCSHDWN, data, FALSE);

	// Wait for the hardware to shut down
	EnterCriticalSection();

	while(TRUE) {
		udelay(100000);
	}
}

void pmu_poweroff() {
	OpenIBootShutdown();

	pmu_write_oocshdwn(PMU_OOCSHDWN_GOSTBY);

	lcd_shutdown();
}

void pmu_set_iboot_stage(uint8_t stage) {
	int8_t state;
	pmu_get_gpmem_reg(PMU_IBOOTSTATE, (uint8_t*) &state);

	if(state >= 0) {
		pmu_set_gpmem_reg(PMU_IBOOTSTAGE, stage);
	}
}

int pmu_get_gpmem_reg(int reg, uint8_t* out) {
	if(reg > PMU_MAXREG)
		return -1;

	if((GPMemCachedPresent & (0x1 << reg)) == 0) {
		uint8_t registers[1];

		registers[0] = reg + 0x67;

		if(i2c_rx(PMU_I2C_BUS, PMU_GETADDR, registers, 1, &GPMemCache[reg], 1) != 0) {
			return -1;
		}

		GPMemCachedPresent |= (0x1 << reg);
	}

	*out = GPMemCache[reg];

	return 0;	
}

int pmu_set_gpmem_reg(int reg, uint8_t data) {
	if(pmu_write_reg(reg + 0x67, data, TRUE) == 0) {
		GPMemCache[reg] = data;
		GPMemCachedPresent |= (0x1 << reg);
		return 0;
	}

	return -1;
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

int query_adc(int flags, uint32_t* result) {
	// clear the done bit if it is set
	pmu_get_reg(PMU_ADCSTS);
	
	// set up flags
	if (flags == 0x3) {
		pmu_write_reg(PMU_ADCCON, flags | 0x20, FALSE);
		udelay(80000);
	}
	pmu_write_reg(PMU_ADCCON, flags | 0x10, FALSE);
	
	// wait until done
	uint64_t startTime = timer_get_system_microtime();
	while (!(pmu_get_reg(PMU_ADCSTS) & 0x2)) {
		if (has_elapsed(startTime, 40000)) {
			return -1;
		}
	}

	uint8_t out[2];
	pmu_get_regs(PMU_ADCOUT1, out, 2);
	*result = (out[1] << 2) | (out[0] & 0x3);
	return 0;
}

int pmu_get_battery_voltage() {
	return (pmu_get_reg(PMU_VOLTAGE1) << 8) | pmu_get_reg(PMU_VOLTAGE2);
}

static int bcd_to_int(int bcd) {
	//TODO: implement
/*
	return (bcd & 0xF) + (((bcd >> 4) & 0xF) * 10);
*/
	return 0;
}

const char* get_dayofweek_str(int day) {
	switch(day) {
		case 0:
			return "Sunday";
		case 1:
			return "Monday";
		case 2:
			return "Tuesday";
		case 3:
			return "Wednesday";
		case 4:
			return "Thursday";
		case 5:
			return "Friday";
		case 6:
			return "Saturday";
	}

	return NULL;
}

static const int days_in_months_leap_year[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
static const int days_in_months[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
uint64_t pmu_get_epoch()
{
	//TODO: implement
	return 0;
	if (0) {
		//TODO: remove me (make stuff compile)
		bcd_to_int(1);
	}

/*
	int i;
	int days;
	int years;
	uint64_t secs;
	int32_t offset;
	uint8_t rtc_data[PMU_RTCYR - PMU_RTCSC + 1];
	uint8_t rtc_data2[PMU_RTCYR - PMU_RTCSC + 1];

	do
	{
		pmu_get_regs(PMU_RTCSC, rtc_data, PMU_RTCYR - PMU_RTCSC + 1);
		pmu_get_regs(PMU_RTCSC, rtc_data2, PMU_RTCYR - PMU_RTCSC + 1);
	} while(rtc_data2[0] != rtc_data[0]);

	secs = bcd_to_int(rtc_data[0] & PMU_RTCSC_MASK);

	secs += 60 * bcd_to_int(rtc_data[PMU_RTCMN - PMU_RTCSC] & PMU_RTCMN_MASK);
	secs += 3600 * bcd_to_int(rtc_data[PMU_RTCHR - PMU_RTCSC] & PMU_RTCHR_MASK);

	days = bcd_to_int(rtc_data[PMU_RTCDT - PMU_RTCSC] & PMU_RTCDT_MASK) - 1;

	years = 2000 + bcd_to_int(rtc_data[PMU_RTCYR - PMU_RTCSC] & PMU_RTCYR_MASK);
	for(i = 1970; i < years; ++i)
	{
		if((i & 0x3) != 0)
			days += 365;	// non-leap year
		else
			days += 366;	// leap year
	}
	
	if((years & 0x3) != 0)
		days += days_in_months[(rtc_data[PMU_RTCMT - PMU_RTCSC] & PMU_RTCMT_MASK) - 1];
	else
		days += days_in_months_leap_year[(rtc_data[PMU_RTCMT - PMU_RTCSC] & PMU_RTCMT_MASK) - 1];

	secs += ((uint64_t)days) * 86400;

	pmu_get_regs(0x6B, (uint8_t*) &offset, sizeof(offset));

	secs += offset;
	return secs;
*/
}

void epoch_to_date(uint64_t epoch, int* year, int* month, int* day, int* day_of_week, int* hour, int* minute, int* second)
{
	int i;
	int dec = 0;
	int days_since_1970 = 0;
	const int* months_to_days;

	i = 1970;	
	while(epoch >= dec)
	{
		epoch -= dec;
		if((i & 0x3) != 0)
		{
			dec = 365 * 86400;
			if(epoch >= dec) days_since_1970 += 365;
		} else
		{
			dec = 366 * 86400;
			if(epoch >= dec) days_since_1970 += 366;
		}
		++i;
	}

	*year = i - 1;

	if(((i - 1) & 0x3) != 0)
		months_to_days = days_in_months;
	else
		months_to_days = days_in_months_leap_year;

	for(i = 0; i < 12; ++i)
	{
		dec = months_to_days[i] * 86400;
		if(epoch < dec)
		{
			days_since_1970 += months_to_days[i - 1];
			epoch -= months_to_days[i - 1] * 86400;
			*month = i;
			break;
		}
	}

	for(i = 0; i < 31; ++i)
	{
		if(epoch < 86400)
		{
			*day = i + 1;
			break;
		}
		epoch -= 86400;
	}

	days_since_1970 += i;

	*day_of_week = (days_since_1970 + 4) % 7;

	for(i = 0; i < 24; ++i)
	{
		if(epoch < 3600)
		{
			*hour = i;
			break;
		}
		epoch -= 3600;
	}

	for(i = 0; i < 60; ++i)
	{
		if(epoch < 60)
		{
			*minute = i;
			break;
		}
		epoch -= 60;
	}

	*second = epoch;
}

void pmu_date(int* year, int* month, int* day, int* day_of_week, int* hour, int* minute, int* second)
{
	epoch_to_date(pmu_get_epoch(), year, month, day, day_of_week, hour, minute, second);
}

static PowerSupplyType identify_usb_charger() {
	uint32_t dn;
	uint32_t dp;

	usb_set_charger_identification_mode(USBChargerIdentificationDN);
	udelay(2000);
	if (query_adc(6, &dn) != 0) {
		dn = 0;
	}

	usb_set_charger_identification_mode(USBChargerIdentificationDP);
	udelay(2000);
	if (query_adc(6, &dp) != 0) {
		dp = 0;
	}

	usb_set_charger_identification_mode(USBChargerIdentificationNone);
	if (dn < 0xCC || dp < 0xCC) {
		return PowerSupplyTypeUSBHost;
	}

	int x = (dn * 1000) / dp;
	if((x - 1291) <= 214) {
		return PowerSupplyTypeUSBBrick1000mA;
	}

	if((x - 901) <= 219 && dp <= 479 ) {
		return PowerSupplyTypeUSBBrick500mA;
	} else {
		return PowerSupplyTypeUSBHost;
	}
}

PowerSupplyType pmu_get_power_supply() {
	if (pmu_get_reg(0x4) & 1<<3) {
		// USB powered
		return identify_usb_charger();
	} else if (pmu_get_reg(0x6) & 1<<3) {
		// Firewire powered
		return PowerSupplyTypeFirewire;
	} else {
		// Battery powered
		return PowerSupplyTypeBattery;
	}
}

void pmu_charge_settings(int UseUSB, int SuspendUSB, int StopCharger) {
	//TODO implement: this is all done via i2c ont the 2g touch instead of GPIO... needs a full rewrite

/*
	PowerSupplyType type = pmu_get_power_supply();

	if(type != PowerSupplyTypeUSBHost)	// No need to suspend USB, since we're not plugged into a USB host
		SuspendUSB = FALSE;

	if(SuspendUSB)
		gpio_pin_output(PMU_GPIO_CHARGER_USB_SUSPEND, 1);
	else
		gpio_pin_output(PMU_GPIO_CHARGER_USB_SUSPEND, 0);

	if(StopCharger) {
		gpio_pin_output(PMU_GPIO_CHARGER_SUSPEND, 1);
		gpio_pin_output(PMU_GPIO_CHARGER_SHUTDOWN, 1);
	} else {
		gpio_pin_output(PMU_GPIO_CHARGER_SUSPEND, 0);
		gpio_pin_output(PMU_GPIO_CHARGER_SHUTDOWN, 0);
	}

	if(type == PowerSupplyTypeUSBBrick500mA || type == PowerSupplyTypeUSBBrick1000mA || (type == PowerSupplyTypeUSBHost && UseUSB))
		gpio_pin_output(PMU_GPIO_CHARGER_USB_500_1000, 1);
	else
		gpio_pin_output(PMU_GPIO_CHARGER_USB_500_1000, 0);

	if(type == PowerSupplyTypeUSBBrick1000mA)
		gpio_pin_output(PMU_GPIO_CHARGER_USB_1000, 1);
	else
		gpio_pin_output(PMU_GPIO_CHARGER_USB_1000, 0);
*/
}

int pmu_gpio(int gpio, int is_output, int value)
{
	//TODO: implement
	return 0;
/*
	int mask;

	if(gpio < 1 || gpio > 3)
		return -1;

	mask = 1 << (gpio - 1);

	pmu_write_reg(PMU_GPIO1CFG + (gpio - 1), (value == 0) ? 0 : 0x7, 0);

	if(is_output)
		pmu_write_reg(PMU_GPIOCTL, pmu_get_reg(PMU_GPIOCTL) & ~mask, 0);
	else
		pmu_write_reg(PMU_GPIOCTL, pmu_get_reg(PMU_GPIOCTL) | mask, 0);

	return 0;
*/
}

void cmd_time(int argc, char** argv) {
	int day;
	int month;
	int year;
	int hour;
	int minute;
	int second;
	int day_of_week;
	pmu_date(&year, &month, &day, &day_of_week, &hour, &minute, &second);
	bufferPrintf("Current time: %02d:%02d:%02d, %s %02d/%02d/%02d GMT\r\n", hour, minute, second, get_dayofweek_str(day_of_week), month, day, year);
	//bufferPrintf("Current time: %02d:%02d:%02d, %s %02d/%02d/20%02d\r\n", pmu_get_hours(), pmu_get_minutes(), pmu_get_seconds(), pmu_get_dayofweek_str(), pmu_get_month(), pmu_get_day(), pmu_get_year());
	//bufferPrintf("Current time: %llu\n", pmu_get_epoch());
}
COMMAND("time", "display the current time according to the RTC", cmd_time);

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

void cmd_pmu_charge(int argc, char** argv) {
	if(argc < 2) {
		bufferPrintf("Usage: %s <on|off>\r\n", argv[0]);
		return;
	}

	if(strcmp(argv[1], "on") == 0) {
		pmu_charge_settings(TRUE, FALSE, FALSE);
		bufferPrintf("Charger on\r\n");
	} else if(strcmp(argv[1], "off") == 0) {
		pmu_charge_settings(FALSE, FALSE, TRUE);
		bufferPrintf("Charger off\r\n");
	} else {
		bufferPrintf("Usage: %s <on|off>\r\n", argv[0]);
		return;
	}
}
COMMAND("pmu_charge", "turn on and off the power charger", cmd_pmu_charge);

void cmd_pmu_nvram(int argc, char** argv) {
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
}
COMMAND("pmu_nvram", "list powernvram registers", cmd_pmu_nvram);

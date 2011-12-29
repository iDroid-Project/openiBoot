#include "openiboot.h"
#include "commands.h"
#include "clock.h"
#include "vibrator.h"
#include "timer.h"
#include "hardware/timer.h"
#include "util.h"

void vibrator_loop(int frequency, int period, int time)
{
	// The second option represents the duty cycle of the PWM that drives the vibrator.
	if(frequency > 0)
	{
		int prescaler = 1;
		uint32_t count = TicksPerSec / frequency;

		while(count > 0xFFFF)
		{
			count >>= 1;
			prescaler <<= 1;
		}

		uint32_t countOn = (count * (100 - period))/100;

		timer_init(VibratorTimer, countOn, count, prescaler - 1, 0, FALSE, FALSE, FALSE, FALSE, FALSE);
		timer_on_off(VibratorTimer, ON);
	}

	udelay(time);

	if(frequency > 0)
		vibrator_off();
}

void vibrator_once(int time) {
	if (time == 0)
		return;
	uint32_t count = time * (TicksPerSec/1000000);
	while(1) {
		uint32_t countRun = count;
		if (count > TicksPerSec)
			countRun = TicksPerSec;	
		int prescaler = 1;
		while(countRun > 0xFFFF)
		{
			countRun >>= 1;
			prescaler <<= 1;
		}

		timer_init(VibratorTimer, 0, countRun, prescaler - 1, 0, FALSE, FALSE, FALSE, TRUE, FALSE);
		timer_on_off(VibratorTimer, ON);
		if (TicksPerSec > count)
			return;
		count -= TicksPerSec;
		udelay(1000000);
	}
}

void vibrator_off() {
	// turn the vibrator on, and invert tout to set the vibration off
	timer_init(VibratorTimer, 0, 1, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
	timer_on_off(VibratorTimer, ON);
	timer_init(VibratorTimer, 0, 1, 0, 0, FALSE, FALSE, FALSE, FALSE, FALSE);
}

static int cmd_vibrator_loop(int argc, char** argv)
{
	if(argc < 3) {
		bufferPrintf("Usage: %s <frequency 1-12000000> <duty time in percent> <time vibrator on in ms>\r\n", argv[0]);
		return -1;
	}

	int frequency = parseNumber(argv[1]);
	int period = parseNumber(argv[2]);
	int time = parseNumber(argv[3]) * 1000;

	bufferPrintf("Turning on vibrator at frequency %d with %d percent duty time for %d microseconds.\r\n", frequency, period, time);

	vibrator_loop(frequency, period, time);

	return 0;
}
COMMAND("vibrator_loop", "turn the vibrator on in a loop", cmd_vibrator_loop);

static int cmd_vibrator_once(int argc, char** argv)
{
	if(argc < 2) {
		bufferPrintf("Usage: %s <duration in ms>\r\n", argv[0]);
		return -1;
	}

	int time = parseNumber(argv[1]) * 1000;

	bufferPrintf("Turning on vibrator for %d microseconds.\r\n", time);

	vibrator_once(time);

	return 0;
}
COMMAND("vibrator_once", "vibrate once", cmd_vibrator_once);

static int cmd_vibrator_off(int argc, char** argv)
{
	bufferPrintf("Turning off vibrator.\r\n");

	vibrator_off();

	return 0;
}
COMMAND("vibrator_off", "turn the vibrator off", cmd_vibrator_off);

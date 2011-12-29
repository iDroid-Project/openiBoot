#include "openiboot.h"
#include "commands.h"
#include "clock.h"
#include "vibrator.h"
#include "util.h"
#include "radio.h"

void vibrator_loop(int frequency, int period, int timeOn)
{
	char buf[100];
	sprintf(buf, "at+xdrv=4,0,2,%d,%d,%d\r\n", frequency, period, timeOn);

	// write the command
	radio_write(buf);

	// clear the response
	radio_read(buf, sizeof(buf));
}

void vibrator_once(int frequency, int time)
{
	char buf[100];
	sprintf(buf, "at+xdrv=4,0,1,%d,%d,%d\r\n", frequency, time + 1, time);

	// write the command
	radio_write(buf);

	// clear the response
	radio_read(buf, sizeof(buf));
}

void vibrator_off()
{
	char buf[100];

	// write the command
	radio_write("at+xdrv=4,0,0,0,0,0\r\n");

	// clear the response
	radio_read(buf, sizeof(buf));
}

static int cmd_vibrator_loop(int argc, char** argv)
{
	if(argc < 4) {
		bufferPrintf("Usage: %s <frequency 1-12> <period in ms> <time vibrator on during cycle in ms>\r\n", argv[0]);
		return -1;
	}

	int frequency = parseNumber(argv[1]);
	int period = parseNumber(argv[2]);
	int timeOn = parseNumber(argv[3]);

	bufferPrintf("Turning on vibrator at frequency %d in a %d ms cycle with %d duty time.\r\n", frequency, period, timeOn);

	vibrator_loop(frequency, period, timeOn);

	return 0;
}
COMMAND("vibrator_loop", "turn the vibrator on in a loop", cmd_vibrator_loop);

static int cmd_vibrator_once(int argc, char** argv)
{
	if(argc < 3) {
		bufferPrintf("Usage: %s <frequency 1-12> <duration in ms>\r\n", argv[0]);
		return -1;
	}

	int frequency = parseNumber(argv[1]);
	int time = parseNumber(argv[2]);

	bufferPrintf("Turning on vibrator at frequency %d for %d ms.\r\n", frequency, time);

	vibrator_once(frequency, time);

	return 0;
}
COMMAND("vibrator_once", "vibrate once", cmd_vibrator_once);

static int cmd_vibrator_off(int argc, char** argv)
{
	bufferPrintf("Turning off vibrator.\r\n");

	vibrator_off();

	return -1;
}
COMMAND("vibrator_off", "turn the vibrator off", cmd_vibrator_off);

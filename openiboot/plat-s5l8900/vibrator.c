#include <openiboot.h>
#include <clock.h>
#include <vibrator.h>
#ifdef CONFIG_IPHONE_2G
#include <util.h>
#include <radio.h>
#endif
#ifdef CONFIG_IPHONE_3G
#include <timer.h>
#include <hardware/timer.h>
#endif

#ifdef CONFIG_IPHONE_2G
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
#endif

#ifdef CONFIG_IPHONE_3G
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
#endif

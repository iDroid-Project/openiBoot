#ifndef VIBRATOR_H

#ifdef CONFIG_IPHONE
void vibrator_loop(int frequency, int period, int time);
void vibrator_once(int frequency, int time);
void vibrator_off();
#endif
#ifdef CONFIG_3G
void vibrator_loop(int frequency, int period, int time);
void vibrator_once(int time);
void vibrator_off();
#endif

#endif

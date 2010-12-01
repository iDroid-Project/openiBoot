#ifndef CHIPID_H
#define CHIPID_H

#include "openiboot.h"

int chipid_spi_clocktype();

#if defined(CONFIG_IPHONE_4) || defined(CONFIG_IPAD)
unsigned int chipid_get_power_epoch();
unsigned int chipid_get_gpio();
#endif

#endif

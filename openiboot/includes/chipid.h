#ifndef CHIPID_H
#define CHIPID_H

#include "openiboot.h"

int chipid_spi_clocktype();

#ifdef CONFIG_IPHONE_4
unsigned int chipid_get_power_epoch();
#endif

#endif

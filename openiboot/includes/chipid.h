#ifndef CHIPID_H
#define CHIPID_H

#include "openiboot.h"

int chipid_spi_clocktype();
unsigned int chipid_get_power_epoch();
unsigned int chipid_get_security_epoch();
unsigned int chipid_get_gpio_epoch();
unsigned int chipid_get_nand_epoch();

#endif

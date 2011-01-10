#ifndef  HW_H2FMI_H
#define  HW_H2FMI_H

#define H2FMI0_BASE	0x81200000
#define H2FMI1_BASE	0x81300000

#define H2FMI0_CLOCK_GATE	0x26
#define H2FMI1_CLOCK_GATE	0x27

#define H2FMI_CHIP_COUNT	16
#define H2FMI_CHIPID_LENGTH	8

#define H2FMI_REG(x, y)		(((x)->base_address)+(y))
#define H2FMI_UNKREG1(x)	(H2FMI_REG(x, 0x408))
#define H2FMI_UNKREG2(x)	(H2FMI_REG(x, 0x400))
#define H2FMI_UNKREG3(x)	(H2FMI_REG(x, 0x814))
#define H2FMI_UNKREG4(x)	(H2FMI_REG(x, 0x414))
#define H2FMI_UNKREG5(x)	(H2FMI_REG(x, 0x410))
#define H2FMI_UNKREG6(x)	(H2FMI_REG(x, 0x444))
#define H2FMI_CHIP_MASK(x)	(H2FMI_REG(x, 0x40c))
#define H2FMI_UNKREG8(x)	(H2FMI_REG(x, 0x424))
#define H2FMI_DATA(x)		(H2FMI_REG(x, 0x448))
#define H2FMI_UNKREG9(x)	(H2FMI_REG(x, 0x418))
#define H2FMI_UNKREG10(x)	(H2FMI_REG(x, 0x420))

#endif //HW_H2FMI_H

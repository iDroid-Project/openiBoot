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
#define H2FMI_UNK440(x)		(H2FMI_REG(x, 0x440))
#define H2FMI_UNKREG6(x)	(H2FMI_REG(x, 0x444))
#define H2FMI_CHIP_MASK(x)	(H2FMI_REG(x, 0x40c))
#define H2FMI_UNKREG8(x)	(H2FMI_REG(x, 0x424))
#define H2FMI_DATA(x)		(H2FMI_REG(x, 0x448))
#define H2FMI_UNK44C(x)		(H2FMI_REG(x, 0x44C))
#define H2FMI_UNKREG9(x)	(H2FMI_REG(x, 0x418))
#define H2FMI_UNKREG10(x)	(H2FMI_REG(x, 0x420))
#define H2FMI_UNK80C(x)		(H2FMI_REG(x, 0x80C))
#define H2FMI_UNK810(x)		(H2FMI_REG(x, 0x810))
#define H2FMI_UNK10(x)		(H2FMI_REG(x, 0x10))
#define H2FMI_UNK14(x)		(H2FMI_REG(x, 0x14))
#define H2FMI_UNK18(x)		(H2FMI_REG(x, 0x18))
#define H2FMI_UNKC(x)		(H2FMI_REG(x, 0xC))
#define H2FMI_UNK8(x)		(H2FMI_REG(x, 0x8))
#define H2FMI_UNK4(x)		(H2FMI_REG(x, 0x4))
#define H2FMI_PAGE_SIZE(x)	(H2FMI_REG(x, 0x0))
#define H2FMI_ECC_BITS(x)	(H2FMI_REG(x, 0x808))
#define H2FMI_UNK41C(x)		(H2FMI_REG(x, 0x41C))

#endif //HW_H2FMI_H

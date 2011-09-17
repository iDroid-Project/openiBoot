#ifndef HW_ACCEL_H
#define HW_ACCEL_H

// See http://www.st.com/stonline/products/literature/ds/15094/lis331dlh.pdf

#define ACCEL_I2C_BUS 2

#define ACCEL_SETADDR	0x32
#define ACCEL_GETADDR	0x33

#define ACCEL_WHOAMI	0x0F
#define ACCEL_CTRL_REG1	0x20
#define ACCEL_CTRL_REG2	0x21
#define ACCEL_STATUS	0x27
#define ACCEL_OUTX	0x29
#define ACCEL_OUTY	0x2B
#define ACCEL_OUTZ	0x2D

#define ACCEL_WHOAMI_VALUE	0x32

#define ACCEL_CTRL_REG1_PM2	(1 << 7)
#define ACCEL_CTRL_REG1_PM1	(1 << 6)
#define ACCEL_CTRL_REG1_PM0	(1 << 5)
#define ACCEL_CTRL_REG1_DR1	(1 << 4)
#define ACCEL_CTRL_REG1_DR0	(1 << 3)
#define ACCEL_CTRL_REG1_ZEN	(1 << 2)
#define ACCEL_CTRL_REG1_YEN	(1 << 1)
#define ACCEL_CTRL_REG1_XEN	(1 << 0)

#define ACCEL_CTRL_REG2_BOOT	(1 << 7)
#define ACCEL_CTRL_REG2_HPM1	(1 << 6)
#define ACCEL_CTRL_REG2_HPM0	(1 << 5)
#define ACCEL_CTRL_REG2_FDS	(1 << 4)
#define ACCEL_CTRL_REG2_HPEN2	(1 << 3)
#define ACCEL_CTRL_REG2_HPEN1	(1 << 2)
#define ACCEL_CTRL_REG2_HPCF1	(1 << 1)
#define ACCEL_CTRL_REG2_HPCF0	(1 << 0)

#endif

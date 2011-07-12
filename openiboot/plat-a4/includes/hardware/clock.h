#ifndef HW_CLOCKS5L8730_H
#define HW_CLOCKS5L8730_H

// Clock constants for the S5L8730. This device does not have a distinction
// between clock0 and clock1 like the S5L8900 does.

#define NUM_PLL 4

#define PMGR0_BASE			(0xBF100000)

#define CLOCK_BASE_HZ		48000000

#define CLOCK_APLL_CON0 	(PMGR0_BASE + 0x00)
#define CLOCK_APLL_CON1 	(PMGR0_BASE + 0x04)
#define CLOCK_MPLL_CON0 	(PMGR0_BASE + 0x08)
#define CLOCK_MPLL_CON1 	(PMGR0_BASE + 0x0c)
#define CLOCK_EPLL_CON0 	(PMGR0_BASE + 0x10)
#define CLOCK_EPLL_CON1 	(PMGR0_BASE + 0x14)
#define CLOCK_VPLL_CON0 	(PMGR0_BASE + 0x20)
#define CLOCK_VPLL_CON1		(PMGR0_BASE + 0x24)

#define CLOCK_CON0			(PMGR0_BASE + 0x30)
#define CLOCK_CON1			(PMGR0_BASE + 0x5030)

#define CLOCK_CON1_UNSTABLE	(1 << 16)

#define PLLCON0_ENABLE		(1 << 31)
#define PLLCON0_LOCKED		(1 << 29)
#define PLLCON0_UPDATE		(1 << 27)
#define PLLCON0_M_SHIFT		3
#define PLLCON0_M_MASK		0x3FF
#define PLLCON0_M(x)		(((x) >> PLLCON0_M_SHIFT) & PLLCON0_M_MASK)
#define PLLCON0_P_SHIFT		14
#define PLLCON0_P_MASK		0x3F
#define PLLCON0_P(x)		(((x) >> PLLCON0_P_SHIFT) & PLLCON0_P_MASK)
#define PLLCON0_S_SHIFT		0
#define PLLCON0_S_MASK		0x7
#define PLLCON0_S(x)		(((x) >> PLLCON0_S_SHIFT) & PLLCON0_S_MASK)

#define PLLCON1_FSEL_SHIFT	17
#define PLLCON1_FSEL_0		0x5
#define PLLCON1_FSEL_1		0xD
#define PLLCON1_FSEL_2		0x1C

#define PLLCON1_ENABLE		0x960

#define CLOCK_PERF_BASE		((reg32_t*)(PMGR0_BASE + 0x50c0))
#define CLOCK_NUM_PERF		3

#define CLOCK_CON_BASE		((reg32_t*)(PMGR0_BASE + 0x40))
#define CLOCK_NUM_CON		48

#define CLOCK_CON_SRC_SHIFT	28
#define CLOCK_CON_SRC_MASK	0x3
#define CLOCK_CON_SRC(x)	(((x) >> CLOCK_CON_SRC_SHIFT) & CLOCK_CON_SRC_MASK)
#define CLOCK_CON_DIV(x, y)	({ const clock_divider_t *div = &clock_dividers[(y)]; (((x) & div->mask) >> div->shift); })

#define CLOCK_CPUFREQ	(&CLOCK_CON_BASE[0])

#define CLOCK_GATE_BASE	(PMGR0_BASE + 0x1010)
#define CLOCK_GATE_MAX	(0x3F)

#endif

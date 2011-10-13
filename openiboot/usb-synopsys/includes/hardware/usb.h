#ifndef HW_USB_H
#define HW_USB_H

// Maximums supported by OIB
#define USB_NUM_ENDPOINTS	6
#define USB_NUM_FIFOS		15

#if defined(CONFIG_A4)||defined(CONFIG_S5L8920)
#define USB_PHY_3G
#elif defined(CONFIG_IPOD_TOUCH_2G)
#define USB_PHY_2G
#else
#define USB_PHY_1G
#endif

// Hardware configuration
#if defined(USB_PHY_3G)
#define USB 0x86100000
#else
#define USB 0x38400000
#endif

#if defined(USB_PHY_3G)
#ifdef CONFIG_S5L8920
#	define USB_OTGCLOCKGATE 8
#	define USB_PHYCLOCKGATE 8
#	define USB_INTERRUPT 0xE
#else
#	define USB_OTGCLOCKGATE 0x18
#	define USB_PHYCLOCKGATE 0x1D
#	define USB_INTERRUPT 0xD
#endif

#define USB_TURNAROUND 0x5
#elif defined(USB_PHY_2G)
#define USB_OTGCLOCKGATE 0x18
#define USB_PHYCLOCKGATE 0x19
#define USB_INTERRUPT 0x13
#define USB_TURNAROUND 0xB
#else
#define USB_OTGCLOCKGATE 0x2
#define USB_PHYCLOCKGATE 0x23
#define USB_INTERRUPT 0x13
#define USB_TURNAROUND 0x5
#endif

#if defined(USB_PHY_3G) || defined(USB_PHY_2G)
#define RX_FIFO_DEPTH				0x11B
#define TX_FIFO_DEPTH				0x100
#define TX_FIFO_STARTADDR			0x11B
#define PERIODIC_TX_FIFO_STARTADDR	0x21B
#define PERIODIC_TX_FIFO_DEPTH		0x100
#else
#define RX_FIFO_DEPTH				0x1C0
#define TX_FIFO_DEPTH				0x1C0
#define TX_FIFO_STARTADDR			0x200
#define PERIODIC_TX_FIFO_STARTADDR	0x21B
#define PERIODIC_TX_FIFO_DEPTH		0x100
#endif

// Registers
#define GOTGCTL		0x0
#define GOTGINT		0x4
#define GAHBCFG		0x8
#define GUSBCFG		0xC
#define GRSTCTL		0x10
#define GINTSTS		0x14
#define GINTMSK		0x18
#define GRXFSIZ		0x24
#define GNPTXFSIZ	0x28
#define GNPTXFSTS	0x2C
#define GHWCFG1		0x44
#define GHWCFG2		0x48
#define GHWCFG3		0x4C
#define GHWCFG4		0x50
#define DIEPTXF(x)	(0x100 + (4*(x)))
#define DCFG		0x800
#define DCTL		0x804
#define DSTS		0x808
#define DIEPMSK		0x810
#define DOEPMSK		0x814
#define DAINT		0x818
#define DAINTMSK	0x81C
#define DTKNQR1		0x820
#define DTKNQR2		0x824
#define DTKNQR3		0x830
#define DTKNQR4		0x834
#define USB_INREGS	0x900
#define USB_OUTREGS	0xB00

#define PCGCCTL     0xE00

#define PCGCCTL_ONOFF_MASK  3   // bits 0, 1
#define PCGCCTL_ON          0
#define PCGCCTL_OFF         1

#define GOTGCTL_BSESSIONVALID (1 << 19)
#define GOTGCTL_SESSIONREQUEST (1 << 1)

#define GAHBCFG_DMAEN (1 << 5)
#define GAHBCFG_BSTLEN_SINGLE (0 << 1)
#define GAHBCFG_BSTLEN_INCR (1 << 1)
#define GAHBCFG_BSTLEN_INCR4 (3 << 1)
#define GAHBCFG_BSTLEN_INCR8 (5 << 1)
#define GAHBCFG_BSTLEN_INCR16 (7 << 1)
#define GAHBCFG_MASKINT 0x1

#define GUSBCFG_TURNAROUND_MASK 0xF
#define GUSBCFG_TURNAROUND_SHIFT 10
#define GUSBCFG_HNPENABLE (1 << 9)
#define GUSBCFG_SRPENABLE (1 << 8)
#define GUSBCFG_PHYIF16BIT (1 << 3)
#define USB_UNKNOWNREG1_START 0x1708

#define GHWCFG2_TKNDEPTH_SHIFT	26
#define GHWCFG2_TKNDEPTH_MASK	0xF
#define GHWCFG2_NUM_ENDPOINTS_SHIFT	10
#define GHWCFG2_NUM_ENDPOINTS_MASK	0xf

#define GHWCFG4_DED_FIFO_EN			(1 << 25)

#define GRSTCTL_AHBIDLE			(1 << 31)
#define GRSTCTL_TXFFLUSH		(1 << 5)
#define GRSTCTL_TXFFNUM_SHIFT	6
#define GRSTCTL_TXFFNUM_MASK	0x1f
#define GRSTCTL_CORESOFTRESET	0x1
#define GRSTCTL_TKNFLUSH		3

#define GINTMSK_NONE        0x0
#define GINTMSK_OTG         (1 << 2)
#define GINTMSK_SOF         (1 << 3)
#define GINTMSK_GINNAKEFF   (1 << 6)
#define GINTMSK_GOUTNAKEFF  (1 << 7)
#define GINTMSK_SUSPEND     (1 << 11)
#define GINTMSK_RESET       (1 << 12)
#define GINTMSK_ENUMDONE    (1 << 13)
#define GINTMSK_EPMIS       (1 << 17)
#define GINTMSK_INEP        (1 << 18)
#define GINTMSK_OEP         (1 << 19)
#define GINTMSK_DISCONNECT  (1 << 29)
#define GINTMSK_RESUME      (1 << 31)

#define GOTGINT_SESENDDET	(1 << 2)

#define FIFO_DEPTH_SHIFT 16

#define GNPTXFSTS_GET_TXQSPCAVAIL(x) GET_BITS(x, 16, 8)

#define GHWCFG4_DED_FIFO_EN         (1 << 25)

#define DAINT_ALL                   0xFFFFFFFF
#define DAINT_NONE                  0
#define DAINT_OUT_SHIFT             16
#define DAINT_IN_SHIFT              0

#define DAINTMSK_ALL                0xFFFFFFFF
#define DAINTMSK_NONE               0
#define DAINTMSK_OUT_SHIFT          16
#define DAINTMSK_IN_SHIFT           0

#define DCTL_PROGRAMDONE			(1 << 11)
#define DCTL_CGOUTNAK				(1 << 10)
#define DCTL_SGOUTNAK				(1 << 9)
#define DCTL_CGNPINNAK				(1 << 8)
#define DCTL_SGNPINNAK				(1 << 7)
#define DCTL_GOUTNAKSTS				(1 << 3)
#define DCTL_GPINNAKSTS				(1 << 2)
#define DCTL_SFTDISCONNECT			(1 << 1)

#define DSTS_GET_SPEED(x) GET_BITS(x, 1, 2)

#define DCFG_NZSTSOUTHSHK           (1 << 2)
#define DCFG_EPMSCNT                (1 << 18)
#define DCFG_HISPEED                0x0
#define DCFG_FULLSPEED              0x1
#define DCFG_DEVICEADDR_UNSHIFTED_MASK 0x7F
#define DCFG_DEVICEADDR_SHIFT 4
#define DCFG_DEVICEADDRMSK (DCFG_DEVICEADDR_UNSHIFTED_MASK << DCFG_DEVICEADDR_SHIFT)
#define DCFG_ACTIVE_EP_COUNT_MASK	0x1f
#define DCFG_ACTIVE_EP_COUNT_SHIFT	18

#define DOEPTSIZ0_SUPCNT_MASK 0x3
#define DOEPTSIZ0_SUPCNT_SHIFT 29
#define DOEPTSIZ0_PKTCNT_MASK 0x1
#define DEPTSIZ0_XFERSIZ_MASK 0x7F
#define DIEPTSIZ_MC_MASK 0x3
#define DIEPTSIZ_MC_SHIFT 29
#define DEPTSIZ_PKTCNT_MASK 0x3FF
#define DEPTSIZ_PKTCNT_SHIFT 19
#define DEPTSIZ_XFERSIZ_MASK 0x1FFFF

// ENDPOINT_DIRECTIONS register has two bits per endpoint. 0, 1 for endpoint 0. 1, 2 for end point 1, etc.
#define USB_EP_DIRECTION(ep) (USBDirection)(2-((GET_REG(USB + GHWCFG1) >> ((ep) * 2)) & 0x3))
#define USB_ENDPOINT_DIRECTIONS_BIDIR 0
#define USB_ENDPOINT_DIRECTIONS_IN 1
#define USB_ENDPOINT_DIRECTIONS_OUT 2

#define USB_START_DELAYUS 10000
#define USB_SFTDISCONNECT_DELAYUS 4000
#define USB_ONOFFSTART_DELAYUS 100
#define USB_RESETWAITFINISH_DELAYUS 1000
#define USB_SFTCONNECT_DELAYUS 250
#define USB_PROGRAMDONE_DELAYUS 10

#define USB_EPCON_ENABLE		(1 << 31)
#define USB_EPCON_DISABLE		(1 << 30)
#define USB_EPCON_SETD0PID		(1 << 28)
#define USB_EPCON_SETNAK		(1 << 27)
#define USB_EPCON_CLEARNAK		(1 << 26)
#define USB_EPCON_TXFNUM_MASK	0xf
#define USB_EPCON_TXFNUM_SHIFT	22
#define USB_EPCON_STALL			(1 << 21)
#define USB_EPCON_TYPE_MASK		0x3
#define USB_EPCON_TYPE_SHIFT	18
#define USB_EPCON_NAKSTS		(1 << 17)
#define USB_EPCON_ACTIVE		(1 << 15)
#define USB_EPCON_NEXTEP_MASK	0xF
#define USB_EPCON_NEXTEP_SHIFT	11
#define USB_EPCON_MPS_MASK		0x7FF

#define USB_EPINT_INEPNakEff 0x40
#define USB_EPINT_INTknEPMis 0x20
#define USB_EPINT_INTknTXFEmp 0x10
#define USB_EPINT_TimeOUT 0x8
#define USB_EPINT_AHBErr 0x4
#define USB_EPINT_EPDisbld 0x2
#define USB_EPINT_XferCompl 0x1

#define USB_EPINT_Back2BackSetup (1 << 6)
#define USB_EPINT_OUTTknEPDis 0x10
#define USB_EPINT_SetUp 0x8
#define USB_EPINT_EpDisbld 0x1
#define USB_EPINT_NONE 0
#define USB_EPINT_ALL 0xFFFFFFFF

#define USB_2_0 0x0200

#define USB_HIGHSPEED 0
#define USB_FULLSPEED 1
#define USB_LOWSPEED 2
#define USB_FULLSPEED_48_MHZ 3

#define USB_CONTROLEP 0


#endif


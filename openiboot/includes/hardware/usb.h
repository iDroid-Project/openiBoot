#ifndef HW_USB_H
#define HW_USB_H

#include "hardware/s5l8900.h"

// Device
#ifndef CONFIG_IPHONE_4
#define USB                         0x38400000
#define USB_PHY                     0x3C400000
#else
#define USB                         0x86100000
#define USB_PHY                     0x86000000
#endif

// PHY Registers
#define OPHYPWR                     0		
#define OPHYCLK                     0x4		
#define ORSTCON                     0x8
#define OPHYUNK1                    0x1C
#define OPHYUNK2                    0x44
#define OPHYUNK3                    0x60

// OTG Registers
#define GOTGCTL                     0x0     // core USB configuration
#define GOTGINT                     0x4		
#define GAHBCFG                     0x8
#define GUSBCFG                     0xC
#define GRSTCTL                     0x10    // core USB reset
#define GINTSTS                     0x14    // core interrupt
#define GINTMSK                     0x18    // core interrupt mask
#define GRXSTSR                     0x1C    // receive status queue read
#define GRXSTSP                     0x20    // receive status queue read & pop
#define GRXFSIZ                     0x24    // receive FIFO size
#define GNPTXFSIZ                   0x28    // non periodic transmit FIFO size
#define GNPTXFSTS                   0x2C    // non periodic transmit FIFO/queue status
#define GI2CCTL                     0x30    // I2C access
#define GPVNDCTL                    0x34    // PHY vendor control
#define GGPIO                       0x38    // general purpose input/output
#define GUID                        0x3C    // user ID
#define GSNPSID                     0x40    // synopsys ID (read only)
#define GHWCFG1                     0x44    // user hardware config1 (read only)
#define GHWCFG2                     0x48    // user hardware config2 (read only)
#define GHWCFG3                     0x4C    // user hardware config3 (read only)
#define GHWCFG4                     0x50    // user hardware config4 (read only)

#define DIEPTXF(X)                  (0x104 + 4*X)   // device transmit fifo

#define DCFG                        0x800   // device configuration
#define DCTL                        0x804   // device control
#define DSTS                        0x808   // device status
#define DIEPMSK                     0x810   // device in endpoint common interrupt mask
#define DOEPMSK                     0x814   // device out endpoint common interrupt mask
#define DAINT                       0x818   // device all endpoints interrupt
#define DAINTMSK                    0x81C   // device all endpoints interrupt mask
#define DTKNQR1                     0x820   // device in token queue read register-1 (read only)
#define DTKNQR2                     0x824   // device in token queue read register-2 (read only)
#define DVBUSDIS                    0x828   // device VBUS discharge
#define DVBUSPULSE                  0x82C   // device VBUS pulse
#define DTKNQR3                     0x830   // device in token queue read register-3 (read only)
#define DTHRCTL                     0x830   // device thresholding control (read/write)
#define DTKNQR4                     0x834   // device in token queue read register-4 (read only)
#define FIFOEMPTYMASK               0x834   // device in EPs empty inr. mask (read/write)
#define DEACHINT                    0x838   // device each endpoint interrupt register (read only)
#define DEACHINTMSK                 0x83C   // device each endpoint interrupt mask


#define USB_INREGS                  0x900
#define USB_OUTREGS                 0xB00

#define PCGCCTL                     0xE00

// Values
#ifdef CONFIG_IPHONE_4
#define USB_INTERRUPT               0xD
#else
#define USB_INTERRUPT               0x13
#endif

#ifdef CONFIG_IPHONE_4
#define USB_OTGCLOCKGATE            0x18
#define USB_PHYCLOCKGATE            0x1D
#else
#define USB_OTGCLOCKGATE            0x2
#define USB_PHYCLOCKGATE            0x23
#endif

#define PCGCCTL_ONOFF_MASK          3   // bits 0, 1
#define PCGCCTL_ON                  0
#define PCGCCTL_OFF                 1

#define OPHYPWR_FORCESUSPEND        0x1
#define OPHYPWR_PLLPOWERDOWN        0x2
#define OPHYPWR_XOPOWERDOWN         0x4
#define OPHYPWR_ANALOGPOWERDOWN     0x8
#define OPHYPWR_UNKNOWNPOWERDOWN    0x10
#define OPHYPWR_POWERON             0x0 // all the previous flags are off

#define OPHYCLK_CLKSEL_MASK         0x3
#define OPHYCLK_SPEED_48MHZ         48000000
#define OPHYCLK_SPEED_12MHZ         12000000
#define OPHYCLK_SPEED_24MHZ         24000000
#ifdef CONFIG_IPHONE_4
#define OPHYCLK_CLKSEL_48MHZ        0x2
#define OPHYCLK_CLKSEL_12MHZ        0x0
#define OPHYCLK_CLKSEL_24MHZ        0x1
#define OPHYCLK_CLKSEL_OTHER        0x3
#else
// TODO: these values are off according to iboot 3.1.3. Look into this.
#define OPHYCLK_CLKSEL_48MHZ        0x0
#define OPHYCLK_CLKSEL_12MHZ        0x2
#define OPHYCLK_CLKSEL_24MHZ        0x3
#endif

#ifdef CONFIG_IPHONE_4
#define OPHYUNK1_STOP_MASK          0x2
#define OPHYUNK1_START              0x6
#define OPHYUNK2_START              0x733
#define OPHYUNK3_START              0x200
#endif

#define GOTGCTL_BSESSIONVALID       (1 << 19)
#define GOTGCTL_SESSIONREQUEST      (1 << 1)

#define ORSTCON_PHYSWRESET          0x1
#define ORSTCON_LINKSWRESET         0x2
#define ORSTCON_PHYLINKSWRESET      0x4

#define GAHBCFG_DMAEN               (1 << 5)
#define GAHBCFG_BSTLEN_SINGLE       (0 << 1)
#define GAHBCFG_BSTLEN_INCR         (1 << 1)
#define GAHBCFG_BSTLEN_INCR4        (3 << 1)
#define GAHBCFG_BSTLEN_INCR8        (5 << 1)
#define GAHBCFG_BSTLEN_INCR16       (7 << 1)
#define GAHBCFG_MASKINT             0x1

#define GUSBCFG_TURNAROUND          0xB
#define GUSBCFG_TURNAROUND_MASK     0xF
#define GUSBCFG_TURNAROUND_SHIFT    10
#define GUSBCFG_HNPENABLE           (1 << 9)
#define GUSBCFG_SRPENABLE           (1 << 8)
#define GUSBCFG_PHYIF16BIT          (1 << 3)
#define USB_UNKNOWNREG1_START       0x1708

#define GRSTCTL_CORESOFTRESET       (1 << 0)
#define GRSTCTL_TXFFLSH             (1 << 5)
#define GRSTCTL_TXFNUM_MASK         (0x1f << 6)
#define GRSTCTL_UNKN1               (1 << 11)
#define GRSTCTL_AHBIDLE             (1 << 31)
#define GRSTCTL_UNKN_TX_FIFO_SHIFT  12

#define GINTMSK_NONE                0x0
#define GINTMSK_OTG                 (1 << 2)
#define GINTMSK_SOF                 (1 << 3)
#define GINTMSK_INNAK               (1 << 6)
#define GINTMSK_OUTNAK              (1 << 7)
#define GINTMSK_SUSPEND             (1 << 11)
#define GINTMSK_RESET               (1 << 12)
#define GINTMSK_ENUMDONE            (1 << 13)
#define GINTMSK_EPMIS               (1 << 17)
#define GINTMSK_INEP                (1 << 18)
#define GINTMSK_OEP                 (1 << 19)
#define GINTMSK_DISCONNECT          (1 << 29)
#define GINTMSK_RESUME              (1 << 31)

#define GRXFSIZ_DEPTH               0x11B
#define GNPTXFSIZ_STARTADDR         0x11B
#define GNPTXFSIZ_DEPTH             0x100
#define DPTXFSIZ_BASE_STARTADDR     0x21B
#define DPTXFSIZ_DEPTH              0x100
#define FIFO_DEPTH_SHIFT            16
#define USB_NUM_TX_FIFOS            15

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

#define DCTL_SFTDISCONNECT          (1 << 1)
#define DCTL_SGNPINNAK              (1 << 7)
#define DCTL_CGNPINNAK              (1 << 8)
#define DCTL_SGOUTNAK               (1 << 9)
#define DCTL_CGOUTNAK               (1 << 10)
#define DCTL_PROGRAMDONE            (1 << 11)
#define DCTL_SETD0PID               (1 << 28)
#define DCTL_NEXTEP_MASK            0xF
#define DCTL_NEXTEP_SHIFT           11

#define DSTS_GET_SPEED(x) GET_BITS(x, 1, 2)

#define DCFG_NZSTSOUTHSHK           (1 << 2)
#define DCFG_EPMSCNT                (1 << 18)
#define DCFG_HISPEED                0x0
#define DCFG_FULLSPEED              0x1
#define DCFG_DEVICEADDR_UNSHIFTED_MASK 0x7F
#define DCFG_DEVICEADDR_SHIFT       4
#define DCFG_DEVICEADDRMSK          (DCFG_DEVICEADDR_UNSHIFTED_MASK << DCFG_DEVICEADDR_SHIFT)
#define DCFG_DEVICEADDR_RESET       0

#define DOEPTSIZ0_SUPCNT_MASK       0x3
#define DOEPTSIZ0_SUPCNT_SHIFT      29
#define DOEPTSIZ0_PKTCNT_MASK       0x1
#define DEPTSIZ0_XFERSIZ_MASK       0x7F
#define DIEPTSIZ_MC_MASK            0x3
#define DIEPTSIZ_MC_SHIFT           29
#define DEPTSIZ_PKTCNT_MASK         0x3FF
#define DEPTSIZ_PKTCNT_SHIFT        19
#define DEPTSIZ_XFERSIZ_MASK        0x7FFFF

// ENDPOINT_DIRECTIONS register has two bits per endpoint. 0, 1 for endpoint 0. 2, 3 for end point 1, etc.
#define USB_EP_DIRECTION(ep) ((GET_REG(USB + GHWCFG1) >> ((ep) * 2)) & 0x3)
#define USB_ENDPOINT_DIRECTIONS_BIDIR   0
#define USB_ENDPOINT_DIRECTIONS_IN      1
#define USB_ENDPOINT_DIRECTIONS_OUT 	2

#define USB_START_DELAYUS           10000
#define USB_RESET_DELAYUS           1000
#define USB_SFTDISCONNECT_DELAYUS   4000
#define USB_ONOFFSTART_DELAYUS      100
#define USB_PHYPWRPOWERON_DELAYUS   10
#define USB_RESET2_DELAYUS          20
#define USB_RESETWAITFINISH_DELAYUS 1000
#define USB_SFTCONNECT_DELAYUS      250
#define USB_PROGRAMDONE_DELAYUS     10
#define USB_INTERRUPT_CLEAR_DELAYUS 1

#define USB_EPCON_ENABLE            (1 << 31)
#define USB_EPCON_DISABLE           (1 << 30)
#define USB_EPCON_SET0PID           (1 << 28)
#define USB_EPCON_SETNAK            (1 << 27)
#define USB_EPCON_CLEARNAK          (1 << 26)
#define USB_EPCON_STALL             (1 << 21)
#define USB_EPCON_ACTIVE            (1 << 15)
#define USB_EPCON_TYPE_MASK         0x3
#define USB_EPCON_TYPE_SHIFT        18
#define USB_EPCON_TXFNUM_MASK       0xF
#define USB_EPCON_TXFNUM_SHIFT      22
#define USB_EPCON_MPS_MASK          0x7FF
#define USB_EPCON_NONE              0x0

#define USB_EPXFERSZ_NONE           0x0
#define USB_EPXFERSZ_PKTCNT_BIT_0(x) GET_BITS(x, 19, 1)
#define USB_EPXFERSZ_PKTCNT(x) GET_BITS(x, 19, 10)

// in EP interrupts
#define USB_EPINT_INEPNakEff        (1 << 6)
#define USB_EPINT_INTknEPMis        (1 << 5)
#define USB_EPINT_INTknTXFEmp       (1 << 4)
#define USB_EPINT_TimeOUT           (1 << 3)	// Non ISO in EP timeout
#define USB_EPINT_AHBErr            (1 << 2)	// AHB error. Used for out EPs too.
#define USB_EPINT_EPDisbld          (1 << 1)	// EP disabled. Used for out EPs too.
#define USB_EPINT_XferCompl         (1 << 0)	// Transfer done. Used for out EPs too.

// out EP interrupts
#define USB_EPINT_Back2BackSetup    (1 << 6)	// Back to back setup received
#define USB_EPINT_OUTTknEPDis       (1 << 4)
#define USB_EPINT_SetUp             (1 << 3)	// Control out EP setup phase done

#define USB_EPINT_NONE              0
#define USB_EPINT_ALL               0xFFFFFFFF

#define USB_NUM_ENDPOINTS           6

#define USB_2_0                     0x0200

#define USB_HIGHSPEED               0
#define USB_FULLSPEED               1
#define USB_LOWSPEED                2
#define USB_FULLSPEED_48_MHZ        3

#define USB_CONTROLEP               0

#endif


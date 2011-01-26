#ifndef HW_MIPI_DSI_H
#define HW_MIPI_DSI_H

// Device
#define MIPI_DSIM 0x89000000
#define MIPI_DSIM_CLOCKGATE 0x19

// Registers
#define STATUS          0x0     // status register
#define SWRST           0x4     // software reset register
#define CLKCTRL         0x8     // clock control register
#define TIMEOUT         0xC     // time out register
#define CONFIG          0x10    // configuration register
#define ESCMODE         0x14    // escape mode register
#define MDRESOL         0x18    // main display image resolution register
#define MVPORCH         0x1C    // main vertical porch register
#define MHPORCH         0x20    // main horizontal porch register
#define MSYNC           0x24    // main sync area register
#define SDRESOL         0x28    // sub display image resolution register
#define INTSRC          0x2C    // interrupt source register
#define INTMSK          0x30    // interrupt mask register
#define PKTHDR          0x34    // packet header fifo register
#define PAYLOAD         0x38    // payload fifo register
#define RXFIFO          0x3C    // read fifo register
#define FIFOTHLD        0x40    // fifo threshold level register
#define FIFOCTRL        0x44    // fifo control register
#define MEMACCHR        0x48    // fifo memory AC characteristic register
#define PLLCTRL         0x4C    // pll control register
#define PLLTMR          0x50    // pll timer register
#define PHYACCHR        0x54    // d-phy AC characteristic register
#define PHYACCHR1       0x58    // secondary d-phy AC characteristic register

// Values

#define DATA_0  (1 << 0)
#define DATA_1  (1 << 1) 
#define DATA_2  (1 << 2)
#define DATA_3  (1 << 3)

// data lines enabled (depends on platform)
#define DATA_LANES_ENABLED (DATA_0 | DATA_1)
#define NUM_DATA_LANES 2

#define STATUS_STOP                 (1 << 8)
#define STATUS_ULPS                 (1 << 9)
#define STATUS_HS_READY             (1 << 10)
#define STATUS_RESET                (1 << 11)
#define STATUS_SWRST                (1 << 20)
#define STATUS_PLL_STABLE           (1 << 31)
#define STATUS_DATA_STOP_SHIFT      0
#define STATUS_DATA_STOP_MASK       0xf
#define STATUS_DATA_STOP(x)         ((x & STATUS_DATA_STOP_MASK) << STATUS_DATA_STOP_SHIFT)
#define STATUS_DATA_ULPS_SHIFT      4
#define STATUS_DATA_ULPS_MASK       0xf
#define STATUS_DATA_ULPS(x)         ((x & STATUS_DATA_ULPS_MASK) << STATUS_DATA_ULPS_SHIFT)

#define SWRST_RESET                 0x1

#define CLKCTRL_ESC_EN_CLK          (1 << 19)
#define CLKCTRL_ESC_EN_DATA_0       (1 << 20)
#define CLKCTRL_ESC_EN_DATA_1       (1 << 21)
#define CLKCTRL_ESC_EN_DATA_2       (1 << 22)
#define CLKCTRL_ESC_EN_DATA_3       (1 << 23)
#define CLKCTRL_BYTE_CLKEN          (1 << 24)
#define CLKCTRL_BYTE_CLK_SRC        (1 << 25)
#define CLKCTRL_PLL_BYPASS          (1 << 27)
#define CLKCTRL_ESC_CLKEN           (1 << 28)
#define CLKCTRL_TX_REQ_HSCLK        (1 << 31)
#define CLKCTRL_ESC_PRESCALER_SHIFT 0
#define CLKCTRL_ESC_PRESCALER_MASK  0xf
#define CLKCTRL_ESC_PRESCALER(x)    (((x) & CLKCTRL_ESC_PRESCALER_MASK) << CLKCTRL_ESC_PRESCALER_SHIFT)
#define CLKCTRL_ESC_EN_DATA_SHIFT   20
#define CLKCTRL_ESC_EN_DATA_MASK    0xf
#define CLKCTRL_ESC_EN_DATA(x)      (((x) & CLKCTRL_ESC_EN_DATA_MASK) << CLKCTRL_ESC_EN_DATA_SHIFT)
#define CLKCTRL_NONE                0

#define CONFIG_EN_CLK               (1 << 0)
#define CONFIG_EN_DATA_O            (1 << 1)
#define CONFIG_EN_DATA_1            (1 << 2)
#define CONFIG_EN_DATA_2            (1 << 3)
#define CONFIG_EN_DATA_3            (1 << 4)
#define CONFIG_HSE_MODE             (1 << 23)
#define CONFIG_AUTO_MODE            (1 << 24)
#define CONFIG_VIDEO_MODE           (1 << 25)
#define CONFIG_BURST_MODE           (1 << 26)
#define CONFIG_EN_DATA_SHIFT        1
#define CONFIG_EN_DATA_MASK         0xf
#define CONFIG_EN_DATA(x)           (((x) & CONFIG_EN_DATA_MASK) << CONFIG_EN_DATA_SHIFT)
#define CONFIG_NUM_DATA_SHIFT       5       // NUM_DATA is 0-based. Note the x-1 in the macro
#define CONFIG_NUM_DATA_MASK        0x3
#define CONFIG_NUM_DATA(x)          ((((x) - 1) & CONFIG_NUM_DATA_MASK) << CONFIG_NUM_DATA_SHIFT)
#define CONFIG_MAIN_PXL_FMT_SHIFT   12
#define CONFIG_MAIN_PXL_FMT_MASK    0xf
#define CONFIG_MAIN_PXL_FMT(x)      (((x) & CONFIG_MAIN_PXL_FMT_MASK) << CONFIG_MAIN_PXL_FMT_SHIFT)

#define ESCMODE_TX_UIPS_CLK_EXIT    (1 << 0)
#define ESCMODE_TX_UIPS_CLK         (1 << 1)
#define ESCMODE_TX_UIPS_EXIT        (1 << 2)
#define ESCMODE_TX_UIPS_DAT         (1 << 3)
#define ESCMODE_TX_TRIGGER_RST      (1 << 4)
#define ESCMODE_TX_LP               (1 << 6)
#define ESCMODE_CMD_LP              (1 << 7)
#define ESCMODE_FORCE_STOP          (1 << 20)
#define ESCMODE_NONE                0

// these are valid for both MDRESOL and SDRESOL
#define DRESOL_STAND_BY             (1 << 31)
#define DRESOL_HRESOL_SHIFT         0
#define DRESOL_HRESOL_MASK          0x7ff
#define DRESOL_HRESOL(x)            (((x) & DRESOL_HRESOL_MASK) << DRESOL_HRESOL_SHIFT)
#define DRESOL_VRESOL_SHIFT         16
#define DRESOL_VRESOL_MASK          0x7ff
#define DRESOL_VRESOL(x)            (((x) & DRESOL_VRESOL_MASK) << DRESOL_VRESOL_SHIFT)

#define MVPORCH_VBP_SHIFT           0
#define MVPORCH_VBP_MASK            0x7ff
#define MVPORCH_VBP(x)              (((x) & MVPORCH_VBP_MASK) << MVPORCH_VBP_SHIFT)
#define MVPORCH_VFP_SHIFT           16
#define MVPORCH_VFP_MASK            0x7ff
#define MVPORCH_VFP(x)              ((x & MVPORCH_VFP_MASK) << MVPORCH_VFP_SHIFT)
#define MVPORCH_CMD_ALLOW_SHIFT     28
#define MVPORCH_CMD_ALLOW_MASK      0xf
#define MVPORCH_CMD_ALLOW(x)        ((x & MVPORCH_CMD_ALLOW_MASK) << MVPORCH_CMD_ALLOW_SHIFT)

#define MHPORCH_HBP_SHIFT           0
#define MHPORCH_HBP_MASK            0xffff
#define MHPORCH_HBP(x)              (((x) & MHPORCH_HBP_MASK) << MHPORCH_HBP_SHIFT)
#define MHPORCH_HFP_SHIFT           16
#define MHPORCH_HFP_MASK            0xffff
#define MHPORCH_HFP(x)              (((x) & MHPORCH_HFP_MASK) << MHPORCH_HFP_SHIFT)

#define MSYNC_HSPW_SHIFT            0
#define MSYNC_HSPW_MASK             0xffff
#define MSYNC_HSPW(x)               (((x) & MSYNC_HSPW_MASK) << MSYNC_HSPW_SHIFT)
#define MSYNC_VSPW_SHIFT            22
#define MSYNC_VSPW_MASK             0x3ff
#define MSYNC_VSPW(x)               (((x) & MSYNC_VSPW_MASK) << MSYNC_VSPW_SHIFT)

#define PKTHDR_DATA_ID_SHIFT        0
#define PKTHDR_DATA_ID_MASK         0xff
#define PKTHDR_DATA_ID(x)           (((x) & PKTHDR_DATA_ID_MASK) << PKTHDR_DATA_ID_SHIFT)
#define PKTHDR_DATA0_SHIFT          8
#define PKTHDR_DATA0_MASK           0xff
#define PKTHDR_DATA0(x)             (((x) & PKTHDR_DATA0_MASK) << PKTHDR_DATA0_SHIFT)
#define PKTHDR_DATA1_SHIFT          16
#define PKTHDR_DATA1_MASK           0xff
#define PKTHDR_DATA1(x)             (((x) & PKTHDR_DATA1_MASK) << PKTHDR_DATA1_SHIFT)

#define FIFOCTRL_MAIN_DISP_FIFO     (1 << 0)
#define FIFOCTRL_SUB_DISP_FIFO      (1 << 1)
#define FIFOCTRL_I80_FIFO           (1 << 2)
#define FIFOCTRL_TX_SFR_FIFO        (1 << 3)
#define FIFOCTRL_RX_FIFO            (1 << 4)
#define FIFOCTRL_UNKN_1             (1 << 22)

#define PLLCTRL_PMS                 (1 << 1)
#define PLLCTRL_PLL_EN              (1 << 23)
#define PLLCTRL_FREQ_BAND_SHIFT     24
#define PLLCTRL_FREQ_BAND_MASK      0xf
#define PLLCTRL_FREQ_BAND(x)        (((x) & PLLCTRL_FREQ_BAND_MASK) << PLLCTRL_FREQ_BAND_SHIFT)
#define PLLCTRL_NONE                0


// Other settings
#define PLL_STABLE_TIME             150000

#define AFC_CTL(x)			(((x) & 0x7) << 5)
#define AFC_ENABLE			(1 << 14)
#define AFC_DISABLE			(0 << 14)


#endif

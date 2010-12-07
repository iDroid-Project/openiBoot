#include "openiboot.h"
#include "usbphy.h"
#include "hardware/usbphy.h"
#include "timer.h"
#include "clock.h"

void usb_phy_init() {
	// power on PHY
	SET_REG(USB_PHY + OPHYUNK4, OPHYUNK4_START);
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_PLLPOWERDOWN | OPHYPWR_XOPOWERDOWN);

	udelay(USB_PHYPWRPOWERON_DELAYUS);

	SET_REG(USB_PHY + OPHYUNK1, OPHYUNK1_START);
	SET_REG(USB_PHY + OPHYUNK2, OPHYUNK2_START);
	
	// select clock
	uint32_t phyClockBits;
	switch (clock_get_frequency(FrequencyBaseUsbPhy))
	{
		case OPHYCLK_SPEED_12MHZ:
			phyClockBits = OPHYCLK_CLKSEL_12MHZ;
			break;

		case OPHYCLK_SPEED_24MHZ:
			phyClockBits = OPHYCLK_CLKSEL_24MHZ;
			break;

		case OPHYCLK_SPEED_48MHZ:
			phyClockBits = OPHYCLK_CLKSEL_48MHZ;
			break;

		default:
			phyClockBits = OPHYCLK_CLKSEL_OTHER;
			break;
	}
	SET_REG(USB_PHY + OPHYCLK, (GET_REG(USB_PHY + OPHYCLK) & ~OPHYCLK_CLKSEL_MASK) | phyClockBits);

	// reset phy
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) | ORSTCON_PHYSWRESET);
	udelay(USB_RESET2_DELAYUS);
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) & (~ORSTCON_PHYSWRESET));
	udelay(USB_RESET_DELAYUS);
}

void usb_phy_shutdown() {
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_FORCESUSPEND | OPHYPWR_PLLPOWERDOWN
		| OPHYPWR_XOPOWERDOWN | OPHYPWR_ANALOGPOWERDOWN | OPHYPWR_UNKNOWNPOWERDOWN); // power down phy
	SET_REG(USB_PHY + ORSTCON, ORSTCON_PHYSWRESET | ORSTCON_LINKSWRESET | ORSTCON_PHYLINKSWRESET); // reset phy/link

	udelay(USB_RESET_DELAYUS);	// wait a millisecond for the changes to stick
}


